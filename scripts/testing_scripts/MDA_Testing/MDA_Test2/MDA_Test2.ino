///////////////////////////////////////////////////////////////////////////////////////////
/* Test8.ino --> InLab_Test6.ino

  Composite Sketch + Helper Files 
  Board: M0 48MHz & SD card

  Created: 9/19/23
  Michelle Pichardo
  David Smith

  Updates:
  - Debug.h now has timer prints
    - Prints time to store data in a buffer and write to file
    - Prints time to create all the files
  - Information prints are added to help the user navigate the program
  - Precision timer changed slightly 

*/
//////////////////////////////////////////////////////////////////////////////////////////

// Header Files: Contains namespaces 
#include "SPI.h"
#include "SD.h"
#include "HelperFunc.h"
#include "Debug.h"
#include "Adafruit_TinyUSB.h"
// #include "Adafruit_BMP085"

// DO NOT CHANGE WITHIN THIS ###############################################################

/* Constants for communication */
const int baudRate = 115200;                // Speed of printing to serial monitor (9600,115200)

// Analog Pins 
#define ANALOG0 A0                          // Analog probe for this sketch
#define ANALOG1 A1                          // Analog probe for this sketch
#define REDLEDpin 13                        // Red

/* Declarations/classes specific to SD card */           
File dataFile;
   
// ##########################################################################################

// OPEN TO CHANGES ..........................................................................
#define RESET_PIN  A3                       // Used to trigger board Reset

/* Constants for Timing */
unsigned long startAnalogTimer = 0;               // Micros and Milis requires unsigned long
unsigned long endAnalogTimer = 0;
unsigned long startFileTimer = 0; 
unsigned long endFileTimer = 0; 

/* Send A0 Voltage to Serial Monitor: initial testing */
float VLo = 0.0;
float Vref = 3.29;                         // Provide highest/ref voltage of circuit [0-3.29]V

/* Create Files variables */
bool filePrint = true; 
unsigned long fileCounter = 1; 
volatile unsigned long timer2Counter;

// Main Program (Runs once) ------------------------------------------------------------------
void setup(){

  // Expose M0 as external USB and set up serial monitor communication
  USB_SPI_initialization(baudRate);

  // Set up Reset pin 
  digitalWrite(RESET_PIN, HIGH);
  pinMode(RESET_PIN, OUTPUT);

  // Setup to create files: Set filePrint = true
  if (filePrint){
  // Set up SD card 
  SD_initialization(chipSelect);
  // Set the analog pin resolution 
  analogReadResolution(12);
  // send notice
  Serial.println("\t< Parameter Setup >");
  // Ask for the desired file (time) length  
  extractIntervalFromInput();
  // Ask to set Parameters
  extractParameters();
  // Ask for session value
  extractSessionNameFromInput();
  // Advise the user
  Serial.println("\n\n\tSESSION "+ String(session_val) + " STARTING\n");
  Serial.println("-> Creating { " + String(maxFiles) + " } files");
  Serial.println("-> Files store { " + String(desiredInterval_s) + "s } worth of data");
  Serial.println("- Red LED = LOW: LED will turn off during collection");
  Serial.println("- After logging the file(s), the board will reset");
  Serial.println("- Only after this reset will the files be visible on the disk\n");
  // Pause for a moment before collecting 
  delay(3000);

  startFileTimer = micros(); 
  }

}


void loop() {

  /* Serial Print test: debug_serialPrintA0()
    - This is a debug_ statement and is only active if 
      the debug file has APRINT set to 1 
    - Do not run this with while creating files 
    - Do run if you want a preliminary test of the board 
        - 1. Set filePrint = false on line 86
        - 2. Set APRINT to 1 in Debug.h
    - To zoom into a region change the values of Vref and Vlo
  */
  debug_serialPrintA0(Vref, VLo); 

  if (filePrint){

    // Set pin from users input
    uint8_t pin = getPin();
    // Turn on LED while writing
    digitalWrite(REDLEDpin, LOW);
    // Create File: MMDDXXXX.tmp 
    dataFile = open_SD_tmp_File_sessionFile(fileCounter, session_val);
    // Checks 
    debug("File Created: ");
    debugln(dataFile.name());
    // Header
    dataFile.println("File time length (us): " + String(desiredInterval_us));
    dataFile.println("Interaverage gap (us): " + String(interaverageDelay));
    dataFile.println("Intersample gap (us): " + String(intersampleDelay));
    dataFile.println("Samples averaged: " + String(numSamples));
    dataFile.println("Time just B4 collection (us)" + String(micros()));    

    // Store start Time
    startAnalogTimer = micros();

    // Gather data over a determined time interval 
    while (micros() - startAnalogTimer < desiredInterval_us){

      // Declare local variable/Buffer 
      unsigned long sum_sensorValue = 0; 

      // Build buffer: read sensor value then sum it to the previous sensor value 
      for (unsigned int counter = 1; counter <= numSamples; counter++){
        sum_sensorValue += analogRead(pin);
        // Pause for stability 
        myDelay_us(intersampleDelay);
      }

      // Write to file 
      dataFile.println(sum_sensorValue);

      // Pause for stability 
      myDelay_us(interaverageDelay);      

    }
    // log anaglog timer 
    dataFile.println("Time just after collection" + String(micros()));
    endAnalogTimer = micros() - startAnalogTimer;
    // Close the file 
    dataFile.close();

    // Send timer 
    timerPrintln("\nTime to create file { " + String(fileCounter) + " } using micros(): " + String(endAnalogTimer));
    timerPrintln("- This does not include file-Open, file-header, file-close");

    Serial.println("File "+ String(fileCounter) + "/" + String(maxFiles) +" complete...");

    // Incriment file counter 
    fileCounter++;

    // Condition when we've reached max files 
    if (fileCounter > maxFiles){

      // send timer 
      endFileTimer = micros() - startFileTimer; 
      timerPrintln("\n\nTime to complete { " + String(fileCounter -1) + " } files using micros(): " + String(endFileTimer));
      timerPrintln("- This does not include setup. This contains overhead from prints and timer calls"); 

      //Turn off LED 
      digitalWrite(REDLEDpin, HIGH);
      // Debug prints 
      Serial.println("MAX number of files (" + String(fileCounter-1) + ") created. Comencing RESET protocol.");
      debugf("File count: %i", fileCounter-1);
      
      // Change Condition 
      filePrint = false; 
      // Pause
      delay(3000);
      // Reset the board to view new files 
      digitalWrite(RESET_PIN, LOW);

      }
   
   }
 
}


