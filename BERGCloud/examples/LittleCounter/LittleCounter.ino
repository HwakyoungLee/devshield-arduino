/*
    LittleCounter - A simple demonstration of how you use the BERG Cloud Arduino
                    client libraries to fetch and send data to BERG Cloud. For
                    more info see http://bergcloud.com/

    This example code is in the public domain.

    https://github.com/bergcloud/devshield-arduino
*/

#include <BERGCloud.h>
#include <SPI.h>

#define nSSEL_PIN 10 // SPI Slave select definition -
//You should not need to change nSSEL_PIN unless you are using a Mega or Leonardo

// The Project Key ties this code into a Project on developer.bergcloud.com
const byte PROJECT_KEY[BC_KEY_SIZE_BYTES] = \
    {0x8B,0x05,0xF7,0x25,0x10,0x54,0x0A,0xE4,0x7C,0x35,0xEE,0xE7,0x26,0xDC,0xD5,0xA8};

// The version of your code
#define VERSION 1

// Define your commands and events here, according to the schema from bergcloud.com

#define COMMAND_SET_COUNTER 1
#define COMMAND_DISPLAY_TEXT 2
#define EVENT_COUNTER_CHANGED 1

// The counter we will increment and send up to the cloud
unsigned int counter;

void setup()
{
  Serial.begin(115200);
  BERGCloud.begin(&SPI, nSSEL_PIN);
  Serial.println("--- Arduino reset ---");

  counter = 0; // Initialise our counter

  // Attempt to connect with our project key and build version
  if (BERGCloud.connect(PROJECT_KEY, VERSION)) {
    Serial.println("Connected to network");
  } else {
    Serial.println("BERGCloud.connect() returned false.");
  }
  
}

void loop()
{ 
  // The ID of the command we've received. 
  byte commandID;
  
  // The command and event objects we use below    
  BERGCloudMessage command, event;

  // Some simple string manipulation

  ///////////////////////////////////////////////////////////////
  // Fetching commands                                         //
  ///////////////////////////////////////////////////////////////
  
  Serial.print("Poll for command... ");
  if (BERGCloud.pollForCommand(command, commandID)) {

    // Print the 
    Serial.print("got command with ID: ");
    Serial.println(commandID, DEC);
    
    // Here we can map the command IDs to method calls within our code
    switch (commandID) {
      case COMMAND_SET_COUNTER:
        // Command one contains an integer
        handleSetCounter(command);
        break;
      case COMMAND_DISPLAY_TEXT:
        // Command two is a simple message containing a string
        handleDisplayText(command);
        break;
      // Default case, we don't have a match for the Command ID
      default:
        Serial.print("WARNING: Unknown command");   
    }
  } else {
    // No command!
    Serial.println("none.");
  }
  
  ///////////////////////////////////////////////////////////////
  // Sending events                                            //
  ///////////////////////////////////////////////////////////////

  Serial.print("Sending an event... ");

  // In this Little Counter example we send up a string and a counter
  //
  // Packing is very straight forward. Just define your
  // BERGCloudMessage object and call pack() passing in each type
  // you wish to encode.
  
  // There is currently a 64 byte limit on the size of events, so be
  // careful with how much data you're sending up!
  
  event.pack("BERG");  // Pack a string
  event.pack(counter); // Pack an unsigned int32
  
  // Send the event object
  if (BERGCloud.sendEvent(EVENT_COUNTER_CHANGED, event))  {
    Serial.println("ok");
  } else {
    Serial.println("failed/busy");
  }
  
  counter++; // Increment our counter each time we loop around

  // A simple delay to rate limit what we send up to the cloud
  delay(5000);
}

void handleSetCounter(BERGCloudMessage &command) {
  String prefixText, counterText;
  int newCounterVal;
  
  prefixText = "Counter set to";

  // We're expecting an integer value for the counter
  if (command.unpack(newCounterVal)) {
    
    Serial.print("Decoded newCounterVal as: ");
    Serial.println(newCounterVal, DEC);
    
    // Set the global to our new value
    counter = newCounterVal;
    
    counterText = String(newCounterVal);

    // Show the string on the OLED screen with display()
    BERGCloud.clearDisplay();
    
    // Print in reverse order
    BERGCloud.display(prefixText);
    BERGCloud.display(counterText);
  } else {
    Serial.println("WARNING: unpacking the new counter value failed");
  }
}

void handleDisplayText(BERGCloudMessage &command) {
  String prefixText, suffixText, finalText;
  int number;
  
  prefixText = "Hello, ";

  // We're expecting a string and a number for display-text, so
  // we attempt to decode these types in turn

  if (command.unpack(suffixText)) {
    Serial.print("Decoded text: ");
    Serial.println(suffixText);

    // Concatenate our strings
    finalText = prefixText + suffixText;

    // Show the string on the OLED screen with display()
    BERGCloud.clearDisplay();
    BERGCloud.display(finalText);
  } else {
    Serial.println("WARNING: unpacking text failed");
  }
  
  // For this command, we can optionally be passed an integer
  // and we log this out on the serial console.
  // 
  // This is to demonstrate serializing multiple values within
  // one command.

  if (command.unpack(number)) {
    Serial.print("Decoded number: ");
    Serial.println(number);
  } else {
    Serial.println("No additional number given");
  }
}
