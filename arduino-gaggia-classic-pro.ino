#include <Arduino.h>
#include <arduino-timer.h>
#include <BasicEncoder.h>

// Cases to catch
// If switch to manual, make sure pot is turned CCW/does not send volts
//

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

// auto STOP_BREW_TIMER = timer_create_default();
// auto BREW_TIMER = timer_create_default();
auto START_UP_TIMER = timer_create_default();

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
float POTENTIOMETER_VALUE_CHANGE_THRESHOLD = .01;

enum PUSH_BUTTON_STATES
{
	IS_PRESSED,
	IS_NOT_PRESSED
};

#define ROTARY_ENCODER_BUTTON_PIN 2
enum PUSH_BUTTON_STATES ROTARY_ENCODER_BUTTON_VALUE = IS_NOT_PRESSED;

#define PRIMARY_BUTTON_PIN A2
enum PUSH_BUTTON_STATES PRIMARY_BUTTON_VALUE = IS_NOT_PRESSED;
boolean isPrimaryButtonPressed = PRIMARY_BUTTON_VALUE == HIGH;

#define SECONDARY_BUTTON_PIN A3
enum PUSH_BUTTON_STATES SECONDARY_BUTTON_VALUE = IS_NOT_PRESSED;
boolean isSecondaryButtonPressed = SECONDARY_BUTTON_VALUE == HIGH;

#define PWM_VOLTAGE_OUT_PIN 9
int PWM_VOLTAGE_OUT_VALUE = 0;
int PWM_VOLTAGE_OUT_MIN_VALUE = 0;
int PWM_VOLTAGE_OUT_MAX_VALUE = 255;

BasicEncoder RotaryEncoder(12, 13);
int ROTARY_ENCODER_VALUE;
int ROTARY_ENCODER_PREV_VALUE = 0;

#define PRIMARY_BUTTON_LED_PIN 4
#define SECONDARY_BUTTON_LED_PIN 7

// Rotary Encoder

// Primary Button

// Secondary Button

void setup()
{
	Serial.begin(9600);

	START_TIMESTAMP = millis();
	LAST_THROTTLE_TIMESTAMP = START_TIMESTAMP;

	setStatusToStartingUp();

	pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);
	pinMode(PRIMARY_BUTTON_LED_PIN, OUTPUT);
	pinMode(SECONDARY_BUTTON_LED_PIN, OUTPUT);

	attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_BUTTON_PIN), rotaryEncoderButtonHandler, CHANGE);
	// attachInterrupt(digitalPinToInterrupt(PRIMARY_BUTTON_PIN), primaryButtonHandler, CHANGE);
	// attachInterrupt(digitalPinToInterrupt(SECONDARY_BUTTON_PIN), secondaryButtonHandler, CHANGE);
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
	//

	// Timers
	START_UP_TIMER.tick();

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

		setPumpVoltage();

		if (isPrimaryButtonPressed)
		{
			// startShot();
		}
		if (isSecondaryButtonPressed)
		{
			// clearLastShotTime();
		}
		break;
	case IN_USE:
		// setPumpVoltage();

		if (isSecondaryButtonPressed)
		{
			// stopShot();
		}
		break;
	}
}

void potentiometerHandler()
{
	POTENTIOMETER_VALUE = (float)analogRead(POTENTIOMETER_PIN) / 1024;
	POTENTIOMETER_AMOUNT_VALUE_CHANGED = abs(POTENTIOMETER_VALUE - POTENTIOMETER_PREV_VALUE);

	if (POTENTIOMETER_AMOUNT_VALUE_CHANGED >= POTENTIOMETER_VALUE_CHANGE_THRESHOLD)
	{
		// Serial.println("UPDATING: Potentiometer threshold reached ... ");
		//
		POTENTIOMETER_PREV_VALUE = POTENTIOMETER_VALUE;
		setShotTime();
	}
}

void rotaryEncoderButtonHandler()
{
	ROTARY_ENCODER_BUTTON_VALUE = (digitalRead(ROTARY_ENCODER_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;
	BUTTON_PRESSED_TIMESTAMP = millis();

	if (BUTTON_PRESSED_TIMESTAMP - BUTTON_PRESSED_PREV_TIMESTAMP > 1000)
	{
		// Serial.println("UPDATING: Rotary Encoder Button is pressed ... ");
		//
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
			Serial.println("UPDATING: Rotary Encoder moved CW ... ");
			//
		}
		else if (ROTARY_ENCODER_VALUE < ROTARY_ENCODER_PREV_VALUE)
		{
			Serial.println("UPDATING: Rotary Encoder moved CCW ... ");
			//
		}

		ROTARY_ENCODER_PREV_VALUE = ROTARY_ENCODER_VALUE;
	}
}

void primaryButtonHandler()
{
	PRIMARY_BUTTON_VALUE = (digitalRead(PRIMARY_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;
	BUTTON_PRESSED_TIMESTAMP = millis();

	if (BUTTON_PRESSED_TIMESTAMP - BUTTON_PRESSED_PREV_TIMESTAMP > 1000)
	{
		// Serial.println("UPDATING: Primary Button is pressed ... ");
		//
		BUTTON_PRESSED_PREV_TIMESTAMP = BUTTON_PRESSED_TIMESTAMP;
	}
}

void secondaryButtonHandler()
{
	SECONDARY_BUTTON_VALUE = (digitalRead(SECONDARY_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;
	BUTTON_PRESSED_TIMESTAMP = millis();

	if (BUTTON_PRESSED_TIMESTAMP - BUTTON_PRESSED_PREV_TIMESTAMP > 1000)
	{
		// Serial.println("UPDATING: Secondary Button is pressed ... ");
		//
		BUTTON_PRESSED_PREV_TIMESTAMP = BUTTON_PRESSED_TIMESTAMP;
	}
}

void setStatusToStartingUp()
{
	CURRENT_DEVICE_STATUS = STARTING_UP;
	analogWrite(PRIMARY_BUTTON_LED_PIN, LOW);
	analogWrite(SECONDARY_BUTTON_LED_PIN, LOW);
}

void setStatusToReady()
{
	CURRENT_DEVICE_STATUS = READY;
	analogWrite(PRIMARY_BUTTON_LED_PIN, HIGH);
	analogWrite(SECONDARY_BUTTON_LED_PIN, HIGH);
}

void setStatusToWait()
{
	CURRENT_DEVICE_STATUS = WAIT;
	analogWrite(PRIMARY_BUTTON_LED_PIN, LOW);
	analogWrite(SECONDARY_BUTTON_LED_PIN, LOW);
}

void setStatusToInUse()
{
	CURRENT_DEVICE_STATUS = IN_USE;
	analogWrite(PRIMARY_BUTTON_LED_PIN, LOW);
	analogWrite(SECONDARY_BUTTON_LED_PIN, HIGH);
}

void setModeToManual()
{
	USER_DEVICE_MODE = MANUAL;
}

void setModeToAutomatic()
{
	USER_DEVICE_MODE = AUTOMATIC;
}

void setShotTime()
{
	int allowedShotTimeRange = DEVICE_MAX_SHOT_TIME - DEVICE_MIN_SHOT_TIME;
	USER_SHOT_TIME = (float)POTENTIOMETER_VALUE * allowedShotTimeRange + DEVICE_MIN_SHOT_TIME;
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

String getProfileName()
{
	switch (USER_PROFILE)
	{
	case PROFILE_A:
		return "10s pre-infuse @ 3.5";
		break;
	case PROFILE_B:
		return "Ramp up & down";
		break;
	case PROFILE_C:
		return "9 bar shot";
		break;
	case PROFILE_D:
		return "6 bar shot";
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
	Serial.print("  PWM_VOLTAGE_OUT_VALUE: ");
	Serial.print(PWM_VOLTAGE_OUT_VALUE);
	Serial.println("  ");
	Serial.println("}");
}