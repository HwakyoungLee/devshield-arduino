/*

BERGCloud library for Arduino

Copyright (c) 2013 BERG Cloud Ltd. http://bergcloud.com/

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

BERGCloudArduino BERGCloud;

uint16_t BERGCloudArduino::SPITransaction(uint8_t *dataOut, uint8_t *dataIn, uint16_t dataSize, bool finalCS)
{
  uint16_t i;

  if ( (dataOut == NULL) || (dataIn == NULL) || (spi == NULL) )
  {
    _LOG("Invalid parameter (CBERGCloudArduino::SPITransaction)\r\n");
    return 0;
  }

  digitalWrite(nSSELPin, LOW);

  for (i = 0; i < dataSize; i++)
  {
    *dataIn++ = spi->transfer(*dataOut++);
  }

  if (finalCS)
  {
    digitalWrite(nSSELPin, HIGH);
  }

  return dataSize;
}

void BERGCloudArduino::timerReset(void)
{
  resetTime = millis();
}

uint32_t BERGCloudArduino::timerRead_mS(void)
{
  return millis() - resetTime;
}

void BERGCloudArduino::begin(SPIClass *_spi, uint8_t _nSSELPin)
{
  /* Call base class method */
  BERGCloudBase::begin();

  /* Configure nSSEL control pin */
  nSSELPin = _nSSELPin;
  pinMode(nSSELPin, OUTPUT);

  /* Configure SPI */
  spi = _spi;

  if (spi == NULL)
  {
    _LOG("Spi is NULL (CBERGCloudArduino::begin)\r\n");
    return;
  }

  spi->begin();
  spi->setBitOrder(MSBFIRST);
  spi->setDataMode(SPI_MODE0);
  spi->setClockDivider(SPI_CLOCK_DIV4);
}

void BERGCloudArduino::end()
{
  /* Deconfigure SPI */
  if (spi != NULL)
  {
    spi->end();
  }

  /* Deconfigure nSSEL control pin */
  pinMode(nSSELPin, INPUT_PULLUP);

  /* Call base class method */
  BERGCloudBase::end();
}

bool BERGCloudArduino::display(String& s)
{
  uint8_t text[BC_PRINT_MAX_CHARS + 1]; /* +1 for null terminator */

  /* Convert to C string */
  s.getBytes(text, sizeof(text));

  return display((const char *)text);
}

uint16_t BERGCloudArduino::getHostType(void)
{
  return BC_HOST_ARDUINO;
}

#ifdef BERGCLOUD_PACK_UNPACK

bool BERGCloudMessage::pack(double& n)
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

bool BERGCloudMessage::pack(String& s)
{
  uint16_t strLen = s.length();
  uint32_t i = 0;

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

bool BERGCloudMessage::unpack(String& s)
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

bool BERGCloudMessage::pack_boolean(boolean n)
{
  return pack((bool)n);
}

bool BERGCloudMessage::unpack_boolean(boolean &n)
{
  bool a;
  if (!unpack(a))
  {
    return false;
  }

  n = (boolean)a;
  return true;
}

bool BERGCloudArduino::pollForCommand(BERGCloudMessageBuffer& buffer, String &commandName)
{
  bool result = false;
  char tmp[31 + 1]; /* +1 for null terminator */

  commandName = ""; /* Empty string */
  result = pollForCommand(buffer, tmp, sizeof(tmp));

  if (result)
  {
    commandName = String(tmp);
  }

  return result;
}

bool BERGCloudArduino::sendEvent(String& eventName, BERGCloudMessageBuffer& buffer)
{
  uint8_t temp[eventName.length() + 1]; /* +1 for null terminator */
  eventName.getBytes(temp, sizeof(temp));
  return sendEvent((const char *)temp, buffer);
}

#endif // #ifdef BERGCLOUD_PACK_UNPACK

#endif // #ifdef ARDUINO
