#include<FastLED.h>

#define NUM_LEDS 7
#define LED_PIN 3

CRGB leds[NUM_LEDS];
uint8_t hue = 0;
void setHue(){
  //
}
void setDisplay(int dispNum, int num){// set black pixels to show characters
  int zeroIndex = dispNum*NUM_LEDS; // dispNum should be zero index
  switch(num){
    case 0:
      leds[zeroIndex + 6] = CHSV(0, 0, 0);
      break;
    case 1:
      leds[zeroIndex] = CHSV(0, 0, 0);
      leds[zeroIndex + 1] = CHSV(0, 0, 0);
      leds[zeroIndex + 4] = CHSV(0, 0, 0);
      leds[zeroIndex + 5] = CHSV(0, 0, 0);
      leds[zeroIndex + 6] = CHSV(0, 0, 0);
      break;
    case 2:
      leds[zeroIndex] = CHSV(0, 0, 0);
      leds[zeroIndex + 3] = CHSV(0, 0, 0);
      break;
    case 3:
      leds[zeroIndex] = CHSV(0, 0, 0);
      leds[zeroIndex + 5] = CHSV(0, 0, 0);
      break;
    case 4:
      leds[zeroIndex + 1] = CHSV(0, 0, 0);
      leds[zeroIndex + 4] = CHSV(0, 0, 0);
      leds[zeroIndex + 5] = CHSV(0, 0, 0);
      break;
    case 5:
      leds[zeroIndex + 2] = CHSV(0, 0, 0);
      leds[zeroIndex + 5] = CHSV(0, 0, 0);
      break;
    case 6:
      leds[zeroIndex + 2] = CHSV(0, 0, 0);
      break;
    case 7:
      leds[zeroIndex] = CHSV(0, 0, 0);
      leds[zeroIndex + 4] = CHSV(0, 0, 0);
      leds[zeroIndex + 5] = CHSV(0, 0, 0);
      leds[zeroIndex + 6] = CHSV(0, 0, 0);
      break;
    case 8:
      break;
    case 9:
      leds[zeroIndex + 5] = CHSV(0, 0, 0);
      break;
    default:
      break;
  }
}

void setup() {
  delay(3000);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30);

}

void loop() {
  int number = 5;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue + (i * 10), 255, 255);
  }
  EVERY_N_MILLISECONDS(15){
    hue++;
  }
  // EVERY_N_SECONDS(1){
  //   number++;
  // }
  setDisplay(0, number);
  FastLED.show();
}

// #include <FastLED.h>
// #define NUM_LEDS  18
// #define LED_PIN   3
// CRGB leds[NUM_LEDS];
// uint8_t hue = 0;

// void setup() {
//   FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
//   FastLED.setBrightness(10);
// }

// void loop() {
//   for (int i = 0; i < NUM_LEDS; i++) {
//     //leds[i] = CHSV(hue, 255, 255);
//     leds[i] = CHSV(hue + (i * 10), 255, 255);
//   }
//   EVERY_N_MILLISECONDS(15){
//     hue++;
//   }
//   FastLED.show();
// }
