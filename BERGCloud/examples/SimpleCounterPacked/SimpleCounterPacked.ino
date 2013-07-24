/*
    SimpleCounterPacked - A simple demonstration of how you use the
                          BERG Cloud Arduino client libraries to
                          fetch and send automatically packed data
                          to BERG Cloud. For more info see
                          http://bergcloud.com/

    This example code is in the public domain.

    https://github.com/bergcloud/devboard-clientlib-arduino
*/

#include <BERGCloud.h>
#include <SPI.h>

// These values should be edited to reflect your Product setup on bergcloud.com

#define MY_PRODUCT_VERSION 0x00000001
const uint8_t MY_PRODUCT_ID[16] =  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

// Define your commands and events here, according to the schema from bergcloud.com

#define EXAMPLE_EVENT_ID 0x01

// DO NOT CHANGE DEFINES BELOW THIS LINE

#define nSSEL_PIN 10

int counter;
  
void setup()
{
  Serial.begin(115200);
  BERGCloud.begin(&SPI, nSSEL_PIN);
  Serial.println("--- reset ---");
  counter = 0;

  if (BERGCloud.joinNetwork(MY_PRODUCT_ID, MY_PRODUCT_VERSION))
  {
    Serial.println("Joined/Rejoined network");
  }
  else
  {
    Serial.println("joinNetwork() returned false.");
  }
}

void loop()
{
  uint8_t a;
  uint8_t eui64[BC_EUI64_SIZE_BYTES];
  char claimcode[BC_CLAIMCODE_SIZE_BYTES];
  uint32_t i;
  uint8_t commandID;
  int8_t rssi;
  uint8_t lqi;
  CMessage command, event;
  String text;
  unsigned int value;
  
  delay(1000);

  Serial.print("Poll for command... ");
  if (BERGCloud.pollForCommand(command, commandID))
  {
  
    Serial.print("Got command 0x");
    Serial.print(commandID, HEX);
    Serial.println(" containing:");

    command.print();

    if (command.unpack(value))
    {
      Serial.print("got value = ");
      Serial.println(value);
    }
    else
    {
      Serial.println("unpack(int) returned false.");
    }

    if (command.unpack(text))
    {
      Serial.print("got text = ");
      Serial.println(text);
    }
    else
    {
      Serial.println("unpack(text) returned false.");
    }
  }
  else
  {
    Serial.println("none.");
  }

  delay(1000);

  Serial.print("Sending an event... ");
  
  event.pack("BERG");
  event.pack(counter);
 
  counter++;

  if (BERGCloud.sendEvent(EXAMPLE_EVENT_ID, event))
  {
    Serial.println("ok");
  }
  else
  {
    Serial.println("failed/busy");
  }

  delay(1000);

  // The following method calls are examples of how you query the
  // network and BERG Cloud shield state from within your Arduino code
  
  if (BERGCloud.getNetworkState(a))
  {
    switch(a)
    {
    case BC_NETWORK_STATE_CONNECTED:
      Serial.println("Network State: Connected");
      break;
    case BC_NETWORK_STATE_CONNECTING:
      Serial.println("Network State: Connecting...");
      break;
    case BC_NETWORK_STATE_DISCONNECTED:
      Serial.println("Network State: Disconnected");
      break;
    default:
      Serial.println("Network State: Unknown!");
      break;
    }
  }
  else
  {
    Serial.println("getNetworkState() returned false.");
  }

  if (BERGCloud.getClaimingState(a))
  {
    switch(a)
    {
    case BC_CLAIM_STATE_CLAIMED:
      Serial.println("Claim State: Claimed");
      break;
    case BC_CLAIM_STATE_NOT_CLAIMED:
      Serial.println("Claim State: Not Claimed");
      break;
    default:
      Serial.println("Claim State: Unknown!");
      break;
    }
  }
  else
  {
    Serial.println("getClaimingState() returned false.");
  }

  if (BERGCloud.getClaimcode(claimcode))
  {
    Serial.print("Claim code: ");
    Serial.println(claimcode);
  }
  else
  {
    Serial.println("getClaimcode() returned false.");
  }

  if (BERGCloud.getEUI64(BC_EUI64_NODE, eui64))
  {
    Serial.print("Node EUI64: 0x");
    for (i=0; i < sizeof(eui64); i++)
    {
      if (eui64[7-i] < 0x10)
      {
        Serial.print("0");
      }
      Serial.print(eui64[7-i], HEX);
    }
    Serial.println("");
  }
  else
  {
    Serial.println("getEUI64(BC_EUI64_NODE) returned false.");
  }

  if (BERGCloud.getEUI64(BC_EUI64_PARENT, eui64))
  {
    Serial.print("Parent EUI64: 0x");
    for (i=0; i < sizeof(eui64); i++)
    {
      if (eui64[7-i] < 0x10)
      {
        Serial.print("0");
      }
      Serial.print(eui64[7-i], HEX);
    }
    Serial.println("");
  }
  else
  {
    Serial.println("getEUI64(BC_EUI64_PARENT) returned false.");
  }

  if (BERGCloud.getEUI64(BC_EUI64_COORDINATOR, eui64))
  {
    Serial.print("Coordinator EUI64: 0x");
    for (i=0; i < sizeof(eui64); i++)
    {
      if (eui64[7-i] < 0x10)
      {
        Serial.print("0");
      }
      Serial.print(eui64[7-i], HEX);
    }
    Serial.println("");
  }
  else
  {
    Serial.println("getEUI64(BC_EUI64_COORDINATOR) returned false.");
  }

  if (BERGCloud.getSignalQuality(rssi, lqi))
  {
    Serial.print("Last-hop signal quality: RSSI ");
    Serial.print(rssi, DEC);
    Serial.print(" dBm, LQI ");
    Serial.print(lqi, DEC);
    Serial.println("/255");
  }
  else
  {
    Serial.println("getSignalQuality returned false.");
  }
}

