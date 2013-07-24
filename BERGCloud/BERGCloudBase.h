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


#ifndef BERGCLOUDBASE_H
#define BERGCLOUDBASE_H

#include "BERGCloudConfig.h"
#include "BERGCloudConst.h"
#include "Buffer.h"
#include "LogPrint.h"

#define BERGCLOUD_LIB_VERSION (0x0100)
#define MAX_SERIAL_DATA (64)

typedef struct {
  uint8_t command;
  uint8_t *pTx;
  uint16_t txSize;
  uint8_t *pRx;
  uint16_t rxMaxSize;
  uint16_t *pRxSize;
  uint16_t *pTxParam;
  uint16_t *pRxParam;
} _BC_TRANSACTION;

class CBERGCloudBase
{
public:
  bool pollForCommand(uint8_t *pCommandBuffer, uint16_t commandBufferSize, uint16_t& commandSize, uint8_t& commandID);
  bool pollForCommand(CBuffer& buffer, uint8_t& commandID);
  bool sendEvent(uint8_t eventCode, uint8_t *pEventBuffer, uint16_t eventSize);
  bool sendEvent(uint8_t eventCode, CBuffer& buffer);
  bool getNetworkState(uint8_t& state);
  bool getSignalQuality(int8_t& rssi, uint8_t& lqi);
  bool joinNetwork(const uint8_t productID[16] = nullProductID, uint32_t version = 0);
  bool getClaimingState(uint8_t& state);
  bool getClaimcode(char claimcode[BC_CLAIMCODE_SIZE_BYTES]);
  bool getEUI64(uint8_t type, uint8_t eui64[BC_EUI64_SIZE_BYTES]);
  bool setDisplayStyle(uint8_t style);
  bool print(const char *pText);
  uint8_t m_lastResponse;
  static uint8_t nullProductID[16];
protected:
  void begin(void);
  void end(void);
  void logError(uint8_t value);
  virtual uint16_t SPITransaction(uint8_t *pDataOut, uint8_t *pDataIn, uint16_t dataSize, bool finalCS) = 0;
  virtual void timerReset(void) = 0;
  virtual uint32_t timerRead_mS(void) = 0;
private:
  uint16_t Crc16(uint8_t data, uint16_t crc);
  uint8_t SPITransaction(uint8_t data, bool finalCS);
  void initTransaction(_BC_TRANSACTION *pTr);
  bool transaction(_BC_TRANSACTION *tr);
  bool m_synced;
};

#endif // #ifndef BERGCLOUDBASE_H
