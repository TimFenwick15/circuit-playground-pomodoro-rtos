#include <Arduino_FreeRTOS.h>
#include <Adafruit_CircuitPlayground.h>

#define SECONDS_OF_WORK (60)
#define SECONDS_OF_BREAK (30)
#define NOTIFICATION_PERCENTAGE (10)
#define PERCENT_MAX (100)
#define LED_COUNT (10)
#define ULONG_MAX (0xFFFFFFFF)

typedef enum {
  eStateWork = 0,
  eStateBreak
} teState;

typedef enum {
  eColourRed = 0,
  eColourGreen,
  eColourBlue
} teColour;

void taskCountTime(void* pvParameters);
void taskClearLights(void* pvParameters);
void taskUpdateLights(void* pvParameters);

static TaskHandle_t clearLightsTaskHandle = NULL;
static TaskHandle_t updateLightsTaskHandle = NULL;

static teColour eColour = eColourRed;

void setup() {
  Serial.begin(9600);
  
  while (!Serial) {
    ; /* wait for serial port to connect */
  }
  xTaskCreate(
    taskCountTime
    ,  "CountTime"
    ,  128  /* Stack size */
    ,  NULL
    ,  2  /* Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest. */
    ,  NULL );

  xTaskCreate(
    taskClearLights
    ,  "ClearLights"
    ,  128
    ,  NULL
    ,  1
    ,  &clearLightsTaskHandle );

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
  unsigned int count = 0;
  teState eState = eStateWork;
  unsigned int percentageTime = 0;
  for (;;)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("count");

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

