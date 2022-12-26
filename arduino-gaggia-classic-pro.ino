#include <Arduino.h>
#include <BasicEncoder.h>
#include <arduino-timer.h>
#include <movingAvg.h>
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

String DEVICE_BUILD_VERSION = "0.9";
String DEVICE_NAME = "Gaggia Classic Pro";

unsigned long START_TIMESTAMP = 0;
unsigned long BUTTON_PRESSED_TIMESTAMP = 0;
unsigned long BUTTON_PRESSED_PREV_TIMESTAMP = 0;
unsigned long SHOT_START_TIMESTAMP = 0;
unsigned long SHOT_CURRENT_TIMESTAMP = 0;

auto START_UP_TIMER = timer_create_default();
auto SHOT_TIMER = timer_create_default();
auto WAIT_STATUS_TIMER = timer_create_default();
auto LOOP_DEBOUNCED = timer_create_default();

enum DEVICE_STATUS
{
	STARTING_UP,
	WAIT,
	READY,
	IN_USE
};
enum DEVICE_STATUS CURRENT_DEVICE_STATUS;

enum DEVICE_PROFILES
{
	PROFILE_A,
	PROFILE_B,
	PROFILE_C,
	PROFILE_D,
	PROFILE_E
};
enum DEVICE_PROFILES USER_PROFILE = PROFILE_A;

int DEVICE_MIN_SHOT_TIME = 7;
int DEVICE_MAX_SHOT_TIME = 91;

int USER_SHOT_TIME = DEVICE_MIN_SHOT_TIME;
int CURRENT_SHOT_TIME = 0;
enum DEVICE_MODES
{
	MANUAL,
	AUTOMATIC
};
enum DEVICE_MODES USER_DEVICE_MODE = MANUAL;

#define POTENTIOMETER_PIN A0
int POTENTIOMETER_VALUE;
int POTENTIOMETER_RAW_VALUE;
movingAvg POTENTIOMETER_AVERAGE_VALUE(20);

enum PUSH_BUTTON_STATES
{
	IS_PRESSED,
	IS_NOT_PRESSED
};

#define ROTARY_ENCODER_BUTTON_PIN 5
enum PUSH_BUTTON_STATES ROTARY_ENCODER_BUTTON_VALUE = IS_NOT_PRESSED;

#define PRIMARY_BUTTON_PIN 2
#define PRIMARY_BUTTON_LED_PIN 8
enum PUSH_BUTTON_STATES PRIMARY_BUTTON_VALUE = IS_NOT_PRESSED;
boolean isPrimaryButtonPressed = PRIMARY_BUTTON_VALUE == IS_PRESSED;

#define SECONDARY_BUTTON_PIN 3
#define SECONDARY_BUTTON_LED_PIN 4
enum PUSH_BUTTON_STATES SECONDARY_BUTTON_VALUE = IS_NOT_PRESSED;
boolean isSecondaryButtonPressed = SECONDARY_BUTTON_VALUE == IS_PRESSED;

#define PWM_VOLTAGE_OUT_PIN 9
int PWM_VOLTAGE_OUT_VALUE = 0;
int PWM_VOLTAGE_OUT_MIN_VALUE = 0;
int PWM_VOLTAGE_OUT_MAX_VALUE = 255;

BasicEncoder RotaryEncoder(12, 13);
int ROTARY_ENCODER_VALUE;
int ROTARY_ENCODER_PREV_VALUE = 0;

#define LCD_RS_PIN A5
#define LCD_EN_PIN A4
#define LCD_DB4_PIN A3
#define LCD_DB5_PIN A2
#define LCD_DB6_PIN A1
#define LCD_DB7_PIN 7

Adafruit_LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_DB4_PIN, LCD_DB5_PIN, LCD_DB6_PIN, LCD_DB7_PIN);
int32_t charid_string;

#define LCD_BACKLIGHT_R_PIN 11
#define LCD_BACKLIGHT_G_PIN 10
#define LCD_BACKLIGHT_B_PIN 6

void setup()
{
	Serial.begin(9600);

	pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);

	pinMode(PRIMARY_BUTTON_PIN, INPUT_PULLUP);
	pinMode(PRIMARY_BUTTON_LED_PIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(PRIMARY_BUTTON_PIN), primaryButtonHandler, CHANGE);

	pinMode(SECONDARY_BUTTON_PIN, INPUT_PULLUP);
	pinMode(SECONDARY_BUTTON_LED_PIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(SECONDARY_BUTTON_PIN), secondaryButtonHandler, CHANGE);

	pinMode(LCD_BACKLIGHT_R_PIN, OUTPUT);
	pinMode(LCD_BACKLIGHT_G_PIN, OUTPUT);
	pinMode(LCD_BACKLIGHT_B_PIN, OUTPUT);

	lcd.begin(20, 4);
	setInitialScreen();

	START_TIMESTAMP = millis();
	LOOP_DEBOUNCED.every(1, debouncedLoop);
	POTENTIOMETER_AVERAGE_VALUE.begin();

	setStatusToStartingUp();
}

void loop()
{
	START_UP_TIMER.tick();
	SHOT_TIMER.tick();
	WAIT_STATUS_TIMER.tick();
	LOOP_DEBOUNCED.tick();

	switch (CURRENT_DEVICE_STATUS)
	{
	case STARTING_UP:
		START_UP_TIMER.in(2000, [](void *argument) -> bool
											{ 
				setStatusToReady();
        START_UP_TIMER.cancel(); });
		break;
	case WAIT:
		break;
	case READY:
		// rotaryEncoderButtonHandler();
		rotaryEncoderHandler();
		break;
	case IN_USE:
		break;
	}

	if (CURRENT_DEVICE_STATUS != STARTING_UP)
	{
		updateLcdDisplay();
	}
}

void debouncedLoop()
{
	RotaryEncoder.service();
	potentiometerHandler();
}

void potentiometerHandler()
{
	POTENTIOMETER_RAW_VALUE = analogRead(POTENTIOMETER_PIN);
	POTENTIOMETER_AVERAGE_VALUE.reading(POTENTIOMETER_RAW_VALUE);
	int movingAverage = POTENTIOMETER_AVERAGE_VALUE.getAvg();

	if (movingAverage < 25)
	{
		POTENTIOMETER_VALUE = 0;
	}
	else if (movingAverage > 925)
	{
		POTENTIOMETER_VALUE = 1024;
	}
	else
	{
		POTENTIOMETER_VALUE = ((float(movingAverage) - 25) / 900) * 1023;
	}

	switch (USER_DEVICE_MODE)
	{
	case MANUAL:
		setPumpVoltage();
		break;
	case AUTOMATIC:
		setShotTime();
		break;
	}
}

void rotaryEncoderButtonHandler()
{
	ROTARY_ENCODER_BUTTON_VALUE = (digitalRead(ROTARY_ENCODER_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;
	BUTTON_PRESSED_TIMESTAMP = millis();
	boolean isButtonDebounceComplete = (BUTTON_PRESSED_TIMESTAMP - BUTTON_PRESSED_PREV_TIMESTAMP) > 1000;

	if (isButtonDebounceComplete)
	{
		BUTTON_PRESSED_PREV_TIMESTAMP = BUTTON_PRESSED_TIMESTAMP;
		USER_DEVICE_MODE = USER_DEVICE_MODE == MANUAL ? AUTOMATIC : MANUAL;
	}
}

void rotaryEncoderHandler()
{
	int rotaryEncoderChange = RotaryEncoder.get_change();

	if (rotaryEncoderChange)
	{
		ROTARY_ENCODER_VALUE = RotaryEncoder.get_count();

		if (ROTARY_ENCODER_VALUE > ROTARY_ENCODER_PREV_VALUE)
		{
			setProfile("next");
		}
		else if (ROTARY_ENCODER_VALUE < ROTARY_ENCODER_PREV_VALUE)
		{
			setProfile("previous");
		}

		ROTARY_ENCODER_PREV_VALUE = ROTARY_ENCODER_VALUE;
	}
}

void primaryButtonHandler()
{
	PRIMARY_BUTTON_VALUE = (digitalRead(PRIMARY_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;
	BUTTON_PRESSED_TIMESTAMP = millis();
	boolean isButtonDebounceComplete = (BUTTON_PRESSED_TIMESTAMP - BUTTON_PRESSED_PREV_TIMESTAMP) > 500;

	if (isButtonDebounceComplete)
	{
		BUTTON_PRESSED_PREV_TIMESTAMP = BUTTON_PRESSED_TIMESTAMP;

		switch (CURRENT_DEVICE_STATUS)
		{
		case READY:
			setStatusToInUse();
			startShot();
			break;
		}
	}
}

void secondaryButtonHandler()
{
	SECONDARY_BUTTON_VALUE = (digitalRead(SECONDARY_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;
	BUTTON_PRESSED_TIMESTAMP = millis();
	boolean isButtonDebounceComplete = (BUTTON_PRESSED_TIMESTAMP - BUTTON_PRESSED_PREV_TIMESTAMP) > 500;

	if (isButtonDebounceComplete)
	{
		BUTTON_PRESSED_PREV_TIMESTAMP = BUTTON_PRESSED_TIMESTAMP;

		switch (CURRENT_DEVICE_STATUS)
		{
		case IN_USE:
			setStatusToWait();
			stopShot();
			break;
		case READY:
			clearLastShotTime();
			break;
		}
	}
}

void setInitialScreen()
{
	lcd.setCursor(0, 0);
	lcd.print(DEVICE_NAME);

	lcd.setCursor(0, 1);
	lcd.print("--------------------");

	lcd.setCursor(0, 2);
	lcd.print(getStatusName() + "...");

	lcd.setCursor(0, 3);
	lcd.print("Build version: " + DEVICE_BUILD_VERSION);
}

void updateLcdDisplay()
{
	lcd.setCursor(0, 0);
	lcd.print("---- Timer: ");

	lcd.setCursor(12, 0);
	lcd.print((CURRENT_SHOT_TIME < 10) ? '0' + String(CURRENT_SHOT_TIME) : String(CURRENT_SHOT_TIME));

	lcd.setCursor(16, 0);
	lcd.print("----");

	lcd.setCursor(0, 1);
	lcd.print(getModeName());

	if (USER_DEVICE_MODE == MANUAL)
	{
		setLcdForManualMode();
	}
	else
	{
		setLcdForAutomaticMode();
	}
}

void setLcdForManualMode()
{
	int pumpVoltageAsPercentage = ((float)PWM_VOLTAGE_OUT_VALUE / 255) * 100;

	lcd.setCursor(0, 2);
	lcd.print("                    ");

	lcd.setCursor(0, 3);
	lcd.print("---- Pump: ");

	lcd.setCursor(11, 3);
	lcd.print(String(pumpVoltageAsPercentage) + "%  ");

	lcd.setCursor(16, 3);
	lcd.print("----");
}

void setLcdForAutomaticMode()
{
	lcd.setCursor(0, 2);
	lcd.print(getProfileName());

	lcd.setCursor(0, 3);
	lcd.print("-- Shot Time: ");

	lcd.setCursor(15, 3);
	lcd.print((USER_SHOT_TIME < 10) ? '0' + String(USER_SHOT_TIME) : String(USER_SHOT_TIME));

	lcd.setCursor(18, 3);
	lcd.print("--");
}

void setStatusToStartingUp()
{
	CURRENT_DEVICE_STATUS = STARTING_UP;
	analogWrite(PRIMARY_BUTTON_LED_PIN, 255);
	analogWrite(SECONDARY_BUTTON_LED_PIN, 255);

	analogWrite(LCD_BACKLIGHT_R_PIN, 0);
	analogWrite(LCD_BACKLIGHT_G_PIN, 255);
	analogWrite(LCD_BACKLIGHT_B_PIN, 255);
}

void setStatusToReady()
{
	lcd.clear();

	CURRENT_DEVICE_STATUS = READY;
	analogWrite(SECONDARY_BUTTON_LED_PIN, 0);

	if (USER_DEVICE_MODE == MANUAL)
	{
		analogWrite(PRIMARY_BUTTON_LED_PIN, 255);
		analogWrite(LCD_BACKLIGHT_R_PIN, 0);
		analogWrite(LCD_BACKLIGHT_G_PIN, 0);
		analogWrite(LCD_BACKLIGHT_B_PIN, 0);
	}
	else
	{
		analogWrite(PRIMARY_BUTTON_LED_PIN, 0);
		analogWrite(LCD_BACKLIGHT_R_PIN, 255);
		analogWrite(LCD_BACKLIGHT_G_PIN, 0);
		analogWrite(LCD_BACKLIGHT_B_PIN, 255);
	}
}

void setStatusToWait()
{
	CURRENT_DEVICE_STATUS = WAIT;
	analogWrite(PRIMARY_BUTTON_LED_PIN, 255);
	analogWrite(SECONDARY_BUTTON_LED_PIN, 255);

	analogWrite(LCD_BACKLIGHT_R_PIN, 0);
	analogWrite(LCD_BACKLIGHT_G_PIN, 255);
	analogWrite(LCD_BACKLIGHT_B_PIN, 255);

	WAIT_STATUS_TIMER.in(5000, [](void *argument) -> bool
											 { 
				setStatusToReady();
        WAIT_STATUS_TIMER.cancel(); });
}

void setStatusToInUse()
{
	CURRENT_DEVICE_STATUS = IN_USE;
	analogWrite(PRIMARY_BUTTON_LED_PIN, 255);
	analogWrite(SECONDARY_BUTTON_LED_PIN, 0);

	analogWrite(LCD_BACKLIGHT_R_PIN, 255);
	analogWrite(LCD_BACKLIGHT_G_PIN, 255);
	analogWrite(LCD_BACKLIGHT_B_PIN, 0);
}

void setModeToManual()
{
	USER_DEVICE_MODE = MANUAL;
	stopPumpVoltage();
	stopShot();
}

void setModeToAutomatic()
{
	USER_DEVICE_MODE = AUTOMATIC;
}

void setProfile(String dir)
{
	switch (USER_PROFILE)
	{
	case PROFILE_A:
		USER_PROFILE = dir == "next" ? PROFILE_B : PROFILE_E;
		break;
	case PROFILE_B:
		USER_PROFILE = dir == "next" ? PROFILE_C : PROFILE_A;
		break;
	case PROFILE_C:
		USER_PROFILE = dir == "next" ? PROFILE_D : PROFILE_B;
		break;
	case PROFILE_D:
		USER_PROFILE = dir == "next" ? PROFILE_E : PROFILE_C;
		break;
	case PROFILE_E:
		USER_PROFILE = dir == "next" ? PROFILE_A : PROFILE_D;
		break;
	}
}

void setShotTime()
{
	int allowedShotTimeRange = DEVICE_MAX_SHOT_TIME - DEVICE_MIN_SHOT_TIME;
	USER_SHOT_TIME = (float)POTENTIOMETER_VALUE * allowedShotTimeRange + DEVICE_MIN_SHOT_TIME;
}

void updateShotTimer()
{
	SHOT_CURRENT_TIMESTAMP = millis();
	int timeEllapsedinSecs = (SHOT_CURRENT_TIMESTAMP - SHOT_START_TIMESTAMP) / 1000;

	CURRENT_SHOT_TIME = timeEllapsedinSecs < USER_SHOT_TIME ? timeEllapsedinSecs : USER_SHOT_TIME;
}

void setPumpVoltage()
{
	PWM_VOLTAGE_OUT_VALUE = ((float)POTENTIOMETER_VALUE / 1024) * 255;
	analogWrite(PWM_VOLTAGE_OUT_PIN, PWM_VOLTAGE_OUT_VALUE);
}

void stopPumpVoltage()
{
	PWM_VOLTAGE_OUT_VALUE = 0;
	analogWrite(PWM_VOLTAGE_OUT_PIN, PWM_VOLTAGE_OUT_VALUE);
}

void clearLastShotTime()
{
	CURRENT_SHOT_TIME = 0;
}

String getProfileName()
{
	switch (USER_PROFILE)
	{
	case PROFILE_A:
		return "3.5 bar pre-infuse  ";
		break;
	case PROFILE_B:
		return "Ramp up + down      ";
		break;
	case PROFILE_C:
		return "6 bar shot          ";
		break;
	case PROFILE_D:
		return "9 bar shot          ";
		break;
	case PROFILE_E:
		return "Turbo shot          ";
		break;
	}
}

String getModeName()
{
	switch (USER_DEVICE_MODE)
	{
	case MANUAL:
		return "Manual Mode";
		break;
	case AUTOMATIC:
		return "Automatic";
		break;
	}
}

String getStatusName()
{
	switch (CURRENT_DEVICE_STATUS)
	{
	case STARTING_UP:
		return "Starting up";
		break;
	case READY:
		return "Ready";
		break;
	case WAIT:
		return "Wait";
		break;
	case IN_USE:
		return "In Use";
		break;
	}
}

void startShot()
{
	SHOT_START_TIMESTAMP = millis();

	clearLastShotTime();
	setStatusToInUse();
	setPumpVoltage(); // min, based on profile

	SHOT_TIMER.in((long)USER_SHOT_TIME * 1000, handleShotTimerExpire);
	SHOT_TIMER.every(250, handleShotUpdates);
}

bool handleShotUpdates(void *argument)
{
	setPumpVoltage(); // update, based on timeline + profile
	updateShotTimer();
	return true;
}

bool handleShotTimerExpire(void *argument)
{
	CURRENT_SHOT_TIME = USER_SHOT_TIME;
	stopShot();
	return false;
}

void stopShot()
{
	setStatusToWait();
	stopPumpVoltage();
	SHOT_TIMER.cancel();
}