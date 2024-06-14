#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "definitions.h"

// NeoPixel Library include and define statements
#define PIN 2           // Pin connected to the NeoPixels
#define NUMPIXELS 100   // Number of NeoPixels
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500    // Time (in milliseconds) to pause between pixels

SoftwareSerial mySerial(10, 11); // RX, TX

// Structure to define a building with its number of pixels and corresponding pins
typedef struct __attribute((__packed__)) {
  uint8_t numberofPixels;
  uint8_t pins[6];
} ledBuildingType_t;

// Color map for different priority levels
uint32_t colorMap[5] = {0, 0xFF0000, 0xFFA500, 0x0000FF, 0x00FF00};

// Definitions of buildings with their respective pins
ledBuildingType_t Building_1 = {6, {0, 1, 10, 11, 20, 21}};
ledBuildingType_t Building_2 = {6, {3, 4, 13, 11, 20, 21}};
ledBuildingType_t Building_3 = {6, {8, 9, 18, 19, 28, 29}};
ledBuildingType_t Building_4 = {6, {70, 71, 80, 81, 90, 91}};
ledBuildingType_t Building_5 = {6, {73, 74, 83, 81, 90, 91}};
ledBuildingType_t Building_6 = {6, {78, 79, 88, 89, 98, 99}};

// Definitions of individual LEDs
ledBuildingType_t L_1 = {1, {30}};
ledBuildingType_t L_2 = {1, {32}};
ledBuildingType_t L_3 = {1, {34}};
ledBuildingType_t L_4 = {1, {36}};
ledBuildingType_t L_5 = {1, {38}};
ledBuildingType_t L_6 = {1, {60}};
ledBuildingType_t L_7 = {1, {62}};
ledBuildingType_t L_8 = {1, {64}};
ledBuildingType_t L_9 = {1, {66}};
ledBuildingType_t L_10 = {1, {68}};

// Array of building LED pointers
ledBuildingType_t* bLEDs[16] = {&Building_1, &Building_2, &Building_3, &Building_4, &Building_5, &Building_6,
                                &L_1, &L_2, &L_3, &L_4, &L_5, &L_6, &L_7, &L_8, &L_9, &L_10};

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("\n\nCapstone Arduino\n\n");

  // Initialize software serial communication
  mySerial.begin(9600);
  
  // Initialize the NeoPixel library
  pixels.begin();
}

void loop() {
  // Continuously read building information
  readBuildingInfo();
}

void readBuildingInfo() {
  // Check if enough data is available
  if (mySerial.available() < 4) return;

  // Read the header markers
  uint8_t m1 = mySerial.read();
  uint8_t m2 = mySerial.peek();
  if (!(m1 == 0xa5 && m2 == 0x5a)) return;

  m2 = mySerial.read();
  Serial.println("Received header markers");

  // Read the length bytes
  uint8_t l1 = mySerial.read();
  uint8_t l2 = mySerial.read();
  if ((uint8_t)(l1 & l2) != 0) return;

  Serial.print("Number of buildings: "); Serial.println(l1);

  // Calculate CRC
  uint8_t crc = 0b10101010;
  crc ^= l1;
  crc ^= l2;

  // Allocate memory for building status records
  BuildingStatusRecord_t* buildings;
  buildings = (BuildingStatusRecord_t*) malloc(l1 * sizeof(BuildingStatusRecord_t));
  if (buildings == NULL) {
    Serial.println("ERROR: OOM");
    return;
  }

  // Read building status records
  uint8_t* p = (uint8_t*) buildings;
  for (int i = 0; i < l1 * sizeof(BuildingStatusRecord_t); i++) {
    uint32_t s = millis();
    while (!mySerial.available()) {
      if (millis() - s > 5000) {
        Serial.println("Timeout waiting for serial data from esp32");
        free(buildings);
        return;
      }
    }
    *p = mySerial.read();
    crc ^= *p++;
  }

  // Read the received CRC
  uint32_t s = millis();
  while (!mySerial.available()) {
    if (millis() - s > 5000) {
      Serial.println("Timeout waiting for serial data from esp32");
      free(buildings);
      return;
    }
  }
  uint8_t recCRC = mySerial.read();
  Serial.print("Received   CRC= "); Serial.println(recCRC, HEX);
  Serial.print("Calculated CRC= "); Serial.println(crc, HEX);

  // Check CRC
  if (recCRC != crc) {
    Serial.println("ERROR: CRC checksums do not match");
  }

  // Update NeoPixels based on building statuses
  for (int i = 0; i < l1; i++) {
    ledBuildingType_t& currentBuilding = *bLEDs[i];

    for (int j = 0; j < currentBuilding.numberofPixels; j++) {
      if (buildings[i].effectivePriority < 5) { // Ensure the priority is within bounds
        pixels.setPixelColor(currentBuilding.pins[j], colorMap[buildings[i].effectivePriority]);
      }
    }

    // Print building status to serial
    Serial.print("Building id       = "); Serial.println(buildings[i].id);
    Serial.print("\t desired prio   = "); Serial.println(buildings[i].desiredPriority);
    Serial.print("\t effective prio = "); Serial.println(buildings[i].effectivePriority); Serial.println();

    Serial.print("\t Grid.id       = "); Serial.println(buildings[i].grid.id);
    Serial.print("\t Grid.status   = "); Serial.println(buildings[i].grid.status);
    Serial.print("\t Grid.capacity = "); Serial.println(buildings[i].grid.capacity);
    Serial.print("\t Grid.output   = "); Serial.println(buildings[i].grid.output); Serial.println();

    Serial.print("\t Solar.id       = "); Serial.println(buildings[i].solar.id);
    Serial.print("\t Solar.status   = "); Serial.println(buildings[i].solar.status);
    Serial.print("\t Solar.capacity = "); Serial.println(buildings[i].solar.capacity);
    Serial.print("\t Solar.output   = "); Serial.println(buildings[i].solar.output); Serial.println();

    Serial.print("\t Battery.id       = "); Serial.println(buildings[i].battery.id);
    Serial.print("\t Battery.status   = "); Serial.println(buildings[i].battery.status);
    Serial.print("\t Battery.capacity = "); Serial.println(buildings[i].battery.capacity);
    Serial.print("\t Battery.output   = "); Serial.println(buildings[i].battery.output); Serial.println();

    Serial.print("\t Generator.id       = "); Serial.println(buildings[i].generator.id);
    Serial.print("\t Generator.status   = "); Serial.println(buildings[i].generator.status);
    Serial.print("\t Generator.capacity = "); Serial.println(buildings[i].generator.capacity);
    Serial.print("\t Generator.output   = "); Serial.println(buildings[i].generator.output); Serial.println();
  }

  // Show the updated pixel colors
  pixels.show();
  
  // Free the allocated memory
  free(buildings);
}
