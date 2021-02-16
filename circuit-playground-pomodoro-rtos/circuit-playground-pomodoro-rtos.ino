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

void taskCountTime(void* pvParameters);
void taskClearLights(void* pvParameters);
void taskUpdateLights(void* pvParameters);

static TaskHandle_t clearLightsTaskHandle = NULL;
static TaskHandle_t updateLightsTaskHandle = NULL;

void setup() {
  Serial.begin(9600);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
  xTaskCreate(
    taskCountTime
    ,  "CountTime"
    ,  128  // Stack size
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
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

void taskCountTime(void *pvParameters) {
  unsigned int count = 0;
  teState eState = eStateWork;
  for (;;)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("count");

    unsigned int percentageTime = (PERCENT_MAX * count) / SECONDS_OF_WORK;

    // Send a notification, 10 percentage interval
    if ((percentageTime % NOTIFICATION_PERCENTAGE) == 0) {
      xTaskNotify(updateLightsTaskHandle, percentageTime, eSetValueWithOverwrite);
    }

    count++;
    
    // Change state. Send a notification, state has changed from work to break
    if (percentageTime >= PERCENT_MAX) {
      count = 0;
      if (eState == eStateWork) {
        eState = eStateBreak;
      }
      else if (eState == eStateBreak) {
        eState = eStateWork;
      }
      else {
        // completeness
      }
      // send notification
      xTaskNotify(clearLightsTaskHandle, eState, eSetValueWithOverwrite);
    }
  }
}

void taskUpdateLights(void* pvParameters) {
  //const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 5000 );
  BaseType_t xResult;
  unsigned long ulNotifiedValue = 0;
  unsigned long ulLed = 0;

  for (;;) {
    // sleep until notified
    xResult = xTaskNotifyWait( pdFALSE,    /* Don't clear bits on entry. */
                     ULONG_MAX,            /* Clear all bits on exit. */
                     &ulNotifiedValue,     /* Stores the notified value. */
                     portMAX_DELAY );
    Serial.print("light ");
    Serial.println(ulNotifiedValue);

    ulLed = ulNotifiedValue / NOTIFICATION_PERCENTAGE;
    
    // light up based on percentage completed
    CircuitPlayground.setPixelColor(ulLed, 255,   0,   0);
  }
}

void taskClearLights(void* pvParameters) {
  //const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 5000 );
  BaseType_t xResult;
  unsigned long ulNotifiedValue = 0;
  for (;;) {
    // sleep until notified
    xResult = xTaskNotifyWait( pdFALSE,    /* Don't clear bits on entry. */
                 ULONG_MAX,            /* Clear all bits on exit. */
                 &ulNotifiedValue,     /* Stores the notified value. */
                 portMAX_DELAY );

    Serial.println("clear");
    
    // Turn off lights
    CircuitPlayground.clearPixels();
  }
}







/*
void TaskBlink(void *pvParameters)
{
  (void) pvParameters;

  for (;;) // A Task shall never return or exit.
  {
    CircuitPlayground.clearPixels();
    vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
    CircuitPlayground.setPixelColor(1, 255,   0,   0);
    vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
  }
}

void TaskAnalogRead(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  for (;;)
  {
    // read the input on analog pin 0:
    int sensorValue = analogRead(A0);
    // print out the value you read:
    Serial.println(sensorValue);
    vTaskDelay(1);  // one tick delay (15ms) in between reads for stability
  }
}*/



  /*xTaskCreate(
    TaskBlink
    ,  "Blink"
    ,  128  // Stack size
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    TaskAnalogRead
    ,  "AnalogRead"
    ,  128
    ,  NULL
    ,  1
    ,  NULL );*/



    
//void TaskBlink(void* pvParameters );
//void TaskAnalogRead(void* pvParameters );
