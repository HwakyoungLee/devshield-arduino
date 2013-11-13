/*
    HelloWorld - A simple demonstration of how you use the BERG Cloud Arduino
                 client libraries to fetch and send data to BERG Cloud. For
                 more info see http://bergcloud.com/

    This example code is in the public domain.

    https://github.com/bergcloud/devshield-arduino
*/

#include <BERGCloud.h>
#include <SPI.h>
#define nSSEL_PIN 10 // SPI Slave select definition - DO NOT EDIT


// These values should be edited to reflect your Product setup on developer.bergcloud.com

// The Project Key as specified in the Project developer page on bergcloud.com 
const uint8_t PROJECT_KEY[BC_PRODUCT_KEY_SIZE_BYTES] =  \
    {0xE4,0xBF,0xEE,0xD0,0x4E,0x25,0x43,0x9A,0x63,0x7F,0x32,0x15,0x95,0x08,0x4B,0x5E};

// The version of your code
#define BUILD_VERSION 0x0001

// Define your commands and events here, according to the schema from bergcloud.com

#define COMMAND_HELLO_WORLD_DISPLAY_TEST 0x01
#define EVENT_HELLO_WORLD_COUNTER 0x01


// Our counter we will increment and send up to the cloud
uint32_t counter;

void setup()
{
  Serial.begin(115200);
  BERGCloud.begin(&SPI, nSSEL_PIN);
  Serial.println("--- Arduino reset ---");

  counter = 0; // Initialise our counter

  // Attempt to connect with our project key and build version
  if (BERGCloud.connect(PROJECT_KEY, BUILD_VERSION)) {
    Serial.println("Connected to network");
  } else {
    Serial.println("BERGCloud.connect() returned false.");
  }
}

void loop()
{ 
  // The ID of the command we've received. 
  uint8_t commandID;
  
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
      case COMMAND_DISPLAY_TEXT:
        // Command one is a simple message containing a string and an integer
        handleDisplayText(command);
        break;
        
      // Default case, we don't have a match for the Command ID
      default:
        print("WARNING: Unknown command ID");   
    }
  } else {
    // No command!
    Serial.println("none.");
  }

  delay(1000);
  
  ///////////////////////////////////////////////////////////////
  // Sending events                                          //
  ///////////////////////////////////////////////////////////////

  Serial.print("Sending an event... ");

  // In this HelloWorld example we send up a string and a counter
  //
  // Packing is very straight forward. Just define your
  // BERGCloudMessage object and call pack() passing in each type
  // you wish to encode.
  
  event.pack("BERG");  // Pack a string
  event.pack(counter); // Pack an unsigned int32
  
  counter++; // Increment our counter each time we loop around

  // Send the event object
  if (BERGCloud.sendEvent(EVENT_HELLO_WORLD_COUNTER, event))  {
    Serial.println("ok");
  } else {
    Serial.println("failed/busy");
  }

  // A simple delay to rate limit what we send up to the cloud
  delay(1000);
}


void handleDisplayText(command) {
  String prefixText, suffixText, finalText;
  int32_t number;
  
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
    BERGCloud.display(finalText.c_str());
  } else {
    Serial.println("WARNING: unpacking text failed");
  }

  if (command.unpack(number)) {
    Serial.print("Decoded number: ");
    Serial.println(number);
  } else {
    Serial.println("WARNING: unpacking number failed");
  }
}

