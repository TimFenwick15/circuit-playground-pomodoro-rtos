#include <Arduino_FreeRTOS.h>
#include <Adafruit_CircuitPlayground.h>

#define SECONDS_OF_WORK (60)
#define SECONDS_OF_BREAK (30)
#define NOTIFICATION_PERCENTAGE (10)
#define PERCENT_MAX (100)
#define LED_COUNT (10)
#define ULONG_MAX (0xFFFFFFFF)

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

typedef enum {
  eStateWork = 0,
  eStateBreak
} teState;
/*
typedef enum {
  eColourRed = 0,
  eColourGreen,
  eColourBlue
} teColour;*/

void taskCountTime(void* pvParameters);
void taskState(void* pvParameters);
//void taskClearLights(void* pvParameters);
void taskUpdateLights(void* pvParameters);

static TaskHandle_t stateTaskHandle        = NULL;
//static TaskHandle_t clearLightsTaskHandle  = NULL;
static TaskHandle_t updateLightsTaskHandle = NULL;

//static teColour eColour = eColourRed;

void setup() {
  Serial.begin(9600); /* BUG: unless serial is started and a console opened, the code hangs */
  while (!Serial) {
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
    ,  stateTaskHandle );

  /*xTaskCreate(
    taskClearLights
    ,  "ClearLights"
    ,  128
    ,  NULL
    ,  1
    ,  &clearLightsTaskHandle );
*/
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

void taskCountTime(void* pvParameters) {
  //unsigned int count = 0;
  //teState eState = eStateWork;
  //unsigned int percentageTime = 0;
  for (;;)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("count");
    xTaskNotify(updateLightsTaskHandle, NOTIFICATION_POMODORO_TICK, eSetBits);
#if 0
    switch (eState) {
      case eStateWork:
        percentageTime = (PERCENT_MAX * count) / SECONDS_OF_WORK;
        break;
      case eStateBreak:
        percentageTime = (PERCENT_MAX * count) / SECONDS_OF_BREAK;
        break;
    }

    /* Send a notification, 10 percentage interval */
    if ((percentageTime % NOTIFICATION_PERCENTAGE) == 0) {
      xTaskNotify(updateLightsTaskHandle, percentageTime, eSetValueWithOverwrite);
    }
    
    /* Change state. Send a notification, state has changed from work to break */
    if (percentageTime >= PERCENT_MAX) {
      count = 0;
      if (eStateWork == eState) {
        eState = eStateBreak;
      }
      else if (eState == eStateBreak) {
        eState = eStateWork;
      }
      else {
        /* completeness */
      }
      
      xTaskNotify(clearLightsTaskHandle, eState, eSetValueWithOverwrite);
    }
    
    count++;
#endif
  }
}

void taskState(void* pvParameters) {
    
  teState pomodoroState = eStateWork;
  unsigned short tickCount = 0;
  unsigned int stateTime = 0;

  for (;;) {
    BaseType_t xResult;
    unsigned long ulNotifiedValue = 0;
    unsigned long lightCommand = 0;
    unsigned short percentageTime = 0;
    bool changeLight = false;
    bool changeState = false;
    stateTime = 
    xResult = xTaskNotifyWait( pdFALSE, /* Don't clear bits on entry. */
                 ULONG_MAX,             /* Clear all bits on exit. */
                 &ulNotifiedValue,      /* Stores the notified value. */
                 portMAX_DELAY );

                 
    if (ulNotifiedValue & NOTIFICATION_POMODORO_TICK) {
      tickCount++;
      percentageTime = (PERCENT_MAX * count) / stateTime;
      if (percentageTime >= PERCENT_MAX) {
        changeState = true;  
      }

      /**** TBC. We need to know what the last update notification was so we can set the new update notification when appropriate. Then rewrite light function, then implement button poll ***/
      changeLight = true;
    }
    if (ulNotifiedValue & NOTIFICATION_POMODORO_BUTTON) {
      changeState = true;
    }

    /* Change state */
    if (true == changeState) {
      if (eStateWork == pomodoroState) {
        pomodoroState = eStateBreak;
        stateTime = SECONDS_OF_BREAK;
        lightCommand |= NOTIFICATION_LIGHT_G;
      }
      else {
        pomodoroState = eStateWork;
        stateTime = SECONDS_OF_WORK;
        lightCommand |= NOTIFICATION_LIGHT_R;
      }
    }

    /* Send on notification */
    if ((true == changeState) || (true == changeLight)) {
      xTaskNotify(updateLightsTaskHandle, lightCommand, eSetBits);
    }
  }
}

void taskUpdateLights(void* pvParameters) {
  BaseType_t xResult;
  unsigned long ulNotifiedValue = 0;
  unsigned long ulLed = 0;

  for (;;) {
    xResult = xTaskNotifyWait( pdFALSE,    /* Don't clear bits on entry. */
                     ULONG_MAX,            /* Clear all bits on exit. */
                     &ulNotifiedValue,     /* Stores the notified value. */
                     portMAX_DELAY );

    Serial.print("light ");
    Serial.println(ulNotifiedValue);
    

    ulLed = ulNotifiedValue / NOTIFICATION_PERCENTAGE;
    
    /* light up based on percentage completed */
    switch (eColour) {
      case eColourRed:
        CircuitPlayground.setPixelColor(ulLed, 255, 0, 0);  
        break;
      case eColourGreen:
        CircuitPlayground.setPixelColor(ulLed, 0, 255, 0);  
        break;
      case eColourBlue:
        CircuitPlayground.setPixelColor(ulLed, 0, 0, 255);  
        break;
    }
  }
}
#if 0
void taskClearLights(void* pvParameters) {
  BaseType_t xResult;
  unsigned long ulNotifiedValue = 0;
  for (;;) {
    xResult = xTaskNotifyWait( pdFALSE, /* Don't clear bits on entry. */
                 ULONG_MAX,             /* Clear all bits on exit. */
                 &ulNotifiedValue,      /* Stores the notified value. */
                 portMAX_DELAY );

    Serial.print("clear ");
    Serial.println(ulNotifiedValue);

    if (eStateWork == ulNotifiedValue) {
      eColour = eColourRed;
    }
    else if (eStateBreak == ulNotifiedValue) {
      eColour = eColourGreen;
    }
    else {
      /* completeness */
    }

    CircuitPlayground.clearPixels();
  }
}
#endif
