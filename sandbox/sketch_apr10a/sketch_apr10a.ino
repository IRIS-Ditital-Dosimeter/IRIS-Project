/*
 * This program is a simple binary write/read benchmark.
 */
#include "FreeStack.h"
#include "SdFat.h"
#include "sdios.h"

// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3
/*
  Change the value of SD_CS_PIN if you are using SPI and
  your hardware does not use the default value, SS.
  Common values are:
  Arduino Ethernet shield: pin 4
  Sparkfun SD shield: pin 8
  Adafruit SD shields and modules: pin 10
*/
// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = 4;
#else   // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(12)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS

// Set PRE_ALLOCATE true to pre-allocate file clusters.
const bool PRE_ALLOCATE = true;

// Set SKIP_FIRST_LATENCY true if the first read/write to the SD can
// be avoid by writing a file header or reading the first record.
const bool SKIP_FIRST_LATENCY = true;

// Size of read/write.
const size_t BUF_SIZE = 512;

// File size in MB where MB = 1,000,000 bytes.
const uint32_t FILE_SIZE_MB = 5;

// Write pass count.
const uint8_t WRITE_COUNT = 2;

// Read pass count.
const uint8_t READ_COUNT = 2;
//==============================================================================
// End of configuration constants.
//------------------------------------------------------------------------------
// File size in bytes.
const uint32_t FILE_SIZE = 1000000UL * FILE_SIZE_MB;

// Insure 4-byte alignment.
uint32_t buf32[(BUF_SIZE + 3) / 4];
uint8_t* buf = (uint8_t*)buf32;

#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

// Serial output stream
ArduinoOutStream cout(Serial);
//------------------------------------------------------------------------------
// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------
void cidDmp() {
  cid_t cid;
  if (!sd.card()->readCID(&cid)) {
    error("readCID failed");
  }
  cout << F("\nManufacturer ID: ");
  cout << uppercase << showbase << hex << int(cid.mid) << dec << endl;
  cout << F("OEM ID: ") << cid.oid[0] << cid.oid[1] << endl;
  cout << F("Product: ");
  for (uint8_t i = 0; i < 5; i++) {
    cout << cid.pnm[i];
  }
  cout << F("\nRevision: ") << cid.prvN() << '.' << cid.prvM() << endl;
  cout << F("Serial number: ") << hex << cid.psn() << dec << endl;
  cout << F("Manufacturing date: ");
  cout << cid.mdtMonth() << '/' << cid.mdtYear() << endl;
  cout << endl;
}
//------------------------------------------------------------------------------
void clearSerialInput() {
  uint32_t m = micros();
  do {
    if (Serial.read() >= 0) {
      m = micros();
    }
  } while (micros() - m < 10000);
}
//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  // Wait for USB Serial
  while (!Serial) {
    yield();
  }
  delay(1000);
  // Set input pins
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  // Set the analog pin resolution 
  analogReadResolution(12);

  if (!ENABLE_DEDICATED_SPI) {
    cout << F(
        "\nSet ENABLE_DEDICATED_SPI nonzero in\n"
        "SdFatConfig.h for best SPI performance.\n");
  }

}
//------------------------------------------------------------------------------
void loop() {
  // float s;
  // uint32_t t;

  // Discard any input.
  clearSerialInput();

  // F() stores strings in flash to save RAM
  cout << F("Type any character to start\n");
  while (!Serial.available()) {
    yield();
  }

  if (!sd.begin(SD_CONFIG)) {
    sd.initErrorHalt(&Serial);
  }

  // open or create file - truncate existing file.
  if (!file.open("bench1.dat", O_CREAT | O_WRITE | O_APPEND)) {
    error("open failed");
  }


  // fill buf with known data
  if (BUF_SIZE > 1) {
    for (size_t i = 0; i < (BUF_SIZE - 4); i+=4) {
      // buf[i] = 'A' + (i % 26);
      buf[i] = micros();
      buf[i+1] = analogRead(A0);
      buf[i+2] = analogRead(A1);
      buf[i+3] = micros();
    }
    // buf[BUF_SIZE - 2] = '\r';
  }
  // buf[BUF_SIZE - 1] = '\n';

  cout << F("FILE_SIZE_MB = ") << FILE_SIZE_MB << endl;
  cout << F("BUF_SIZE = ") << BUF_SIZE << F(" bytes\n");
  cout << F("Starting write test, please wait.") << endl << endl;

  // // do write test
  uint32_t n = FILE_SIZE / BUF_SIZE;
  // cout << F("write speed and latency") << endl;
  // cout << F("speed,max,min,avg") << endl;
  // cout << F("KB/Sec,usec,usec,usec") << endl;
  // for (uint8_t nTest = 0; nTest < WRITE_COUNT; nTest++) {
  //   file.truncate(0);
  //   if (PRE_ALLOCATE) {
  //     if (!file.preAllocate(FILE_SIZE)) {
  //       error("preAllocate failed");
  //     }
  //   }
  //   maxLatency = 0;
  //   minLatency = 9999999;
  //   totalLatency = 0;
  //   skipLatency = SKIP_FIRST_LATENCY;
  //   t = millis();

    for (uint32_t i = 0; i < n; i++) {
      // uint32_t m = micros();
      if (file.write(buf, BUF_SIZE) != BUF_SIZE) {
        error("write failed");
      }
      // m = micros() - m;
      // totalLatency += m;
      // if (skipLatency) {
      //   // Wait until first write to SD, not just a copy to the cache.
      //   skipLatency = file.curPosition() < 512;
      // } else {
      //   if (maxLatency < m) {
      //     maxLatency = m;
      //   }
      //   if (minLatency > m) {
      //     minLatency = m;
      //   }
      // }
    }
    // t = millis() - t;

  cout << F("done ") << endl;

  file.close();
  sd.end();
}
