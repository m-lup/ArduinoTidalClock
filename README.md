### ArduinoTidalClock
Tide Clock by Mariya Lupandina 25.04.21

Methods for telling time are constructed to serve specific purposes. 
The common 12-hour clock was implemented to standardize time, maximizing efficiency and profit 
in the transportation and manufacturing sectors. The Tidal Clock proposes structuring our days around 
the asynchronous ebb and flow of the tides to step outside the optimization mindset 
and begin building a closer awareness of the environmental systems that we live within.

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
