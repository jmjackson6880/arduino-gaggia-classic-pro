#include <Arduino.h>
#include "Adafruit_LiquidCrystal.h"
#include <arduino-timer.h>

Adafruit_LiquidCrystal lcd(0);
int32_t charid_string;

/*  Project To-Dos
---
  - Voltage: read and log output
  - Voltage: output varied voltage
  - Variable voltage graphs
  - Graph outputs based on voltage
  - Internet-connected Arduino (OTA updates)
*/

/*
 # LCD screen
 # Displays data/modes to LCD screen.

 Hardware:
 - Arduino Uno R3, 5v
 - Adafruit Standard LCD 16x2 (P/N 181)
 - Adafruit i2c / SPI character LCD backpack (P/N 292)

 LCD Backpack Circuit:
 - 5V to 5V
 - GND to GND
 - CLK to A5
 - DAT to A4
*/

#define DEVICE_NAME "Gaggia Classic Pro"
#define DEVICE_BUILD_VERSION "0.5"
#define DEVICE_ID "PEIWSOTFqa8AOeq"
#define BREW_MIN_TIME 5
#define BREW_MAX_TIME 90
#define BUTTON_STOP_PIN 3
#define BUTTON_BREW_PIN 2
#define SENSOR_BREW_TIME_PIN A2
#define SENSOR_PROFILES_PIN A1
#define STATUS_LED_R_PIN 9
#define STATUS_LED_G_PIN 10
#define STATUS_LED_B_PIN 11

enum STATUS { STARTING_UP, DISABLED, WAIT, READY, IN_USE };
enum PROFILES { OFF, PROFILE_A, PROFILE_B, PROFILE_C, PROFILE_D };
enum LED_COLORS { OUT, GREEN, RED, BLUE, WHITE };

enum STATUS CURRENT_STATUS;
enum PROFILES CURRENT_PROFILE;
enum LED_COLORS CURRENT_LED_COLOR;

int CURRENT_BREW_TIME = 0;
int BREW_TIME = 0;
int BREW_BUTTON_STATE = 0;
int STOP_BUTTON_STATE = 0;
int BREW_TIME_SENSOR_VALUE = 0;
int PROFILES_SENSOR_VALUE = 0;

auto STOP_BREW_TIMER = timer_create_default();
auto BREW_TIMER = timer_create_default();
auto START_UP_TIMER = timer_create_default();

unsigned long BREW_START_TIMESTAMP;
unsigned long BREW_CURRENT_TIMESTAMP;

String CURRENT_PROFILE_DISPLAY = "";
String CURRENT_STATUS_DISPLAY = "";

void setup(void) {
  Serial.begin(9600);

  CURRENT_LED_COLOR = WHITE;
  lcd.begin(16, 2);
  lcd.clear();
  setStatusToStartingUp();
}

void loop(void) {
  BREW_BUTTON_STATE = digitalRead(BUTTON_BREW_PIN);
  STOP_BUTTON_STATE = digitalRead(BUTTON_STOP_PIN);

  boolean isBrewButtonPressed = BREW_BUTTON_STATE == HIGH;
  boolean isStopButtonPressed = STOP_BUTTON_STATE == HIGH;

  PROFILES_SENSOR_VALUE = analogRead(SENSOR_PROFILES_PIN);
  BREW_TIME_SENSOR_VALUE = analogRead(SENSOR_BREW_TIME_PIN);

  START_UP_TIMER.tick();
  BREW_TIMER.tick();
  STOP_BREW_TIMER.tick();
  
  switch (CURRENT_STATUS) {
    case STARTING_UP:
      setInitialScreen();
      START_UP_TIMER.in(2000, [](void *argument) -> bool { 
        lcd.clear();
        setStatusToReady();
        START_UP_TIMER.cancel();
      });
      break;
    case DISABLED:
      getBrewTimeSetting(BREW_TIME_SENSOR_VALUE);
      getProfileSetting(PROFILES_SENSOR_VALUE);
      break;
    case WAIT:
      /* Handle wait mode */
      break;
    case READY:
      getBrewTimeSetting(BREW_TIME_SENSOR_VALUE);
      getProfileSetting(PROFILES_SENSOR_VALUE); 
      if (isBrewButtonPressed) {
        startBrew();
      }
      if (isStopButtonPressed) {
        clearLastBrewTime();
      }
      break;
    case IN_USE:
      if (isStopButtonPressed) {
        stopBrew();
      }
      break;
  }

  setIndicatorLED();
  
  if (CURRENT_STATUS != STARTING_UP) {
    updateLcdDisplay();
  }
}

void setInitialScreen() {
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  lcd.setCursor(0, 1);
  lcd.print("Build v");
  lcd.setCursor(7, 1);
  lcd.print(DEVICE_BUILD_VERSION);
}

void updateLcdDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.setCursor(6, 0);
  lcd.print((CURRENT_BREW_TIME < 10) ? '0' + String(CURRENT_BREW_TIME) : String(CURRENT_BREW_TIME));
  lcd.setCursor(10, 0);
  lcd.print(CURRENT_STATUS_DISPLAY);
  lcd.setCursor(0, 1);
  lcd.print(CURRENT_PROFILE_DISPLAY);
  lcd.setCursor(10, 1);
  lcd.print((BREW_TIME < 10) ? '0' + String(BREW_TIME) : String(BREW_TIME));
  lcd.setCursor(13, 1);
  lcd.print("sec");
}

void getProfileSetting(int value) {
  int potPercentage = (float)value/1023 * 100;
  
  if (potPercentage >= 80) {
    setProfileToD();
    setStatusToReady();
  } else if (potPercentage >= 60) {
    setProfileToC();
    setStatusToReady();
  } else if (potPercentage >= 40) {
    setProfileToB();
    setStatusToReady();
  } else if (potPercentage >= 20) {
    setProfileToA();
    setStatusToReady();
  } else {
    setProfileToOff();
    setStatusToDisabled();
  }
}

void getBrewTimeSetting(int value) {
  int valueToSet = (float)value/1023 * BREW_MAX_TIME;
  BREW_TIME = valueToSet >= BREW_MIN_TIME ? valueToSet : BREW_MIN_TIME;
}

void startBrew() {
  // Reset all timers + variables
  //
  BREW_TIMER.cancel();
  clearLastBrewTime();
  BREW_START_TIMESTAMP = millis();
  
  // Reference profile data
  //
  Serial.println("Load pressure profile data...");

  // Status to In Use
  //
  setStatusToInUse();

  // Start brew timer, stop when expires
  //
  BREW_TIMER.in((long)BREW_TIME*1000, [](void *argument) -> bool {
    CURRENT_BREW_TIME = BREW_TIME;
    stopBrew();
  });

  // Send initial volts to pump as determined by profile
  //
  Serial.println("Apply initial pump voltage...");

  // Manage brew data in loop while timer is active
  BREW_TIMER.every(250, [](void *argument) -> bool { 
		BREW_CURRENT_TIMESTAMP = millis();
    int timeEllapsedinSecs = (BREW_CURRENT_TIMESTAMP - BREW_START_TIMESTAMP)/1000;

    // Manage shot timer on screen
    //
    CURRENT_BREW_TIME = timeEllapsedinSecs < BREW_TIME ? timeEllapsedinSecs : BREW_TIME;

    // Update volts sent to pump as determined by profile
    //
    Serial.println("Modify pump voltage based on profile...");
	 });
}

void stopBrew() {
  // Temporarily switch status to Wait, then Ready
  //
  setStatusToWait();
  STOP_BREW_TIMER.in(5000, [](void *argument) -> bool { 
    setStatusToReady();
    STOP_BREW_TIMER.cancel();
  });

  // Stop all timers
  //
  BREW_TIMER.cancel();

  // Stop volts to pump
  //
  Serial.println("Stop pump voltage...");
}

void setStatusToStartingUp() {
  CURRENT_STATUS = STARTING_UP;
  CURRENT_STATUS_DISPLAY = "      ";
  CURRENT_LED_COLOR = WHITE;
}

void setStatusToDisabled() {
  CURRENT_STATUS = DISABLED;
  CURRENT_STATUS_DISPLAY = "      ";
  CURRENT_LED_COLOR = OUT;
}

void setStatusToReady() {
  CURRENT_STATUS = READY;
  CURRENT_STATUS_DISPLAY = "Ready!";
  CURRENT_LED_COLOR = GREEN;
}

void setStatusToWait() {
  CURRENT_STATUS = WAIT;
  CURRENT_STATUS_DISPLAY = "Stop!!";
  CURRENT_LED_COLOR = RED;
}

void setStatusToInUse() {
  CURRENT_STATUS = IN_USE;
  CURRENT_STATUS_DISPLAY = "In Use";
  CURRENT_LED_COLOR = BLUE;
}

void setProfileToOff() {
  CURRENT_PROFILE_DISPLAY = "Off      ";
  CURRENT_PROFILE = OFF;
}

void setProfileToA() {
  CURRENT_PROFILE_DISPLAY = "Profile A";
  CURRENT_PROFILE = OFF;
}

void setProfileToB() {
  CURRENT_PROFILE_DISPLAY = "Profile B";
  CURRENT_PROFILE = OFF;
}

void setProfileToC() {
  CURRENT_PROFILE_DISPLAY = "Profile C";
  CURRENT_PROFILE = OFF;
}

void setProfileToD() {
  CURRENT_PROFILE_DISPLAY = "Profile D";
  CURRENT_PROFILE = OFF;
}

void clearLastBrewTime() {
  CURRENT_BREW_TIME = 0;
}

void setIndicatorLED() {
  switch (CURRENT_LED_COLOR) {
    case OUT:
      analogWrite(STATUS_LED_R_PIN, 0); 
      analogWrite(STATUS_LED_G_PIN, 0);
      analogWrite(STATUS_LED_B_PIN, 0);
      break;
    case RED:
      analogWrite(STATUS_LED_R_PIN, 255); 
      analogWrite(STATUS_LED_G_PIN, 0);
      analogWrite(STATUS_LED_B_PIN, 0);
      break;
    case BLUE:
      analogWrite(STATUS_LED_R_PIN, 0); 
      analogWrite(STATUS_LED_G_PIN, 0);
      analogWrite(STATUS_LED_B_PIN, 255);
      break;
    case GREEN:
      analogWrite(STATUS_LED_R_PIN, 0); 
      analogWrite(STATUS_LED_G_PIN, 255);
      analogWrite(STATUS_LED_B_PIN, 0);
      break;
    case WHITE:
      analogWrite(STATUS_LED_R_PIN, 255); 
      analogWrite(STATUS_LED_G_PIN, 255);
      analogWrite(STATUS_LED_B_PIN, 255);
      break;
  }
}