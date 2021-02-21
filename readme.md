Making a Pomodoro timer on an Adafruit Circuit Playground using freeRTOS.

## FreeRTOS on Arduino
Found an example of FreeRTOS ported to Arduino here: https://feilipu.me/2015/11/24/arduino_freertos/ :heart:

In Sketch > Include Libraries > Manage Libraries..., search for FreeRTOS and install the first result.

Then include in the code: #include <Arduino_FreeRTOS.h>

I started with the blink example.

Task notifications are a way for tasks to pass data between themselves. This means we don't need to define globals. A reference is here: https://www.freertos.org/RTOS_Task_Notification_As_Event_Group.html

## Pomodoro Timer
- Work for 25 minutes
- Take a 5 minute break

This could be broken into tasks:
- Count minutes
- Check for state change between work and break
- Update the lights

## Bugs
- Code doesn't run unless serial console open, this seemed to affect the example as well
  - This has been worked around for now. The comment in setup() describes the problem

