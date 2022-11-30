# Arduino Project: Gaggia Classic Pro Mod

Just another gratuitous project for technically-minded coffee snobs and giant nerds.

This project builds off of the amazing (https://www.shadesofcoffee.co.uk/)[Shades of Coffee] mods for the Gaggia Classic Pro. Specifically, the addition of an (https://www.shadesofcoffee.co.uk/post-2018/gaggia-classic-pro2019-pid-kit---132din-single-display)[upgraded temperature sensor and PID] for precise control of brew and steam temperatures, as well as the (https://www.shadesofcoffee.co.uk/post-2018/gaggia-classic---flow-control-dimmer-kit)[flow control dimmer kit]. This mod takes the dimmer kit a few steps further by introducing an Arduino as a microcontroller along with a series of sensors and output devices. Together, these components provide the functionality necessary for the user to select and load a pressure profile, set a shot time, and push the "start" button. Or, the user has the option to operate the machine manually using a flow control knob.

There are other similar projects out there. (https://www.google.com/search?rlz=1C5CHFA_enUS841US841&sxsrf=ALiCzsbGp50YCr51Wm168XTbH1bHXEwS2Q:1669829448061&q=gaggiuino&spell=1&sa=X&ved=2ahUKEwiw7di4t9b7AhU2FFkFHTD4DLwQBSgAegQIBRAB&biw=1902&bih=1373&dpr=1)[Google "gaggiuino"] for more inspiration.

## Warnings & Disclaimers

This project requires a strong baseline knowledge of programming, electricity, and microcontrollers. Implementing these modifications requires working with dangerous mainline voltages at 110/220 volts, and will include the modification of wiring. Destroyed electronics, shocks, fires, injury and death are possible. Make sure you know what you're doing.

## Components

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

## Circuit

## Libraries & Dependencies

- Arduino
- Ardunio Timer
- Basic Encoder
