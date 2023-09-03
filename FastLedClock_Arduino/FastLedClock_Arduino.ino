#include<FastLED.h>

#define NUM_LEDS 28
#define LED_PIN 3

CRGB leds[NUM_LEDS];
uint8_t hue = 0;
unsigned long milSec = millis();
int number = 0000;
int hrTen, hrOne, minTen, minOne;

// void getNum(int num){
//   minOne = num%10;
//   minTen = (num%100)/10;
//   hrOne = (num%1000)/100;
//   hrTen = (num%10000)/1000;
// }

void setHue(){
  for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue + (i * 10), 255, 255);
  }
  EVERY_N_MILLISECONDS(5){
    hue++;
  }
}
void setDisplay(int num){// set black pixels to show characters
  int zeroIndex = 0;// dispNum should be zero index
  int mod = 10000, div =1000;
  for(int i=0;i<4;i++){
    int N = (num%mod)/div;
    switch(N){
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
    zeroIndex += 7;
    mod /= 10;
    div /= 10;
  }
}

void setup() {
  delay(3000);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(10);
}

void loop() {
  if(number > 9999)
    number = 0;
  EVERY_N_MILLISECONDS(10){
    number++;
  }

  setHue();
  setDisplay(number);
  FastLED.show();
}

