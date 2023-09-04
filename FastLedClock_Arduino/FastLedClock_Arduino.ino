#include<FastLED.h>

#define NUM_LEDS 28
#define LED_PIN 3
#define BUZZER_PIN 4
CRGB leds[NUM_LEDS];
uint8_t hue = 0;
unsigned long milSec = millis();
int number = 0000;
int hrTen, hrOne, minTen, minOne, temperature = 20 humidity = 80;
bool toRing = false;

void setHue(){
  for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue + (i * 10), 255, 255);
  }
  EVERY_N_MILLISECONDS(15){
    hue++;
  }
}
void setDisplayNumber(int num){// set black pixels to show characters
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



/*
void setAlarm(int minuts){
  if(toRing){
    EVERY_N_MINUTES(minuts){
      playBuzzer();
      toRing = false;
    }
  }
}

void playBuzzer(){
  while (true) {
    EVERY_N_MILLISECONDS(500){
      digitalWrite(BUZZER_PIN, HIGH);
    }
    EVERY_N_MILLISECONDS(500){
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
}
*/

void setTemp(){
  //get temp
  //temperature = ihbkknknkj(IDK);
  setDisplayNumber(temperature*100);
  leds[14 + 3] = CHSV(0, 0, 0);// degree symbol
  leds[14 + 4] = CHSV(0, 0, 0);
  leds[14 + 5] = CHSV(0, 0, 0);
  leds[21 + 2] = CHSV(0, 0, 0);// C
  leds[21 + 3] = CHSV(0, 0, 0);
}

void setHumi(){
  //get humi
  //humidity = ihbkknknkj(IDK);
  setDisplayNumber(humidity*100);
  leds[14 + 0] = CHSV(0, 0, 0);// - symbol
  leds[14 + 1] = CHSV(0, 0, 0);
  leds[14 + 2] = CHSV(0, 0, 0);
  leds[14 + 3] = CHSV(0, 0, 0);
  leds[14 + 4] = CHSV(0, 0, 0);
  leds[14 + 5] = CHSV(0, 0, 0);

  leds[21 + 1] = CHSV(0, 0, 0);// H
  leds[21 + 4] = CHSV(0, 0, 0);
}

void SetDisplay(int menu){
  switch(menu){
    case 1:// time
      setDisplayNumber(number);
      break;
    case 2:// Temp
      setTemp();
      break;
    case 3:// Humidity
      setHumi();
      break;
    case 4:// Timer
      setTimer();
      break;
    default:
      break;
  }
}

void setup() {
  delay(3000);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(10);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  if(number > 9999)
    number = 0;
  EVERY_N_MILLISECONDS(10){
    number++;
  }

  setHue();
  SetDisplay(1);
  FastLED.show();
}
