#include <Arduino_FreeRTOS.h>
#include <Adafruit_CircuitPlayground.h>

#define SECONDS_OF_WORK (60)
#define SECONDS_OF_BREAK (30)
#define NOTIFICATION_PERCENTAGE (10)
#define PERCENT_MAX (100)
#define LED_COUNT (10)
#define ULONG_MAX (0xFFFFFFFF)
#define COLOUR_MAX (255)

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

unsigned long ulPowerOfTwo(unsigned short power);

void taskCountTime(void* pvParameters);
void taskState(void* pvParameters);
void taskUpdateLights(void* pvParameters);

static TaskHandle_t stateTaskHandle        = NULL;
static TaskHandle_t updateLightsTaskHandle = NULL;

void setup()
{
  Serial.begin(9600); /* BUG: unless serial is started and a console opened, the code hangs */
  while (!Serial)
  {
    ; /* wait for serial port to connect */
  }
  
  xTaskCreate(
    taskCountTime
    ,  "CountTime"
    ,  128  /* Stack size */
    ,  NULL
    ,  3  /* Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest. */
    ,  NULL );
    
  xTaskCreate(
    taskState
    ,  "State"
    ,  128  /* Stack size */
    ,  NULL
    ,  2  /* Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest. */
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
unsigned long ulPowerOfTwo(unsigned short power)
{
  unsigned long result = 2;
  for (unsigned short i = power; i > 1; i--)
  {
    result *= 2;
  }
  return result;
}
/*
 * Tasks
 */
void taskCountTime(void* pvParameters)
{
  for (;;)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("count");
    xTaskNotify(stateTaskHandle, NOTIFICATION_POMODORO_TICK, eSetBits);
  }
}

void taskState(void* pvParameters)
{
    
  teState pomodoroState = eStateWork;
  unsigned short tickCount = 0;
  unsigned int stateTime = SECONDS_OF_WORK;
  unsigned short previousNotificationPercentage = 0;

  for (;;)
  {
    BaseType_t xResult;
    unsigned long ulNotifiedValue = 0;
    unsigned long lightCommand = 0;
    unsigned short percentageTime = 0;
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
      percentageTime = (PERCENT_MAX * tickCount) / stateTime;
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
  unsigned long ulNotifiedValue = 0;
  unsigned long ulLed = 0;

  for (;;)
  {
    unsigned short red = 0;
    unsigned short green = 0;
    unsigned short blue = 0;
    xResult = xTaskNotifyWait( pdFALSE,    /* Don't clear bits on entry. */
                     ULONG_MAX,            /* Clear all bits on exit. */
                     &ulNotifiedValue,     /* Stores the notified value. */
                     portMAX_DELAY );

    Serial.println("light");

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
    for (unsigned short i = 0; i < LED_COUNT; i++)
    {
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

