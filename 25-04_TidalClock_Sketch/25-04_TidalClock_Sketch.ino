/*  Tidal Clock by Mariya Lupandina 25.04.21

The following code allows you to program an Arduino nano MCU to calculate the tide predictions for one tidal cycle
with the help of an RTC and then radially display the position of the high and low tide times relative to a 24 hr clock. 
The tide predictions are automatically updated once the tidal cycle is completeThe display is also code to show an 
undulating circle at its center that grows as the tide comes in and shrinks as the tide goes out. The MCU also tells a stepper motor to 
complete one revolution per tidal cycling, rotate at the pace of the cycle.

For this project you need:
- Arduino nano MCU
- 128 x 128 OLED Display
- RTC
- Stepper motor & motor module
- a power supply

Tide prediction code based of:
Tide_calculator.ino by Luke Miller, copyright (c) 2019
This code calculates the current tide height for the pre-programmed site. 
It requires a real time clock (DS1307 or DS3231 chips) to generate a time for the calculation.
The site is set by the name of the included library, for other sites use this link: // Other sites available at http://github.com/millerlp/Tide_calculator.
Written under version 1.6.4 of the Arduino IDE.

 The harmonic constituents used here were originally derived from 
 the Center for Operational Oceanic Products and Services (CO-OPS),
 National Ocean Service (NOS), National Oceanic and Atmospheric 
 Administration, U.S.A.

The display code based of: 
Arduino UNO and 128x128 OLED Display for analog clock by upir, 2023

Tide prediction code based of:
Tide_calculator.ino by Luke Miller, copyright (c) 2019
This code calculates the current tide height for the pre-programmed site. 
It requires a real time clock (DS1307 or DS3231 chips) to generate a time for the calculation.
The site is set by the name of the included library, for other sites use this link: // Other sites available at http://github.com/millerlp/Tide_calculator.
Written under version 1.6.4 of the Arduino IDE.

 The harmonic constituents used here were originally derived from 
 the Center for Operational Oceanic Products and Services (CO-OPS),
 National Ocean Service (NOS), National Oceanic and Atmospheric 
 Administration, U.S.A.

The display code based of: 
Arduino UNO and 128x128 OLED Display for analog clock by upir, 2023

*/ 
//Initial setup
//Header files for talking to real time clock
#include <Wire.h> // Required for RTClib
#include <SPI.h> // Required for RTClib to compile properly
#include <RTClib.h> // From https://github.com/millerlp/RTClib
//Header file for tide prediction at given site
#include "TidelibTheBatteryNewYorkHarborNewYork.h"
//Header file for talking with OLED display
#include <Arduino.h>
#include <U8g2lib.h> // u8g2 library for drawing on OLED display - needs to be installed in Arduino IDE first
#include <Stepper.h> // motor library

// ---------------------- Display & Tide Calc Setup ----------------------
//U8G2_SH1107_128X128_1_HW_I2C u8g2(U8G2_R0);  // For 128x128 SH1107
#define CS 10
#define DC 9
#define RST 8
U8G2_SH1107_128X128_1_4W_HW_SPI u8g2(U8G2_R2, 10, 9, 8); //[page buffer, size = 128 bytes]
//RTC_DS1307 RTC;  // Uncomment when using this chip
RTC_DS3231 RTC; // Uncomment when using this chip
TideCalc myTideCalc;

const int center_x = 64;
const int center_y = 64;
const int min_radius = 4;
const int max_radius = 31;
int current_radius = min_radius;
bool growing = true;

// ---------------------- Stepper Setup ----------------------
const int stepsPerRevolution = 2048;
const float stepsPerMinute = 2048.0 / (24.0 * 60.0);  // Steps per minute

const int in1Pin = 2;
const int in2Pin = 4;
const int in3Pin = 3;
const int in4Pin = 5;

Stepper stepper(stepsPerRevolution, in1Pin, in3Pin, in2Pin, in4Pin);
int currentStepperPos = 0;
int lastMinuteChecked = -1;
int lastPrintedMinute = -5; 

// ---------------------- Pulse Animation State ----------------------
unsigned long lastUpdate = 0;     // Time of last animation update
const unsigned long animationInterval = 1000; // Update every 1 second

// ---------------------- Tide Event Containers ----------------------

DateTime tideTimes[4];  // Stores up to 4 tide times (2 high + 2 low)
int tideType[4];        // 1 = High tide, 0 = Low tide
DateTime lastPredictionTime;
int currDay;  


/// ---------------------- Setup ----------------------
void setup() {
  Wire.begin();           // Start I2C communication
  RTC.begin();            // Start RTC
  u8g2.begin();           // Start display
  Serial.begin(57600);    // Initialize serial monitor for debugging
  u8g2.setContrast(255);  // Set OLED brightness

  stepper.setSpeed(10);  // Required even if we use .step()
  //Serial.begin(9600);
  Serial.println("Stepper initialized...");

  DateTime now = RTC.now();
  currDay = now.day();
  lastPredictionTime = now;

  Serial.println("Starting TideClock");
  Serial.print("Station: ");
  Serial.print(myTideCalc.returnStationID());
  Serial.print(" (ID ");
  Serial.print(myTideCalc.returnStationIDnumber());
  Serial.println(")");

  DateTime startOfDay(now.year(), now.month(), now.day(), 0, 0, 0);
  updateTidePredictions(startOfDay);
  resetStepperToZero();      // Always start at step 0
  updateStepperToTime(now);  // Rotate to current position

}

// ---------------------- Loop ----------------------
void loop() {
DateTime now = RTC.now();

  // Update tide predictions at start of a new day
  if (now.day() != currDay) {
    currDay = now.day();
    updateTidePredictions(now);
  }
  // Update stepper position once per minute
  if (now.minute() != lastPrintedMinute) {
    updateStepperToTime(now);   // ← This keeps the motor synced to time
    lastPrintedMinute = now.minute();
  }
  
  // Update the pulse animation based on tide phase
  updatePulse(now);

  // Render display
  drawDisplay(now);
}

// ---------------------- Stepper Update ----------------------
void resetStepperToZero() {
  // Move one full revolution backward — adjust if needed
  stepper.step(-stepsPerRevolution);  
  currentStepperPos = 0;

  Serial.println("Stepper reset to 0 position.");
}

void updateStepperToTime(DateTime now) {
  int minutesSinceMidnight = now.hour() * 60 + now.minute();
  int expectedStep = (int)(minutesSinceMidnight * stepsPerMinute) % stepsPerRevolution;

  int stepDelta = expectedStep - currentStepperPos;

  // Handle wrap-around direction efficiently
  if (stepDelta < -stepsPerRevolution / 2) {
    stepDelta += stepsPerRevolution;
  } else if (stepDelta > stepsPerRevolution / 2) {
    stepDelta -= stepsPerRevolution;
  }

  // Move the motor
  stepper.step(stepDelta);
  currentStepperPos = expectedStep;

  // Print only once every 5 minutes
  if (now.minute() % 5 == 0 && now.minute() != lastPrintedMinute) {
    lastPrintedMinute = now.minute();

    Serial.print("Time: ");
    if (now.hour() < 10) Serial.print("0");
    Serial.print(now.hour());
    Serial.print(":");
    if (now.minute() < 10) Serial.print("0");
    Serial.print(now.minute());

    Serial.print(" | Stepper Position: ");
    Serial.println(currentStepperPos);
  }
}

  // 1. Tide Prediction Function — Called Once Per Day
void updateTidePredictions(DateTime now) {
  Serial.println("\n==========================");
  Serial.print("Generating tide predictions starting from ");
  Serial.print(now.month()); Serial.print("/");
  Serial.print(now.day()); Serial.print(" ");
  Serial.print(now.hour()); Serial.print(":");
  Serial.print(now.minute());
  Serial.println();
  Serial.println("==========================");

  DateTime startTime = now - TimeSpan(0, 2, 0, 0); // Start 2 hours before current time
  DateTime endTime = now + TimeSpan(1, 6, 0, 0);   // Search for up to 30 hours ahead to catch full cycles

  const int interval = 2; // Sample every 2 minutes
  DateTime prevTime = startTime;
  DateTime currTime = startTime + TimeSpan(0, 0, interval, 0);

  float prevTide = myTideCalc.currentTide(prevTime);
  float currTide = myTideCalc.currentTide(currTime);

  int count = 0;
  while (currTime <= endTime && count < 4) {
    DateTime nextTime = currTime + TimeSpan(0, 0, interval, 0);
    float nextTide = myTideCalc.currentTide(nextTime);

    // Detect high tide: rising to falling
    if (prevTide < currTide && currTide > nextTide) {
      tideTimes[count] = currTime;
      tideType[count] = 1; // High tide
      Serial.print("High Tide at ");
      printTime(currTime);
      count++;
    }

    // Detect low tide: falling to rising
    if (prevTide > currTide && currTide < nextTide) {
      tideTimes[count] = currTime;
      tideType[count] = 0; // Low tide
      Serial.print("Low Tide  at ");
      printTime(currTime);
      count++;
    }

    prevTime = currTime;
    prevTide = currTide;
    currTime = nextTime;
    currTide = nextTide;
  }

  if (count < 4) {
    Serial.println("⚠️ Warning: Less than 4 tide events found.");
  }
  Serial.println("==========================\n");
}

// 2. Pulse Animation — Grows/Shrinks Toward Tide Events
void updatePulse(DateTime now) {
  if (millis() - lastUpdate < animationInterval) return;
  lastUpdate = millis();

  // Find the current interval (between two known tide events)
  DateTime previous, next;
  bool growing = true;

  for (int i = 0; i < 4; i++) {
    if (tideTimes[i] > now) {
      next = tideTimes[i];
      previous = tideTimes[(i - 1 + 4) % 4]; // Wrap around
      growing = (tideType[i] == 1); // If next is high tide, pulse is growing
      break;
    }
  }

  float totalTime = (next - previous).totalseconds();
  float elapsedTime = (now - previous).totalseconds();
  float phase = constrain(elapsedTime / totalTime, 0, 1); // Normalize

  // Linearly interpolate pulse radius
  if (growing) {
    current_radius = min_radius + (max_radius - min_radius) * phase;
  } else {
    current_radius = max_radius - (max_radius - min_radius) * phase;
  }
}

// 3. Drawing Function — Display Background, Pulse, Ticks
void drawDisplay(DateTime now) {
  u8g2.firstPage();
  do {
    draw_background();
    draw_tides(current_radius);
    draw_ticks();
  } while (u8g2.nextPage());
}

// Draw outer circle and center point
void draw_background() {
  u8g2.setDrawColor(1);
u8g2.drawCircle(center_x, center_y, 58, U8G2_DRAW_ALL);
  u8g2.drawPixel(center_x, center_y); // Center mark
}

// Draw the pulse disc that grows/shrinks
void draw_tides(int radius) {
  u8g2.setDrawColor(1);
  u8g2.drawDisc(center_x, center_y, radius, U8G2_DRAW_ALL);
}

// Draw tick marks for tide events
void draw_ticks() {
  for (int i = 0; i < 4; i++) {
    float hour = tideTimes[i].hour() + tideTimes[i].minute() / 60.0;
    float angle_deg = (hour / 24.0) * 360.0;
    float angle_rad = angle_deg * PI / 180.0;

    // Base radius of the circle
    float base_radius = 58;
    float tick_length = 10;

    float r1, r2;

    if (tideType[i] == 1) {
      // High tide: tick goes outward
      r1 = base_radius;
      r2 = base_radius + tick_length;
    } else {
      // Low tide: tick goes inward
      r1 = base_radius - tick_length;
      r2 = base_radius;
    }

    float x1 = center_x + sin(angle_rad) * r1;
    float y1 = center_y - cos(angle_rad) * r1;
    float x2 = center_x + sin(angle_rad) * r2;
    float y2 = center_y - cos(angle_rad) * r2;

    u8g2.drawLine(x1, y1, x2, y2);
  }
}

// 4. Serial Print Function
void printTime(DateTime now) {
  if (now.hour() < 10) Serial.print("0");
  Serial.print(now.hour()); Serial.print(":");

  if (now.minute() < 10) Serial.print("0");
  Serial.print(now.minute()); Serial.print(":");

  if (now.second() < 10) Serial.print("0");
  Serial.println(now.second());
}
