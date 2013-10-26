/*

BERGCloud library for Arduino

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


#ifdef ARDUINO

#include <stdint.h>
#include <stddef.h>

#include "BERGCloudArduino.h"

CBERGCloud BERGCloud;

uint16_t CBERGCloud::SPITransaction(uint8_t *pDataOut, uint8_t *pDataIn, uint16_t dataSize, bool finalCS)
{
  uint16_t i;

  if ( (pDataOut == NULL) || (pDataIn == NULL) || (m_pSPI == NULL) )
  {
    _LOG("Invalid parameter (CBERGCloudArduino::SPITransaction)\r\n");
    return 0;
  }

  digitalWrite(m_nSSELPin, LOW);

  for (i = 0; i < dataSize; i++)
  {
    *pDataIn++ = m_pSPI->transfer(*pDataOut++);
  }

  if (finalCS)
  {
    digitalWrite(m_nSSELPin, HIGH);
  }

  return dataSize;
}

void CBERGCloud::timerReset(void)
{
  m_resetTime = millis();
}

uint32_t CBERGCloud::timerRead_mS(void)
{
  return millis() - m_resetTime;
}

void CBERGCloud::begin(SPIClass *pSPI, uint8_t nSSELPin)
{
  /* Call base class method */
  CBERGCloudBase::begin();

  /* Configure nSSEL control pin */
  m_nSSELPin = nSSELPin;
  pinMode(m_nSSELPin, OUTPUT);

  /* Configure SPI */
  m_pSPI = pSPI;

  if (m_pSPI == NULL)
  {
    _LOG("pSPI is NULL (CBERGCloudArduino::begin)\r\n");
    return;
  }

  m_pSPI->begin();
  m_pSPI->setBitOrder(MSBFIRST);
  m_pSPI->setDataMode(SPI_MODE0);
  m_pSPI->setClockDivider(SPI_CLOCK_DIV4);
}

void CBERGCloud::end()
{
  /* Deconfigure SPI */
  if (m_pSPI != NULL)
  {
    m_pSPI->end();
  }

  /* Deconfigure nSSEL control pin */
  pinMode(m_nSSELPin, INPUT_PULLUP);

  /* Call base class method */
  CBERGCloudBase::end();
}

bool CBERGCloud::print(String& s)
{
  uint8_t text[BC_PRINT_MAX_CHARS + 1]; /* +1 for null terminator */

  /* Convert to C string */
  s.getBytes(text, sizeof(text));

  /* Print */
  return print((const char *)text);
}

#ifdef BERGCLOUD_PACK_UNPACK

bool CMessage::pack(double& n)
{
  /* For 16-bit Arduino platforms we can treat a double literal */
  /* value as as float; this is so pack(1.234) will work. */

  if (sizeof(double) == sizeof(float))
  {
     return pack((float)n);
  }

   _LOG("Pack: 8-byte double type is not supported.\r\n");
  return false;
}

bool CMessage::pack(String& s)
{
  uint16_t strLen = s.length();
  uint32_t i = 1; /* Character index starts at one */

  /* Add header */
  if (!pack_raw_header(strLen))
  {
    return false;
  }

  /* Add data */
  while (strLen-- > 0)
  {
    add((uint8_t)s.charAt(i++));
  }

  return true;
}

bool CMessage::unpack(String& s)
{
  uint16_t sizeInBytes;

  if (!unpack_raw_header(&sizeInBytes))
  {
    return false;
  }

  if (!remaining(sizeInBytes))
  {
    _LOG_UNPACK_ERROR_NO_DATA;
    return false;
  }

  s = ""; /* Empty string */
  while (sizeInBytes-- > 0)
  {
    s += (char)read();
  }

  return true;
}

#endif // #ifdef BERGCLOUD_PACK_UNPACK

#endif // #ifdef ARDUINO
