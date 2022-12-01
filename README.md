# Arduino project: Gaggia Classic Pro mod

Just another gratuitous project for technically-minded coffee snobs and giant nerds.

This project builds off of the amazing and well-documented [Shades of Coffee](https://www.shadesofcoffee.co.uk/) mods for the Gaggia Classic Pro. Specifically, the addition of an [upgraded temperature sensor and PID](https://www.shadesofcoffee.co.uk/post-2018/gaggia-classic-pro2019-pid-kit---132din-single-display) for precise control of brew and steam temperatures, as well as the [flow control dimmer kit](https://www.shadesofcoffee.co.uk/post-2018/gaggia-classic---flow-control-dimmer-kit).

This mod takes the dimmer kit a few steps further by introducing an Arduino as a microcontroller along with a series of sensors and output devices. Together, these components provide the functionality necessary for the user to select and load a pressure profile, set a shot time, and push the "start" button. Or, the user has the option to operate the machine manually using a flow control knob.

There are other similar projects out there. [Google "gaggiuino"](https://www.google.com/search?rlz=1C5CHFA_enUS841US841&sxsrf=ALiCzsbGp50YCr51Wm168XTbH1bHXEwS2Q:1669829448061&q=gaggiuino&spell=1&sa=X&ved=2ahUKEwiw7di4t9b7AhU2FFkFHTD4DLwQBSgAegQIBRAB&biw=1902&bih=1373&dpr=1) for more inspiration.

## Warnings & disclaimers

This project requires at least foundational knowledge of programming, electricity, and microcontrollers. Implementing these modifications requires working with dangerous mainline voltages at 110/220 volts and will include the modification of wiring. You will need to use your own knowledge and experience to determine the best way to make these modifications. Destroyed electronics, shocks, fires, injury and death are possible. Make sure you know what you're doing.

## Component to pin mapping

| Component             | Pin     |
| --------------------- | ------- |
| Potentiometer         | A0      |
| Rotary Encoder Button | 2       |
| Rotary Encoder A      | 12      |
| Rotary Encoder B      | 13      |
| Primary Button        | --      |
| Primary Button LED    | 7       |
| Secondary Button      | --      |
| Secondary Button LED  | 4       |
| 110 Voltage Output    | 9 (PWM) |
| 20x4 LCD Screen       | --      |

## Components & costs

List of example components required and estimated costs. 

- [Arduino](https://store-usa.arduino.cc/collections/boards) (Examples built using Nano and Uno)
- [RGB backlight negative LCD 20x4](https://www.adafruit.com/product/498#technical-details)
- PWM 110/220 Dimmer: [Option A](https://www.amazon.com/gp/product/B0BC297G4B/ref=ppx_yo_dt_b_asin_title_o08_s00?ie=UTF8&th=1), [Option B](https://www.amazon.com/gp/product/B06Y1DT1WP/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1)
- [Rotary Encoder](https://www.adafruit.com/product/377)
- [Potentiometer](https://www.adafruit.com/product/1789)
- [Button](https://www.adafruit.com/product/559) (Qty 2)
- [110v AC to 5V DC Converter](https://www.amazon.com/gp/product/B07YXN8J6R/ref=ppx_yo_dt_b_asin_title_o06_s02?ie=UTF8&th=1)

Expect that the total cost for these items will fall between $75-95 USD. You will also need tools and shop supplies that you may or may not already own. Electrical tools (cutters, strippers, multimeter), soldering iron, wire, connectors, resistors, and shrink tube will be necessary to have on hand.
## Circuit

TO DO

## Libraries & Dependencies

- [Arduino](https://docs.arduino.cc/)
- [Arduino Timer](https://www.arduinolibraries.info/libraries/arduino-timer)
- [Basic Encoder](https://www.arduinolibraries.info/libraries/basic-encoder)

In the `.vscode` directory, you'll find some extensions that may be helpful if you are developing in VS Code. Further configuration specific to your OS will be required.

## Documentation & demos

### Device Status

Device status is managed within in the Arduino codebase.

| Status           			|
| --------------------- |
| STARTING_UP         	|
| READY         				|
| IN_USE         				|
| WAIT         					|

### Modes

To toggle the mode, press the rotary encoder button. 

| Modes          				|
| --------------------- |
| MANUAL        				|
| AUTOMATIC         		|

### Brewing: Manual mode

To change the mode, press the rotary encoder button. Observe that the mode will change on the LCD screen.

#### Flow control

The mode must be "Manual". To start a manually flow-controlled shot, turn the potentiometer to the left limit, then begin dialing to the right. The pump will not start unless the potentiometer is returned to the left limit. Observe that the shot time will begin counting on the LCD screen and that voltage is being increasingly applied to the pump. To stop the shot, return the potentiometer to the left limit.

### Brewing: Automatic mode

To change the mode, press the rotary encoder. Observe that the mode will change on the LCD screen.

#### Profiles

The mode must be "Automatic". To set the profile, turn the rotary encoder. Observe that the profile will change on the LCD screen.

#### Setting a shot time

The mode must be "Automatic". To set the shot time, turn the potentiometer. Observe that the shot time will change on the LCD screen. Shot times can be selected between the device minimum (7 secs) and maximum (91 secs).

#### Brewing

The device status must be "Ready". To start a brew, press the Primary button.  Observe that the shot timer will begin counting on the LCD screen and that pump voltage will be applied relative to the current Profile selected for the duration of the selected shot time. When the brew time has expired, the timer and pump voltage will stop, then the device will enter "Wait" status for a few seconds, returning to "Ready".

#### Stop

The device status must be "In Use". To stop an in-progress brew, press the secondary button. Observe that the pump voltage and shot time counter on the LCD screen will stop. The device will then enter "Wait" status for a few seconds, returning to "Ready".


### Clear last shot time

The device status must be "Ready". To clear the last shot time on the screen (optional), press the secondary button. Observe that the shot time displayed on the LCD screen has reset to zero.
