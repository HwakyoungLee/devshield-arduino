/*

BERGCloud library common API

Copyright (c) 2013 BERG Ltd. http://bergcloud.com/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/


#define __STDC_LIMIT_MACROS /* Include C99 stdint defines in C++ code */
#include <stdint.h>
#include <stddef.h>
#include <string.h> /* For memcpy() */

#include "BERGCloudBase.h"

#define SPI_PROTOCOL_HEADER_SIZE (5) // Data length, CRC16 and command/status
#define MAX_DATA_SIZE (MAX_SERIAL_DATA + SPI_PROTOCOL_HEADER_SIZE)

#define SPI_PROTOCOL_PAD    (0xff)
#define SPI_PROTOCOL_RESET  (0xf5)

#define POLL_TIMEOUT_MS (1000)
#define SYNC_TIMEOUT_MS (1000)

uint8_t CBERGCloudBase::nullProductID[16] = {0};

uint16_t CBERGCloudBase::Crc16(uint8_t data, uint16_t crc)
{
  /* From Ember's code */
  crc = (crc >> 8) | (crc << 8);
  crc ^= data;
  crc ^= (crc & 0xff) >> 4;
  crc ^= (crc << 8) << 4;

  crc ^= ( (uint8_t) ( (uint8_t) ( (uint8_t) (crc & 0xff) ) << 5)) |
    ((uint16_t) ( (uint8_t) ( (uint8_t) (crc & 0xff)) >> 3) << 8);

  return crc;
}

bool CBERGCloudBase::transaction(_BC_TRANSACTION *pTr)
{
  uint16_t i;
  uint8_t rxByte;
  bool timeout;
  uint16_t dataSize;
  uint16_t dataCRC;
  uint16_t calcCRC;
  uint8_t header[SPI_PROTOCOL_HEADER_SIZE];
  uint16_t commandSize;
  bool crcRxParam;

  /* Validate parameters */
  if (  ((pTr->pTx == NULL) && (pTr->txSize != 0)) ||
        (pTr->txSize > MAX_SERIAL_DATA) )
  {
    _LOG("Invalid parameter (CBERGCloudBase::transaction)\r\n");
    return false;
  }

  /* Check synchronisation */
  if (!m_synced)
  {
    timerReset();
    timeout = false;

    do {
      rxByte = SPITransaction(SPI_PROTOCOL_PAD, true);
      timeout = timerRead_mS() > SYNC_TIMEOUT_MS;

    } while ((rxByte != SPI_PROTOCOL_RESET) && !timeout);

    if (timeout)
    {
      _LOG("Timeout, sync (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    /* Resynchronisation successful */
    m_synced = true;
  }

  /* Command size is header plus data */
  commandSize = SPI_PROTOCOL_HEADER_SIZE + pTr->txSize;

  /* Add size of optional paramenter, if present */
  if (pTr->pTxParam != NULL)
  {
    commandSize += sizeof(uint16_t);
  }

  /* Set command size in header */
  header[0] = commandSize >> 8;    /* MSByte */
  header[1] = commandSize & 0xff;  /* LSByte */

  /* Zero CRC in header */
  header[2] = 0;
  header[3] = 0;

  /* Set command */
  header[4] = pTr->command;

  /* Calculate CRC (header and data) */
  calcCRC = 0xffff;
  
  for (i=0; i<SPI_PROTOCOL_HEADER_SIZE; i++)
  {
    calcCRC = Crc16(header[i], calcCRC);
  }

  /* CRC optional parameter */
  if (pTr->pTxParam != NULL)
  {
    calcCRC = Crc16((uint8_t)(*pTr->pTxParam >> 8), calcCRC);
    calcCRC = Crc16((uint8_t)(*pTr->pTxParam), calcCRC);
  }

  for (i=0; i<pTr->txSize; i++)
  {
    calcCRC = Crc16(pTr->pTx[i], calcCRC);
  }

  /* Set CRC in header */
  header[2] = calcCRC >> 8;    /* MSByte */
  header[3] = calcCRC & 0xff;  /* LSByte */

  /* Send header */
  for (i=0; i<SPI_PROTOCOL_HEADER_SIZE; i++)
  {
    rxByte = SPITransaction(header[i], false);

    if (rxByte == SPI_PROTOCOL_RESET)
    {
      _LOG("Reset, send header (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    if (rxByte != SPI_PROTOCOL_PAD)
    {
      _LOG("SyncErr, send header (CBERGCloudBase::transaction)\r\n");
      m_synced = false;
      return false;
    }
  }

  /* Send optional parameter */
  if (pTr->pTxParam != NULL)
  {
    rxByte = SPITransaction((uint8_t)(*pTr->pTxParam >> 8), false);

    if (rxByte == SPI_PROTOCOL_RESET)
    {
      _LOG("Reset, send data (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    if (rxByte != SPI_PROTOCOL_PAD)
    {
      _LOG("SyncErr, send data (CBERGCloudBase::transaction)\r\n");
      m_synced = false;
      return false;
    }

    rxByte = SPITransaction((uint8_t)(*pTr->pTxParam), false);

    if (rxByte == SPI_PROTOCOL_RESET)
    {
      _LOG("Reset, send data (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    if (rxByte != SPI_PROTOCOL_PAD)
    {
      _LOG("SyncErr, send data (CBERGCloudBase::transaction)\r\n");
      m_synced = false;
      return false;
    }
  }

  /* Send data */
  for (i=0; i<pTr->txSize; i++)
  {
    rxByte = SPITransaction(pTr->pTx[i], false);

    if (rxByte == SPI_PROTOCOL_RESET)
    {
      _LOG("Reset, send data (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    if (rxByte != SPI_PROTOCOL_PAD)
    {
      _LOG("SyncErr, send data (CBERGCloudBase::transaction)\r\n");
      m_synced = false;
      return false;
    }
  }

  /* Poll for response */
  timerReset();
  timeout = false;

  do {
    rxByte = SPITransaction(SPI_PROTOCOL_PAD, false);

    if (rxByte == SPI_PROTOCOL_RESET)
    {
      _LOG("Reset, poll (CBERGCloudBase::transaction)\r\n");
      return false;
    }

    // PW TODO: Check if != PAD -> synched=FALSE
    timeout = timerRead_mS() > POLL_TIMEOUT_MS;
    
  } while ((rxByte == SPI_PROTOCOL_PAD) && !timeout);

  if (timeout)
  {
    _LOG("Timeout, poll (CBERGCloudBase::transaction)\r\n");
    m_synced = false;
    return false;
  }

  // PW TODO: Should we 'escape' 0xf5?

  /* Read header, we already have the first byte */
  header[0] = rxByte;

  for (i=1; i < SPI_PROTOCOL_HEADER_SIZE; i++)
  {
    header[i] = SPITransaction(SPI_PROTOCOL_PAD, false);
  }

  /* Read command size (header plus data) */
  commandSize = header[0]; /* MSByte */
  commandSize <<= 8;
  commandSize |= header[1]; /* LSByte */

  /* Validate command size */
  if (pTr->pRxParam == NULL)
  {
    if (commandSize > MAX_DATA_SIZE)
    {
      /* Too big */
      _LOG("SizeErr, read header (CBERGCloudBase::transaction)\r\n");
      m_synced = false;
      return false;
    }
  }
  else
  {
    if (commandSize > (MAX_DATA_SIZE + sizeof(uint16_t)) )
    {
      /* Too big */
      _LOG("SizeErr, read header (CBERGCloudBase::transaction)\r\n");
      m_synced = false;
      return false;
    }
  }


  if (commandSize < SPI_PROTOCOL_HEADER_SIZE)
  {
    /* Too small */
    _LOG("SizeErr, read header (CBERGCloudBase::transaction)\r\n");
    m_synced = false;
    return false;
  }

  /* Calculate data size */
  dataSize = commandSize - SPI_PROTOCOL_HEADER_SIZE;

  /* Read optional paramenter */
  crcRxParam = false;
  if (pTr->pRxParam != NULL)
  {
    if (dataSize >= sizeof(uint16_t))
    {
      *pTr->pRxParam = SPITransaction(SPI_PROTOCOL_PAD, false);
      *pTr->pRxParam <<= 8;
      *pTr->pRxParam |= SPITransaction(SPI_PROTOCOL_PAD, false);
      dataSize -= sizeof(uint16_t);
      crcRxParam = true;
    }
  }

  /* Read the remaining data */
  for (i = 0; i < dataSize; i++)
  {
    rxByte = SPITransaction(SPI_PROTOCOL_PAD, i < dataSize ? false : true);
    
    if (pTr->pRx != NULL)
    {
      pTr->pRx[i] = rxByte;
    }
  }

  /* Read CRC */
  dataCRC = header[2]; /* MSByte */
  dataCRC <<= 8;
  dataCRC |= header[3]; /* LSByte */

  /* Clear CRC bytes */
  header[2] = 0;
  header[3] = 0;

  /* Calculate CRC (header and data) */
  calcCRC = 0xffff;

  for (i=0; i<SPI_PROTOCOL_HEADER_SIZE; i++)
  {
    calcCRC = Crc16(header[i], calcCRC);
  }

  /* CRC optional parameter */
  if (crcRxParam)
  {
    calcCRC = Crc16((uint8_t)(*pTr->pRxParam >> 8), calcCRC);
    calcCRC = Crc16((uint8_t)(*pTr->pRxParam), calcCRC);
  }

  for (i=0; i<dataSize; i++)
  {
    calcCRC = Crc16(pTr->pRx[i], calcCRC);
  }

  if (calcCRC != dataCRC)
  {
    /* Invalid CRC */
    _LOG("CRCErr, read data (CBERGCloudBase::transaction)\r\n");
    m_synced = false;
    return false;
  }

  /* Return Rx data size */
  if (pTr->pRxSize != NULL)
  {
    *pTr->pRxSize = dataSize;
  }

  /* Store reponse */
  m_lastResponse = header[4];

  return (m_lastResponse == SPI_RSP_SUCCESS);
}

void CBERGCloudBase::initTransaction(_BC_TRANSACTION *pTr)
{
  memset(pTr, 0x00, sizeof(_BC_TRANSACTION));
}

bool CBERGCloudBase::pollForCommand(uint8_t *pCommandBuffer, uint16_t commandBufferSize, uint16_t& commandSize, uint8_t& commandID)
{
  /* Returns TRUE if a valid command has been received */

  _BC_TRANSACTION tr;
  uint16_t cmd_id;

  initTransaction(&tr);

  tr.command = SPI_CMD_POLL_FOR_COMMAND;
  tr.pRx = pCommandBuffer;
  tr.rxMaxSize = commandBufferSize;
  tr.pRxSize = &commandSize;
  tr.pRxParam = &cmd_id;

  if (transaction(&tr))
  {
    commandID = (uint8_t)cmd_id;
    return true;
  }

  commandID = 0;
  commandSize = 0;
  return false;
}

bool CBERGCloudBase::pollForCommand(CBuffer& buffer, uint8_t& commandID)
{
  /* Returns TRUE if a valid command has been received */

  _BC_TRANSACTION tr;
  uint16_t cmd_id;
  uint16_t cmd_size;

  initTransaction(&tr);
  buffer.clear();

  tr.command = SPI_CMD_POLL_FOR_COMMAND;
  tr.pRx = buffer.ptr();
  tr.rxMaxSize = buffer.size();
  tr.pRxSize = &cmd_size;
  tr.pRxParam = &cmd_id;

  if (transaction(&tr))
  {
    commandID = (uint8_t)cmd_id;
    buffer.used(cmd_size);
    return true;
  }

  commandID = 0;
  buffer.used(0);
  return false;
}

bool CBERGCloudBase::sendEvent(uint8_t eventCode, uint8_t *pEventBuffer, uint16_t eventSize)
{
  /* Returns TRUE if the event is sent successfully */

  _BC_TRANSACTION tr;
  uint16_t temp;

  initTransaction(&tr);

  temp = BC_EVENT_START_BINARY | eventCode;

  tr.command = SPI_CMD_SEND_EVENT;
  tr.pTx = pEventBuffer;
  tr.txSize = eventSize ;
  tr.pTxParam = &temp;

  return transaction(&tr);
}

bool CBERGCloudBase::sendEvent(uint8_t eventCode, CBuffer& buffer)
{
  _BC_TRANSACTION tr;
  uint16_t temp;
  bool result;

  initTransaction(&tr);

  temp = BC_EVENT_START_PACKED | eventCode;

  tr.command = SPI_CMD_SEND_EVENT;
  tr.pTx = buffer.ptr();
  tr.txSize = buffer.used();
  tr.pTxParam = &temp;

  result = transaction(&tr);

  buffer.clear();
  return result;
}

bool CBERGCloudBase::getNetworkState(uint8_t& state)
{
  _BC_TRANSACTION tr;

  initTransaction(&tr);

  tr.command = SPI_CMD_GET_NETWORK_STATE;
  tr.pRx = &state;
  tr.rxMaxSize = sizeof(uint8_t);

  return transaction(&tr);
}

bool CBERGCloudBase::getSignalQuality(int8_t& rssi, uint8_t& lqi)
{
  _BC_TRANSACTION tr;
  uint8_t temp[2];

  initTransaction(&tr);

  tr.command = SPI_CMD_GET_SIGNAL_QUALITY;
  tr.pRx = temp;
  tr.rxMaxSize = sizeof(temp);

  if (transaction(&tr))
  {
    *(uint8_t *)&rssi = temp[0];
    lqi = temp[1];
    return true;
  }

  return false;
}

bool CBERGCloudBase::joinNetwork(const uint8_t productID[16], uint32_t version)
{
  _BC_TRANSACTION tr;
  uint8_t temp[16 + sizeof(version)];

  initTransaction(&tr);

  memcpy(&temp[0], &productID[0], 16);
  temp[16] = version >> 24;
  temp[17] = version >> 16;
  temp[18] = version >> 8;
  temp[19] = version;

  tr.command = SPI_CMD_SEND_PRODUCT_ANNOUNCE;
  tr.pTx = temp;
  tr.txSize = sizeof(temp);

  return transaction(&tr);
}

bool CBERGCloudBase::getClaimingState(uint8_t& state)
{
  _BC_TRANSACTION tr;

  initTransaction(&tr);

  tr.command = SPI_CMD_GET_CLAIM_STATE;
  tr.pRx = &state;
  tr.rxMaxSize = sizeof(uint8_t);

  return transaction(&tr);
}

bool CBERGCloudBase::getClaimcode(char claimcode[BC_CLAIMCODE_SIZE_BYTES])
{
  _BC_TRANSACTION tr;

  initTransaction(&tr);

  tr.command = SPI_CMD_GET_CLAIMCODE;
  tr.pRx = (uint8_t *)claimcode;
  tr.rxMaxSize = sizeof(claimcode);

  return transaction(&tr);
}

bool CBERGCloudBase::getEUI64(uint8_t type, uint8_t eui64[BC_EUI64_SIZE_BYTES])
{
  _BC_TRANSACTION tr;

  initTransaction(&tr);

  tr.command = SPI_CMD_GET_EUI64;
  tr.pTx = &type;
  tr.txSize = sizeof(uint8_t);
  tr.pRx = eui64;
  tr.rxMaxSize = sizeof(eui64);

  return transaction(&tr);
}

bool CBERGCloudBase::setDisplayStyle(uint8_t style)
{
  _BC_TRANSACTION tr;

  initTransaction(&tr);

  tr.command = SPI_CMD_SET_DISPLAY_STYLE;
  tr.pTx = &style;
  tr.txSize = sizeof(uint8_t);

  return transaction(&tr);
}

bool CBERGCloudBase::print(const char *pString)
{
  _BC_TRANSACTION tr;
  uint8_t strLen = 0;
  const char *pTmp = pString;

  if (pString == NULL)
  {
    return false;
  }

  initTransaction(&tr);

  /* Get string length excluding terminator */
  while ((*pTmp++ != '\0') && (strLen < UINT8_MAX))
  {
    strLen++;
  }

  tr.command = SPI_CMD_DISPLAY_PRINT;
  tr.pTx = (uint8_t *)pString;
  tr.txSize = strLen;

  return transaction(&tr);
}

uint8_t CBERGCloudBase::SPITransaction(uint8_t dataOut, bool finalCS)
{
  uint8_t dataIn = 0;

  SPITransaction(&dataOut, &dataIn, (uint16_t)1, finalCS);

  return dataIn;
}

void CBERGCloudBase::begin(void)
{
  m_synced = false;
  m_lastResponse = SPI_RSP_SUCCESS;
}

void CBERGCloudBase::end(void)
{
}
