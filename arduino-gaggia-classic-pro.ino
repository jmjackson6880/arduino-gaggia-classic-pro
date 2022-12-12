#include <Arduino.h>
#include "Adafruit_LiquidCrystal.h"
#include <arduino-timer.h>
#include <BasicEncoder.h>

Adafruit_LiquidCrystal lcd(0);
int32_t charid_string;

enum INPUTS
{
	POTENTIOMETER,
	ROTARY_ENCODER_BUTTON,
	ROTARY_ENCODER,
	PRIMARY_BUTTON,
	SECONDARY_BUTTON
};
enum OUTPUTS
{
	PRIMARY_BUTTON_LED,
	SECONDARY_BUTTON_LED,
	PWM_VOLTAGE_OUT
};

unsigned long START_TIMESTAMP = 0;
unsigned long CURRENT_TIMESTAMP = 0;
unsigned long LAST_THROTTLE_TIMESTAMP = 0;
unsigned long BUTTON_PRESSED_TIMESTAMP = 0;
unsigned long BUTTON_PRESSED_PREV_TIMESTAMP = 0;
unsigned long SHOT_START_TIMESTAMP = 0;
unsigned long SHOT_CURRENT_TIMESTAMP = 0;

auto START_UP_TIMER = timer_create_default();
auto SHOT_TIMER = timer_create_default();
auto WAIT_STATUS_TIMER = timer_create_default();

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
float POTENTIOMETER_VALUE;
float POTENTIOMETER_PREV_VALUE = 0;
float POTENTIOMETER_AMOUNT_VALUE_CHANGED = 0;

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

#define LCD_BACKLIGHT_R_PIN 11
#define LCD_BACKLIGHT_G_PIN 10
#define LCD_BACKLIGHT_B_PIN 6

void setup()
{
	Serial.begin(9600);

	lcd.begin(20, 4);
	lcd.clear();

	lcd.setCursor(0, 0);
	lcd.print("setup() ...");

	START_TIMESTAMP = millis();
	LAST_THROTTLE_TIMESTAMP = START_TIMESTAMP;

	setStatusToStartingUp();

	pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_BUTTON_PIN), rotaryEncoderButtonHandler, CHANGE);

	pinMode(PRIMARY_BUTTON_PIN, INPUT_PULLUP);
	pinMode(PRIMARY_BUTTON_LED_PIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(PRIMARY_BUTTON_PIN), primaryButtonHandler, CHANGE);

	pinMode(SECONDARY_BUTTON_PIN, INPUT_PULLUP);
	pinMode(SECONDARY_BUTTON_LED_PIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(SECONDARY_BUTTON_PIN), secondaryButtonHandler, CHANGE);

	pinMode(LCD_BACKLIGHT_R_PIN, OUTPUT);
	pinMode(LCD_BACKLIGHT_G_PIN, OUTPUT);
	pinMode(LCD_BACKLIGHT_B_PIN, OUTPUT);
}

void loop()
{
	// Throttled
	//
	CURRENT_TIMESTAMP = millis();

	if ((CURRENT_TIMESTAMP - LAST_THROTTLE_TIMESTAMP) >= 2000)
	{
		LAST_THROTTLE_TIMESTAMP = millis();
		_throttled();
	}

	// LCD Backlight Test
	//
	//  analogWrite(LCD_BACKLIGHT_R_PIN, 50);
	//  analogWrite(LCD_BACKLIGHT_G_PIN, 0);
	//  analogWrite(LCD_BACKLIGHT_B_PIN, 50);

	// Managers
	START_UP_TIMER.tick();
	SHOT_TIMER.tick();
	WAIT_STATUS_TIMER.tick();
	RotaryEncoder.service();

	// Loop based on CURRENT_DEVICE_STATUS
	switch (CURRENT_DEVICE_STATUS)
	{
	case STARTING_UP:
		START_UP_TIMER.in(2000, [](void *argument) -> bool
											{ 
				setStatusToReady();
        START_UP_TIMER.cancel(); });
		break;
	case WAIT:
		/* Handle wait mode */
		break;
	case READY:
		potentiometerHandler();
		rotaryEncoderHandler();
		break;
	case IN_USE:
		/* Handle in use */
		break;
	}
}

void potentiometerHandler()
{
	POTENTIOMETER_VALUE = (float)analogRead(POTENTIOMETER_PIN) / 1024;
	POTENTIOMETER_AMOUNT_VALUE_CHANGED = abs(POTENTIOMETER_VALUE - POTENTIOMETER_PREV_VALUE);
	boolean potentiometerChangeHasExceededThreshold = POTENTIOMETER_AMOUNT_VALUE_CHANGED >= .01;

	if (potentiometerChangeHasExceededThreshold)
	{
		POTENTIOMETER_PREV_VALUE = POTENTIOMETER_VALUE;

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
	boolean rotaryEncoderValueHasIncreased = ROTARY_ENCODER_VALUE > ROTARY_ENCODER_PREV_VALUE;
	boolean rotaryEncoderValueHasDecreased = ROTARY_ENCODER_VALUE < ROTARY_ENCODER_PREV_VALUE;

	if (rotaryEncoderChange)
	{
		ROTARY_ENCODER_VALUE = RotaryEncoder.get_count();

		if (rotaryEncoderValueHasIncreased)
		{
			setProfile("next");
		}
		else if (rotaryEncoderValueHasDecreased)
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

void setStatusToStartingUp()
{
	CURRENT_DEVICE_STATUS = STARTING_UP;
	analogWrite(PRIMARY_BUTTON_LED_PIN, 255);
	analogWrite(SECONDARY_BUTTON_LED_PIN, 255);
}

void setStatusToReady()
{
	CURRENT_DEVICE_STATUS = READY;
	analogWrite(PRIMARY_BUTTON_LED_PIN, 0);
	analogWrite(SECONDARY_BUTTON_LED_PIN, 0);
}

void setStatusToWait()
{
	CURRENT_DEVICE_STATUS = WAIT;
	analogWrite(PRIMARY_BUTTON_LED_PIN, 255);
	analogWrite(SECONDARY_BUTTON_LED_PIN, 255);

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
}

void setModeToManual()
{
	USER_DEVICE_MODE = MANUAL;
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
	PWM_VOLTAGE_OUT_VALUE = POTENTIOMETER_VALUE >= .01 ? (float)POTENTIOMETER_VALUE * 255 + 1 : 0;
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
		return "3.5 bar pre-infuse";
		break;
	case PROFILE_B:
		return "Ramp up + down";
		break;
	case PROFILE_C:
		return "6 bar shot";
		break;
	case PROFILE_D:
		return "9 bar shot";
		break;
	case PROFILE_E:
		return "Turbo shot";
		break;
	}
}

String getModeName()
{
	switch (USER_DEVICE_MODE)
	{
	case MANUAL:
		return "Manual";
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

void _throttled()
{
	Serial.println("_throttled() ... {");
	Serial.print("  CURRENT_DEVICE_STATUS: ");
	Serial.print(getStatusName());
	Serial.println("  ");
	Serial.print("  USER_DEVICE_MODE: ");
	Serial.print(USER_DEVICE_MODE == MANUAL ? "Manual" : "Automatic");
	Serial.println("  ");
	Serial.print("  USER_PROFILE: ");
	Serial.print(getProfileName());
	Serial.println("  ");
	Serial.print("  USER_SHOT_TIME: ");
	Serial.print(USER_SHOT_TIME);
	Serial.println("  ");
	Serial.print("  CURRENT_SHOT_TIME: ");
	Serial.print(CURRENT_SHOT_TIME);
	Serial.println("  ");
	Serial.print("  PWM_VOLTAGE_OUT_VALUE: ");
	Serial.print(PWM_VOLTAGE_OUT_VALUE);
	Serial.println("  ");
	Serial.println("  ------  ");
	Serial.print("  POTENTIOMETER_VALUE: ");
	Serial.print(POTENTIOMETER_VALUE);
	Serial.println("  ");
	Serial.print("  ROTARY_ENCODER_BUTTON_VALUE: ");
	Serial.print(ROTARY_ENCODER_BUTTON_VALUE == IS_PRESSED ? "Pressed" : "Not Pressed");
	Serial.println("  ");
	Serial.print("  ROTARY_ENCODER_VALUE: ");
	Serial.print(ROTARY_ENCODER_VALUE);
	Serial.println("  ");
	Serial.print("  PRIMARY_BUTTON_VALUE: ");
	Serial.print(PRIMARY_BUTTON_VALUE == IS_PRESSED ? "Pressed" : "Not Pressed");
	Serial.println("  ");
	Serial.print("  SECONDARY_BUTTON_VALUE: ");
	Serial.print(SECONDARY_BUTTON_VALUE == IS_PRESSED ? "Pressed" : "Not Pressed");
	Serial.println("  ");
	Serial.println("}");
}