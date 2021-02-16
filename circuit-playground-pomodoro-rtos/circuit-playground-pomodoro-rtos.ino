#include <Adafruit_CircuitPlayground.h>

#define LED_COUNT (10)

static int count = 0;

void setup() {
  CircuitPlayground.begin();
}

void loop() {
  count++;
  if (count >= LED_COUNT) {
    count = 0;
  }
  CircuitPlayground.clearPixels();
  CircuitPlayground.setPixelColor(count, 255,   0,   0);
  delay(1000);
}

