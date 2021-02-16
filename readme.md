Making a Pomodoro timer on an Adafruit Circuit Playground using freeRTOS.

##FreeRTOS on Arduino
Found an example of FreeRTOS ported to Arduino here: https://feilipu.me/2015/11/24/arduino_freertos/ :heart:

In Sketch > Include Libraries > Manage Libraries..., search for FreeRTOS and install the first result.

I addapted the blink example.

##Pomodoro Timer
- Work for 25 minutes
- Take a 5 minute break

This could be broken into tasks:
- Count minutes
- Check for state change between work and brake
- Update the lights

