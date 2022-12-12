# Arduino project: Gaggia Classic Pro mod

Just another gratuitous project for technically-minded coffee snobs and giant nerds.

This project builds off of the amazing and well-documented [Shades of Coffee](https://www.shadesofcoffee.co.uk/) mods for the [Gaggia Classic Pro](https://www.gaggia.com/manual-machines/new-classic/). Specifically, the addition of an [upgraded temperature sensor and PID](https://www.shadesofcoffee.co.uk/post-2018/gaggia-classic-pro2019-pid-kit---132din-single-display) for precise control of brew and steam temperatures, as well as the [flow control dimmer kit](https://www.shadesofcoffee.co.uk/post-2018/gaggia-classic---flow-control-dimmer-kit).

This mod takes the dimmer kit a few steps further by introducing an Arduino as a microcontroller along with a series of sensors and output devices. Together, these components provide the functionality necessary for the user to select and load a pressure profile, set a shot time, and push the "start" button. Or, the user has the option to operate the machine manually using a flow control knob.

There are other similar projects out there. [Google "gaggiuino"](https://www.google.com/search?rlz=1C5CHFA_enUS841US841&sxsrf=ALiCzsbGp50YCr51Wm168XTbH1bHXEwS2Q:1669829448061&q=gaggiuino&spell=1&sa=X&ved=2ahUKEwiw7di4t9b7AhU2FFkFHTD4DLwQBSgAegQIBRAB&biw=1902&bih=1373&dpr=1) for more inspiration.

## Warnings & disclaimers

This project requires at least foundational knowledge of programming, electricity, and microcontrollers. Implementing these modifications requires working with dangerous mainline voltages at 110/220 volts and will include the modification of wiring. You will need to use your own knowledge and experience to determine the best way to make these modifications. Destroyed electronics, shocks, fires, injury and death are possible. Make sure you know what you're doing.

### Caveats

If you don't have a PID installed on your machine, do not bother with this modification. Temperature control comes first.

This is an assumption, but the exact performance of pressure profiles in this project are likely to differ from machine to machine. Age, condition, and versions of the machine will cause subtle differences between users, as will the exact performance and setup of your OVP value / springs. Expect to have to tweak the pressure profile data points to meet your machine. 

## Pin mapping

| Component             | Pin     |
| --------------------- | ------- |
| Potentiometer         | A0      |
| Rotary Encoder Button | 5       |
| Rotary Encoder A      | 12      |
| Rotary Encoder B      | 13      |
| Primary Button        | 2      	|
| Primary Button LED    | 7       |
| Secondary Button      | 3      	|
| Secondary Button LED  | 4       |
| 110 Voltage Output    | 9				|
| 20x4 LCD Screen       | --      |
| 20x4 LCD Screen       | --      |
| LCD Backlight Red     | 11      |
| LCD Backlight Green   | 10      |
| LCD Backlight Blue    | 6      	|

## Components & costs

List of example components required and estimated costs. 

- [Arduino](https://store-usa.arduino.cc/collections/boards) (Examples built using Nano and Uno)
- [RGB backlight negative LCD 20x4](https://www.adafruit.com/product/498#technical-details)
- PWM 110/220 Dimmer: [Option A](https://www.amazon.com/gp/product/B0BC297G4B/ref=ppx_yo_dt_b_asin_title_o08_s00?ie=UTF8&th=1), [Option B](https://www.amazon.com/gp/product/B06Y1DT1WP/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1)
- [Rotary Encoder](https://www.adafruit.com/product/377)
- [Potentiometer](https://www.adafruit.com/product/1789)
- [Button](https://www.adafruit.com/product/559) (Qty 2)
- [110v AC to 5V DC Converter](https://www.amazon.com/gp/product/B07YXN8J6R/ref=ppx_yo_dt_b_asin_title_o06_s02?ie=UTF8&th=1)

Expect that the total cost for these items will fall between $75-95 USD. You can certainly source far more inexpensive components and achieve the same results. You will also need tools and shop supplies that you may or may not already own. Electrical tools (cutters, strippers, multimeter), soldering iron, wire, connectors, resistors, and shrink tube will be necessary to have on hand.

## Libraries & Dependencies

- [Arduino](https://docs.arduino.cc/)
- [Adafruit_LiquidCrystal](https://www.arduinolibraries.info/libraries/adafruit-liquid-crystal)
- [Arduino Timer](https://www.arduinolibraries.info/libraries/arduino-timer)
- [Basic Encoder](https://www.arduinolibraries.info/libraries/basic-encoder)

In the `.vscode` directory, you'll find some extensions that may be helpful if you are developing in VS Code. Further configuration specific to your OS will be required.

## Documentation & demos

[Visit the wiki in Github](https://github.com/jmjackson6880/arduino-gaggia-classic-pro/wiki/Documentation-&-Demos) for more documentation, photos, and demos.

### Arduino Circuit

WIKI - TO DO