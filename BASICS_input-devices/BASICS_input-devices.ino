#include <Arduino.h>

#define POTENTIOMETER_PIN A0
int POTENTIOMETER_VALUE = 0;
int POTENTIOMETER_PREV_VALUE = 0;
int POTENTIOMETER_AMOUNT_VALUE_CHANGED = 0;
int POTENTIOMETER_VALUE_CHANGE_THRESHOLD = 10;

enum PUSH_BUTTON_STATES { IS_PRESSED, IS_NOT_PRESSED };

#define PUSH_BUTTON_PIN 7
enum PUSH_BUTTON_STATES PUSH_BUTTON_VALUE;
enum PUSH_BUTTON_STATES PUSH_BUTTON_PREV_VALUE;

#define ROTARY_BUTTON_PIN 2
enum PUSH_BUTTON_STATES ROTARY_BUTTON_VALUE;
enum PUSH_BUTTON_STATES ROTARY_BUTTON_PREV_VALUE;

#define ROTARY_ENCODER_PIN A1
int ROTARY_ENCODER_VALUE = 0;
int ROTARY_ENCODER_PREV_VALUE = 0;
int ROTARY_ENCODER_AMOUNT_VALUE_CHANGED = 0;
int ROTARY_ENCODER_VALUE_CHANGE_THRESHOLD = 1;

void setup(void) {
  Serial.begin(9600);

	// POTENTIOMETER
	//
	POTENTIOMETER_VALUE = analogRead(POTENTIOMETER_PIN);
	
	// PUSH BUTTON
	//
	pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
	PUSH_BUTTON_VALUE = IS_NOT_PRESSED;

	// ROTARY ENCODER
	//
	pinMode(ROTARY_BUTTON_PIN, INPUT_PULLUP);
	ROTARY_BUTTON_VALUE = IS_NOT_PRESSED;
	ROTARY_ENCODER_VALUE = analogRead(ROTARY_ENCODER_PIN);

	
}

void loop(void) {

	potentiometerHandler();
	pushButtonHandler();
	rotaryButtonHandler();

	// char outChar;
  // while ((outChar=(char)printBuffer.get()) != 0) Serial.print(outChar);
  // AdaEncoder *thisEncoder=NULL;
  // thisEncoder=AdaEncoder::genie();
  // if (thisEncoder != NULL) {
  //   Serial.print(thisEncoder->getID()); Serial.print(':');
  //   clicks=thisEncoder->query();
  //   if (clicks > 0) {
  //     Serial.println(" CW");
  //   }
  //   if (clicks < 0) {
  //      Serial.println(" CCW");
  //   }
  // }


	// ROTARY_ENCODER_VALUE = analogRead(ROTARY_ENCODER_PIN);
	// ROTARY_ENCODER_AMOUNT_VALUE_CHANGED = abs(ROTARY_ENCODER_VALUE - ROTARY_ENCODER_PREV_VALUE);

	// if (ROTARY_ENCODER_AMOUNT_VALUE_CHANGED >= ROTARY_ENCODER_VALUE_CHANGE_THRESHOLD) {
	// 	Serial.println("ROTARY_ENCODER: ");
	// 	Serial.println(ROTARY_ENCODER_PREV_VALUE);

	// 	ROTARY_ENCODER_PREV_VALUE = ROTARY_ENCODER_VALUE;
	// }
	
}

void potentiometerHandler() {
	POTENTIOMETER_VALUE = analogRead(POTENTIOMETER_PIN);
	POTENTIOMETER_AMOUNT_VALUE_CHANGED = abs(POTENTIOMETER_VALUE - POTENTIOMETER_PREV_VALUE);

	if (POTENTIOMETER_AMOUNT_VALUE_CHANGED >= POTENTIOMETER_VALUE_CHANGE_THRESHOLD) {
		Serial.println("POTENTIOMETER: ");
		Serial.println(POTENTIOMETER_PREV_VALUE);

		POTENTIOMETER_PREV_VALUE = POTENTIOMETER_VALUE;
	}
}

void pushButtonHandler() {
	PUSH_BUTTON_VALUE = (digitalRead(PUSH_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;

	if (PUSH_BUTTON_VALUE != PUSH_BUTTON_PREV_VALUE) {
		Serial.println("PUSH_BUTTON: ");

		switch(PUSH_BUTTON_VALUE) {
			case IS_PRESSED:
				Serial.println("IS_PRESSED");
				break;
			case IS_NOT_PRESSED:
				Serial.println("IS_NOT_PRESSED");
				break;
		}

		PUSH_BUTTON_PREV_VALUE = PUSH_BUTTON_VALUE;
	}
}

void rotaryButtonHandler() {
	ROTARY_BUTTON_VALUE = (digitalRead(ROTARY_BUTTON_PIN) == 1) ? IS_PRESSED : IS_NOT_PRESSED;

	if (ROTARY_BUTTON_VALUE != ROTARY_BUTTON_PREV_VALUE) {
		Serial.println("ROTARY_BUTTON: ");

		switch(ROTARY_BUTTON_VALUE) {
			case IS_PRESSED:
				Serial.println("IS_PRESSED");
				break;
			case IS_NOT_PRESSED:
				Serial.println("IS_NOT_PRESSED");
				break;
		}

		ROTARY_BUTTON_PREV_VALUE = ROTARY_BUTTON_VALUE;
	}
}