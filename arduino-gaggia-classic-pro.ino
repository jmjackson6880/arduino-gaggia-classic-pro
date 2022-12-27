#include <Arduino.h>
#include <BasicEncoder.h>
#include <arduino-timer.h>
#include <movingAvg.h>
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

#define PWM_VOLTAGE_OUT_PIN 9
#define POTENTIOMETER_PIN A0
#define ROTARY_ENCODER_BUTTON_PIN 5
#define ROTARY_ENCODER_L_PIN 12
#define ROTARY_ENCODER_R_PIN 13
#define PRIMARY_BUTTON_PIN 2
#define PRIMARY_BUTTON_LED_PIN 8
#define SECONDARY_BUTTON_PIN 3
#define SECONDARY_BUTTON_LED_PIN 4
#define LCD_RS_PIN A5
#define LCD_EN_PIN A4
#define LCD_DB4_PIN A3
#define LCD_DB5_PIN A2
#define LCD_DB6_PIN A1
#define LCD_DB7_PIN 7
#define LCD_BACKLIGHT_R_PIN 11
#define LCD_BACKLIGHT_G_PIN 10
#define LCD_BACKLIGHT_B_PIN 6

int DEVICE_MIN_SHOT_TIME = 7;
int DEVICE_MAX_SHOT_TIME = 90;

unsigned long TIMESTAMP_AT_START_UP = 0;

// unsigned long SHOT_START_TIMESTAMP = 0;
// unsigned long SHOT_CURRENT_TIMESTAMP = 0;

int CURRENT_SHOT_TIME = 0;
int USER_SHOT_TIME = DEVICE_MIN_SHOT_TIME;

boolean INPUTS_DISABLED = false;

auto SHOT_TIMER = timer_create_default();
auto WAIT_TIMER = timer_create_default();
auto LOOP_OUTPUT = timer_create_default();

enum DEVICE_STATUS
{
	STARTING_UP,
	WAIT,
	READY,
	IN_USE,
	ERROR
};
enum DEVICE_STATUS CURRENT_DEVICE_STATUS = STARTING_UP;

enum DEVICE_MODES
{
	MANUAL,
	AUTOMATIC
};
enum DEVICE_MODES USER_DEVICE_MODE = MANUAL;

enum DEVICE_PROFILES
{
	PROFILE_A,
	PROFILE_B,
	PROFILE_C,
	PROFILE_D,
	PROFILE_E
};
enum DEVICE_PROFILES USER_PROFILE = PROFILE_A;

enum PUSH_BUTTON_STATES
{
	IS_PRESSED,
	IS_NOT_PRESSED
};
enum PUSH_BUTTON_STATES ROTARY_ENCODER_BUTTON_VALUE = IS_NOT_PRESSED;
enum PUSH_BUTTON_STATES PRIMARY_BUTTON_VALUE = IS_NOT_PRESSED;
enum PUSH_BUTTON_STATES SECONDARY_BUTTON_VALUE = IS_NOT_PRESSED;

int POTENTIOMETER_VALUE = 0;
int POTENTIOMETER_AVERAGE_PREV_VALUE = 0;
movingAvg POTENTIOMETER_AVERAGE_VALUE(20);

BasicEncoder RotaryEncoder(ROTARY_ENCODER_L_PIN, ROTARY_ENCODER_R_PIN);
int ROTARY_ENCODER_VALUE;
int ROTARY_ENCODER_PREV_VALUE = 0;

int PWM_VOLTAGE_OUT_VALUE = 0;

enum LED_STATES
{
	ON,
	OFF
};
enum LED_STATES PRIMARY_BUTTON_LED_VALUE = OFF;
enum LED_STATES SECONDARY_BUTTON_LED_VALUE = OFF;

Adafruit_LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_DB4_PIN, LCD_DB5_PIN, LCD_DB6_PIN, LCD_DB7_PIN);

void setup()
{
	Serial.begin(9600);

	pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);
	pinMode(PRIMARY_BUTTON_PIN, INPUT_PULLUP);
	pinMode(PRIMARY_BUTTON_LED_PIN, OUTPUT);
	pinMode(SECONDARY_BUTTON_PIN, INPUT_PULLUP);
	pinMode(SECONDARY_BUTTON_LED_PIN, OUTPUT);
	pinMode(LCD_BACKLIGHT_R_PIN, OUTPUT);
	pinMode(LCD_BACKLIGHT_G_PIN, OUTPUT);
	pinMode(LCD_BACKLIGHT_B_PIN, OUTPUT);

	TIMESTAMP_AT_START_UP = millis();
	POTENTIOMETER_AVERAGE_VALUE.begin();
	LOOP_OUTPUT.every(125, loopThrottled);

	lcd.begin(20, 4);
	startUpAndWait();

	loopThrottled();
}

void loop()
{
	WAIT_TIMER.tick();
	LOOP_OUTPUT.tick();
	SHOT_TIMER.tick();
	RotaryEncoder.service();

	inputPotentiometer();
	inputPrimaryButton();
	inputSecondaryButton();
	inputRotaryEncoderButton();
	inputRotaryEncoder();
}

void loopThrottled()
{
	outputLcdBgColor();
	outputLcdScreenRowOne();
	outputLcdScreenRowTwo();
	outputLcdScreenRowThree();
	outputLcdScreenRowFour();
	outputVoltage();
	outputPrimaryButtonLed();
	outputSecondaryButtonLed();

	observers();
}

void observers()
{
	boolean shouldStartManualBrew = ((USER_DEVICE_MODE == MANUAL) && (CURRENT_DEVICE_STATUS == READY) && (POTENTIOMETER_VALUE > 0) && (POTENTIOMETER_AVERAGE_PREV_VALUE == 0));
	if (shouldStartManualBrew)
	{
		CURRENT_DEVICE_STATUS = IN_USE;
		// startManualBrew();
	}

	boolean shouldStopManualBrew = ((USER_DEVICE_MODE == MANUAL) && (CURRENT_DEVICE_STATUS == IN_USE) && (POTENTIOMETER_VALUE == 0) && (POTENTIOMETER_AVERAGE_PREV_VALUE > 0));
	if (shouldStopManualBrew)
	{
		stopAndWait();
	}

	POTENTIOMETER_AVERAGE_PREV_VALUE = POTENTIOMETER_VALUE;

	boolean shouldThrowManualModeError = ((USER_DEVICE_MODE == MANUAL) && (CURRENT_DEVICE_STATUS == READY) && (PWM_VOLTAGE_OUT_VALUE > 0));
	if (shouldThrowManualModeError)
	{
		CURRENT_DEVICE_STATUS = ERROR;
	}

	boolean shouldClearManualModeError = ((USER_DEVICE_MODE == MANUAL) && (CURRENT_DEVICE_STATUS == ERROR) && (PWM_VOLTAGE_OUT_VALUE == 0));
	if (shouldClearManualModeError)
	{
		CURRENT_DEVICE_STATUS = READY;
	}
}

void outputLcdScreenRowOne()
{
	lcd.setCursor(0, 0);
	lcd.print("                    ");

	if (CURRENT_DEVICE_STATUS == STARTING_UP)
	{
		lcd.setCursor(0, 0);
		lcd.print("Gaggia Classic Pro");
	}
	else if (CURRENT_DEVICE_STATUS == READY || CURRENT_DEVICE_STATUS == IN_USE)
	{
		lcd.setCursor(0, 0);
		lcd.print("Timer: ");
		lcd.print(CURRENT_SHOT_TIME);
	}
	else if (CURRENT_DEVICE_STATUS == ERROR)
	{
		if (USER_DEVICE_MODE == MANUAL)
		{
			lcd.setCursor(0, 0);
			lcd.print("Set pump to 0%!");
		}
	}
}

void outputLcdScreenRowTwo()
{
	lcd.setCursor(0, 1);
	lcd.print("                    ");

	if (CURRENT_DEVICE_STATUS == STARTING_UP)
	{
		lcd.setCursor(0, 1);
		lcd.print("Starting up...");
	}
	else if (CURRENT_DEVICE_STATUS == READY || CURRENT_DEVICE_STATUS == IN_USE)
	{
		lcd.setCursor(0, 1);
		lcd.print(USER_DEVICE_MODE == MANUAL ? "Manual / Ready" : "Automatic / Ready");
	}
	else if (CURRENT_DEVICE_STATUS == ERROR)
	{
		lcd.setCursor(0, 1);
		lcd.print(USER_DEVICE_MODE == MANUAL ? "Manual / Error" : "Automatic / Error");
	}
}

void outputLcdScreenRowThree()
{
	lcd.setCursor(0, 2);
	lcd.print("                    ");

	if (CURRENT_DEVICE_STATUS == STARTING_UP)
	{
		lcd.setCursor(0, 2);
		lcd.print("Build version: 0.9");
	}
	else if (CURRENT_DEVICE_STATUS == READY || CURRENT_DEVICE_STATUS == IN_USE || CURRENT_DEVICE_STATUS == ERROR)
	{
		if (USER_DEVICE_MODE == MANUAL)
		{
			lcd.setCursor(0, 2);
			lcd.print("Flow control knob");
		}
		else if (USER_DEVICE_MODE == AUTOMATIC)
		{
			lcd.setCursor(0, 2);
			lcd.print((String)ROTARY_ENCODER_VALUE);
		}
	}
}

void outputLcdScreenRowFour()
{
	int pumpVoltageAsPercentage = ((float)PWM_VOLTAGE_OUT_VALUE / 255) * 100;

	lcd.setCursor(0, 3);
	lcd.print("                    ");

	if (CURRENT_DEVICE_STATUS == STARTING_UP)
	{
		lcd.setCursor(0, 2);
		lcd.print("Build version: 0.9");
	}
	else if (CURRENT_DEVICE_STATUS == READY || CURRENT_DEVICE_STATUS == ERROR || CURRENT_DEVICE_STATUS == IN_USE)
	{
		if (USER_DEVICE_MODE == MANUAL)
		{
			lcd.setCursor(0, 3);
			lcd.print(pumpVoltageAsPercentage);
			lcd.print("%");
		}
		else if (USER_DEVICE_MODE == AUTOMATIC)
		{
			lcd.setCursor(0, 3);
			lcd.print(USER_SHOT_TIME);
			lcd.print(" secs");
		}
	}
}

void outputLcdBgColor()
{
	switch (CURRENT_DEVICE_STATUS)
	{
	case STARTING_UP:
		analogWrite(LCD_BACKLIGHT_R_PIN, 0);
		analogWrite(LCD_BACKLIGHT_G_PIN, 150);
		analogWrite(LCD_BACKLIGHT_B_PIN, 255);
		break;
	case WAIT:
		analogWrite(LCD_BACKLIGHT_R_PIN, 0);
		analogWrite(LCD_BACKLIGHT_G_PIN, 150);
		analogWrite(LCD_BACKLIGHT_B_PIN, 255);
		break;
	case ERROR:
		analogWrite(LCD_BACKLIGHT_R_PIN, 0);
		analogWrite(LCD_BACKLIGHT_G_PIN, 255);
		analogWrite(LCD_BACKLIGHT_B_PIN, 255);
		break;
	case IN_USE:
		analogWrite(LCD_BACKLIGHT_R_PIN, 255);
		analogWrite(LCD_BACKLIGHT_G_PIN, 255);
		analogWrite(LCD_BACKLIGHT_B_PIN, 0);
		break;
	case READY:
		if (USER_DEVICE_MODE == MANUAL)
		{
			analogWrite(LCD_BACKLIGHT_R_PIN, 255);
			analogWrite(LCD_BACKLIGHT_G_PIN, 0);
			analogWrite(LCD_BACKLIGHT_B_PIN, 0);
		}
		else if (USER_DEVICE_MODE == AUTOMATIC)
		{
			analogWrite(LCD_BACKLIGHT_R_PIN, 255);
			analogWrite(LCD_BACKLIGHT_G_PIN, 0);
			analogWrite(LCD_BACKLIGHT_B_PIN, 255);
		}
		break;
	}
}

void outputVoltage()
{
	int value = CURRENT_DEVICE_STATUS == IN_USE ? PWM_VOLTAGE_OUT_VALUE : 0;
	analogWrite(PWM_VOLTAGE_OUT_PIN, value);
}

void outputPrimaryButtonLed()
{
	if (CURRENT_DEVICE_STATUS == READY && USER_DEVICE_MODE == AUTOMATIC)
	{
		analogWrite(PRIMARY_BUTTON_LED_PIN, 0);
	}
	else
	{
		analogWrite(PRIMARY_BUTTON_LED_PIN, 255);
	}
}

void outputSecondaryButtonLed()
{
	if (CURRENT_DEVICE_STATUS != WAIT)
	{
		analogWrite(SECONDARY_BUTTON_LED_PIN, 0);
	}
	else
	{
		analogWrite(SECONDARY_BUTTON_LED_PIN, 255);
	}
}

void inputPotentiometer()
{
	POTENTIOMETER_AVERAGE_VALUE.reading(analogRead(POTENTIOMETER_PIN));
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

	setAutomaticShotTimeValue();
	setOutputVoltageValue();
}

void inputPrimaryButton()
{
	PRIMARY_BUTTON_VALUE = (digitalRead(PRIMARY_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;

	if (PRIMARY_BUTTON_VALUE == IS_PRESSED && !INPUTS_DISABLED)
	{
		startInputDelay();
		Serial.println("Primary");
	}
}

void inputSecondaryButton()
{
	SECONDARY_BUTTON_VALUE = (digitalRead(SECONDARY_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;

	if (SECONDARY_BUTTON_VALUE == IS_PRESSED && !INPUTS_DISABLED)
	{
		startInputDelay();
		Serial.println("Secondary");
	}
}

void inputRotaryEncoderButton()
{
	ROTARY_ENCODER_BUTTON_VALUE = (digitalRead(ROTARY_ENCODER_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;

	if (ROTARY_ENCODER_BUTTON_VALUE == IS_PRESSED && !INPUTS_DISABLED)
	{
		startInputDelay();
		setUserDeviceMode();
	}
}

void inputRotaryEncoder()
{
	int rotaryEncoderChange = RotaryEncoder.get_change();

	if (rotaryEncoderChange)
	{
		ROTARY_ENCODER_VALUE = RotaryEncoder.get_count();

		if (ROTARY_ENCODER_VALUE > ROTARY_ENCODER_PREV_VALUE)
		{
			setUserProfile("next");
		}
		else if (ROTARY_ENCODER_VALUE < ROTARY_ENCODER_PREV_VALUE)
		{
			setUserProfile("previous");
		}

		ROTARY_ENCODER_PREV_VALUE = ROTARY_ENCODER_VALUE;
	}
}

void startUpAndWait()
{
	CURRENT_DEVICE_STATUS = STARTING_UP;

	WAIT_TIMER.in(2000, [](void *argument) -> bool
								{ 
				CURRENT_DEVICE_STATUS = READY;
        return false; });
}

void stopAndWait()
{
	CURRENT_DEVICE_STATUS = WAIT;

	WAIT_TIMER.in(3000, [](void *argument) -> bool
								{ 
				CURRENT_DEVICE_STATUS = READY;
        return false; });
}

void startInputDelay()
{
	INPUTS_DISABLED = true;
	WAIT_TIMER.in(1000, [](void *argument) -> bool
								{ 
				INPUTS_DISABLED = false;
        return false; });
}

void setAutomaticShotTimeValue()
{
	int allowedShotTimeRange = DEVICE_MAX_SHOT_TIME - DEVICE_MIN_SHOT_TIME;
	USER_SHOT_TIME = (((float)POTENTIOMETER_VALUE / 1024) * (allowedShotTimeRange + DEVICE_MIN_SHOT_TIME));

	if (USER_SHOT_TIME < DEVICE_MIN_SHOT_TIME)
	{
		USER_SHOT_TIME = DEVICE_MIN_SHOT_TIME;
	}
	else if (USER_SHOT_TIME > DEVICE_MAX_SHOT_TIME)
	{
		USER_SHOT_TIME = DEVICE_MAX_SHOT_TIME;
	}
}

void setOutputVoltageValue()
{
	PWM_VOLTAGE_OUT_VALUE = ((float)POTENTIOMETER_VALUE / 1024) * 255;
}

void setUserDeviceMode()
{
	if (USER_DEVICE_MODE == MANUAL && (CURRENT_DEVICE_STATUS == READY || CURRENT_DEVICE_STATUS == ERROR))
	{
		CURRENT_DEVICE_STATUS = READY;
		USER_DEVICE_MODE = AUTOMATIC;
	}
	else if (USER_DEVICE_MODE == AUTOMATIC && CURRENT_DEVICE_STATUS == READY)
	{
		USER_DEVICE_MODE = MANUAL;
	}
}

void setUserProfile(String dir)
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

// void updateShotTimer()
// {
// 	SHOT_CURRENT_TIMESTAMP = millis();
// 	int timeEllapsedinSecs = (SHOT_CURRENT_TIMESTAMP - SHOT_START_TIMESTAMP) / 1000;

// 	CURRENT_SHOT_TIME = timeEllapsedinSecs < USER_SHOT_TIME ? timeEllapsedinSecs : USER_SHOT_TIME;
// }

// void stopPumpVoltage()
// {
// 	PWM_VOLTAGE_OUT_VALUE = 0;
// 	analogWrite(PWM_VOLTAGE_OUT_PIN, PWM_VOLTAGE_OUT_VALUE);
// }

// void clearLastShotTime()
// {
// 	CURRENT_SHOT_TIME = 0;
// }

// void startShot()
// {
// 	SHOT_START_TIMESTAMP = millis();

// 	clearLastShotTime();
// 	setStatusToInUse();
// 	setPumpVoltage(); // min, based on profile

// 	SHOT_TIMER.in((long)USER_SHOT_TIME * 1000, handleShotTimerExpire);
// 	SHOT_TIMER.every(250, handleShotUpdates);
// }

// bool handleShotUpdates(void *argument)
// {
// 	setPumpVoltage(); // update, based on timeline + profile
// 	updateShotTimer();
// 	return true;
// }

// bool handleShotTimerExpire(void *argument)
// {
// 	CURRENT_SHOT_TIME = USER_SHOT_TIME;
// 	stopShot();
// 	return false;
// }

// void stopShot()
// {
// 	setStatusToWait();
// 	stopPumpVoltage();
// 	SHOT_TIMER.cancel();
// }