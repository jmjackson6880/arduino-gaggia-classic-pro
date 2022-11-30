#include <Arduino.h>

int32_t charid_string;

#define PWM_PIN 9
int PWM_MIN_VALUE = 0;
int PWM_MAX_VALUE = 255;

int red_light_pin= 2;
int green_light_pin = 4;
int blue_light_pin = 7;



void setup(void) {
  Serial.begin(9600);
	//pinMode(PWM_PIN, OUTPUT);

	pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);
}

void loop(void) {

	// Write a value 55-255
	//
	//analogWrite(PWM_PIN, PWM_MIN_VALUE);
	RGB_color(255, 0, 0); // Red
  delay(1000);
  RGB_color(0, 255, 0); // Off
  delay(1000);
  RGB_color(0, 0, 255); // Blue
	delay(1000);
	// RGB_color(0, 255, 0); // Green
  // delay(1000);
}

void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
 {
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}