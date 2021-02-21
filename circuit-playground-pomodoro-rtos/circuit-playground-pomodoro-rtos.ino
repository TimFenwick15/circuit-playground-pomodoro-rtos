#include <Arduino_FreeRTOS.h>
#include <Adafruit_CircuitPlayground.h>

#define SECONDS_OF_WORK (60)
#define SECONDS_OF_BREAK (30)
#define NOTIFICATION_PERCENTAGE (10)
#define PERCENT_MAX (100)
#define LED_COUNT (10)
#define ULONG_MAX (0xFFFFFFFF)
#define COLOUR_MAX (255)
#define SERIAL_WAIT (200)

#define NOTIFICATION_POMODORO_TICK   (0x01)
#define NOTIFICATION_POMODORO_BUTTON (0x02)

#define NOTIFICATION_LIGHT_0         (0x0001)
#define NOTIFICATION_LIGHT_1         (0x0002)
#define NOTIFICATION_LIGHT_2         (0x0004)
#define NOTIFICATION_LIGHT_3         (0x0008)
#define NOTIFICATION_LIGHT_4         (0x0010)
#define NOTIFICATION_LIGHT_5         (0x0020)
#define NOTIFICATION_LIGHT_6         (0x0040)
#define NOTIFICATION_LIGHT_7         (0x0080)
#define NOTIFICATION_LIGHT_8         (0x0100)
#define NOTIFICATION_LIGHT_9         (0x0200)
#define NOTIFICATION_LIGHT_R         (0x0400)
#define NOTIFICATION_LIGHT_G         (0x0800)
#define NOTIFICATION_LIGHT_B         (0x1000)

typedef enum
{
  eStateWork = 0,
  eStateBreak
} teState;

uint32_t ulPowerOfTwo(uint8_t power);

void taskPollButtons(void* pvParameters);
void taskCountTime(void* pvParameters);
void taskState(void* pvParameters);
void taskUpdateLights(void* pvParameters);

static TaskHandle_t stateTaskHandle        = NULL;
static TaskHandle_t updateLightsTaskHandle = NULL;

void setup()
{
  /* If the following block is left out, the code will run when the board is supplied 5V.
   * If the board is connected to a PC, the code will hang. Waiting for (Serial) and opening a console makes it work.
   * Waiting for serial up to a maximum time as below makes the code run whether supplied 5V or connected to a PC.
   * This adds 2.5s to the start up time, but this demo application doens't require fast boot time.
   * If you open a serial console, the code hangs again.
   * A better way would be to detect if we're connected to a PC, and then setup serial. Else just set up the tasks.
   * Or, we could find what in the Arduino library is causing this.
   * 
   * Incidentally, if (!Serial) works because of "Serial_::operator bool()" defined in ArduinoCore-avr/cores/arduino/CDC.cpp
   * from https://github.com/arduino/ArduinoCore-avr
   */
  uint8_t count = 0;
  while (!Serial)
  {
    delay(10);
    count++;
    if (count >= SERIAL_WAIT)
    {
      break;
    }
  }

  xTaskCreate(
    taskPollButtons
    ,  "PollButtons"
    ,  128  /* Stack size */
    ,  NULL
    ,  3  /* Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest. */
    ,  NULL );
  
  xTaskCreate(
    taskCountTime
    ,  "CountTime"
    ,  128  /* Stack size */
    ,  NULL
    ,  2  /* Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest. */
    ,  NULL );
    
  xTaskCreate(
    taskState
    ,  "State"
    ,  128  /* Stack size */
    ,  NULL
    ,  0  /* Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest. */
    ,  &stateTaskHandle );

  xTaskCreate(
    taskUpdateLights
    ,  "UpdateLights"
    ,  128
    ,  NULL
    ,  0
    ,  &updateLightsTaskHandle );

  CircuitPlayground.begin();
}

void loop()
{
}

/*
 * Helper
 */
uint32_t ulPowerOfTwo(uint8_t power)
{
  uint32_t result = 2;
  for (uint8_t i = power; i > 1; i--)
  {
    result *= 2;
  }
  return result;
}

/*
 * Tasks
 */
void taskPollButtons(void* pvParameters)
{
  bool buttonState = false;
  for (;;)
  {
    vTaskDelay(20 / portTICK_PERIOD_MS);
    if (CircuitPlayground.leftButton() || CircuitPlayground.rightButton())
    {
      /* Debounce - we want a rising edge of a button press only */
      if (false == buttonState)
      {
        buttonState = true;
        xTaskNotify(stateTaskHandle, NOTIFICATION_POMODORO_BUTTON, eSetBits);
        vTaskDelay(100 / portTICK_PERIOD_MS); /* In case the signal bounces on button push, reject further signals for 100ms */
      }
    }
    else
    {
      buttonState = false;
    }
  }
  
}
 
void taskCountTime(void* pvParameters)
{
  for (;;)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    xTaskNotify(stateTaskHandle, NOTIFICATION_POMODORO_TICK, eSetBits);
  }
}

void taskState(void* pvParameters)
{
    
  teState pomodoroState = eStateWork;
  uint16_t tickCount = 0;
  uint16_t stateTime = SECONDS_OF_WORK;
  uint8_t previousNotificationPercentage = 0;

  for (;;)
  {
    BaseType_t xResult;
    uint32_t ulNotifiedValue = 0;
    uint32_t lightCommand = 0;
    uint8_t percentageTime = 0;
    bool changeLight = false;
    bool changeState = false;
    xResult = xTaskNotifyWait( pdFALSE, /* Don't clear bits on entry. */
                 ULONG_MAX,             /* Clear all bits on exit. */
                 &ulNotifiedValue,      /* Stores the notified value. */
                 portMAX_DELAY );

    if (ulNotifiedValue & NOTIFICATION_POMODORO_BUTTON)
    {
      changeState = true;
    }
    else if (ulNotifiedValue & NOTIFICATION_POMODORO_TICK)
    {
      tickCount++;
      percentageTime = (uint8_t)((PERCENT_MAX * tickCount) / stateTime);
      if (percentageTime >= 110)
      {
        changeState = true;
      }
      else if (((percentageTime % NOTIFICATION_PERCENTAGE) == 0)
              && (percentageTime != previousNotificationPercentage))
      {
        /**** Then rewrite light function, then implement button poll ***/

        /*
         * lightCount = 1, command = 0x1
         * lightCount = 2, command = 0x3
         * lightCount = 3, command = 0x7. (2 ** lightCount) - 1
         */
        
        unsigned short lightCount = percentageTime / NOTIFICATION_PERCENTAGE;
        lightCommand |= ulPowerOfTwo(lightCount) - 1;
        previousNotificationPercentage = percentageTime;

        if (eStateWork == pomodoroState)
        {
          lightCommand |= NOTIFICATION_LIGHT_R;
        }
        else
        {
          lightCommand |= NOTIFICATION_LIGHT_G;
        }
        
        changeLight = true;  
      }
      else
      {
        /* Completeness */
      }
    }
    else
    {
      /* Completeness */
    }

    /* Change state */
    if (true == changeState)
    {
      tickCount = 0;
      previousNotificationPercentage = 0;
      
      if (eStateWork == pomodoroState)
      {
        pomodoroState = eStateBreak;
        stateTime = SECONDS_OF_BREAK;
      }
      else
      {
        pomodoroState = eStateWork;
        stateTime = SECONDS_OF_WORK;
      }
    }

    /* Send on notification */
    if ((true == changeState) || (true == changeLight))
    {
      xTaskNotify(updateLightsTaskHandle, lightCommand, eSetBits);
    }
  }
}

void taskUpdateLights(void* pvParameters)
{
  BaseType_t xResult;
  uint32_t ulNotifiedValue = 0;
  uint32_t ulLed = 0;

  for (;;)
  {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    xResult = xTaskNotifyWait( pdFALSE,    /* Don't clear bits on entry. */
                     ULONG_MAX,            /* Clear all bits on exit. */
                     &ulNotifiedValue,     /* Stores the notified value. */
                     portMAX_DELAY );

    if (ulNotifiedValue & NOTIFICATION_LIGHT_R)
    {
      red = COLOUR_MAX;
    }
    if (ulNotifiedValue & NOTIFICATION_LIGHT_G)
    {
      green = COLOUR_MAX;
    }
    if (ulNotifiedValue & NOTIFICATION_LIGHT_B)
    {
      blue = COLOUR_MAX;
    }
    for (uint8_t i = 0; i < LED_COUNT; i++)
    {
      /* Check each numbered light command bit, if this bit is set, turn on this light */
      if (ulNotifiedValue & (1 << i))
      {
        CircuitPlayground.setPixelColor(i, red, green, blue);  
      }
      else
      {
        CircuitPlayground.setPixelColor(i, 0, 0, 0);  
      }
    }
  }
}

