#include <FastLED.h>
#include "uRTCLib.h"

#define NUM_LEDS 29
#define LED_PIN 2
#define BRIGHTNESS 128
#define RTC_ADDRESS 0x68

CRGB leds[NUM_LEDS];
uint8_t hue = 0;
uRTCLib rtc(RTC_ADDRESS);

int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;

// Segment mapping
const int SEGMENT_MAP[7] = { 1, 2, 3, 4, 5, 6, 0 };
const uint8_t DIGIT_MAP[10][7] = {
  { 1, 1, 1, 1, 1, 0, 1 },  // 0
  { 0, 1, 1, 0, 0, 0, 0 },  // 1
  { 1, 1, 0, 1, 1, 1, 0 },  // 2
  { 1, 1, 1, 1, 0, 1, 0 },  // 3
  { 0, 1, 1, 0, 0, 1, 1 },  // 4
  { 1, 0, 1, 1, 0, 1, 1 },  // 5
  { 1, 0, 1, 1, 1, 1, 1 },  // 6
  { 1, 1, 1, 0, 0, 0, 0 },  // 7
  { 1, 1, 1, 1, 1, 1, 1 },  // 8
  { 1, 1, 1, 1, 0, 1, 1 }   // 9
};

void setup() {
  Serial.begin(9600);
  Serial.println("Serial OK");

  setupLEDs();
  setupRTC();
}

void loop() {
  updateTime();
  updateLEDs();
  displayTime();
  FastLED.show();
  delay(5);
}

void setupLEDs() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
}

void setupRTC() {
  URTCLIB_WIRE.begin();
  rtc.refresh();

  if (rtc.year() < 2024) {
    const char* dateStr = __DATE__;  // Format: "Mmm dd yyyy"
    const char* timeStr = __TIME__;  // Format: "hh:mm:ss"

    char monthStr[4];
    int day, year, hour, minute, second;

    sscanf(dateStr, "%s %d %d", monthStr, &day, &year);
    sscanf(timeStr, "%d:%d:%d", &hour, &minute, &second);

    int month = monthStringToNumber(monthStr);
    int dayOfWeek = 1;
    rtc.set(second, minute, hour, dayOfWeek, day, month, year - 2000);
    Serial.println("RTC time set to compilation time");
  } else {
    Serial.println("RTC time is valid, no need to set.");
  }
}

void updateTime() {
  rtc.refresh();
  currentHour = rtc.hour();
  currentMinute = rtc.minute();
  currentSecond = rtc.second();

  Serial.print(currentHour);
  Serial.print(':');
  Serial.print(currentMinute);
  Serial.print(':');
  Serial.println(currentSecond);
}

void updateLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue + (i * 2), 255, 255);
  }
  hue++;
}

void displayTime() {
  setDisplayNumber(0, currentHour / 10);
  setDisplayNumber(1, currentHour % 10);
  setDisplayNumber(2, currentMinute / 10);
  setDisplayNumber(3, currentMinute % 10);

  // blink
  if (currentSecond % 2 == 0) {
    leds[NUM_LEDS - 1] = CHSV(0, 0, 0);
  }
}

void setDisplayNumber(int index, int number) {
  if (index < 0 || index >= 4 || number < 0 || number > 9) {
    return;
  }

  int zeroIndex = index * 7;

  // set seg acc to the digit map
  for (int i = 0; i < 7; i++) {
    if (!DIGIT_MAP[number][i]) {
      leds[zeroIndex + SEGMENT_MAP[i]] = CHSV(0, 0, 0);
    }
  }
}

int monthStringToNumber(const char* monthStr) {
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  for (int i = 0; i < 12; i++) {
    if (strcmp(monthStr, months[i]) == 0) {
      return i + 1;
    }
  }
  return 0;
}
