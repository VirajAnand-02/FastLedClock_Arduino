// main.ino
#include <Arduino.h>
#include <FastLED.h>
#include "LEDControl.h"
#include "ClockUtils.h"

// --- Configuration ---
constexpr int NUM_LEDS = 29;
constexpr int LED_PIN = 2;
constexpr uint8_t BRIGHTNESS = 255;
constexpr bool USE_12H = true;

// Automatically dim the display at night (22:00-07:59).
constexpr bool NIGHT_DIMMING = true;
constexpr uint8_t NIGHT_BRIGHTNESS = 127;

// Frame interval for the animation loop (~60 fps, non-blocking).
constexpr uint16_t FRAME_MS = 16;

// --- Available Animations ---
const AnimationFunction animations[] = {
  animateRainbow,
  animateOcean,
  animateLava,
  animateConfetti,
  animateBreathe
};
constexpr int NUM_ANIMATIONS = sizeof(animations) / sizeof(animations[0]);
int animationIndex = 0;

// --- Global Objects ---
LEDControl<NUM_LEDS, LED_PIN> ledControl(BRIGHTNESS, animations[0]);

int lastSecond = -1;

void handleSerial();
void cycleAnimation();
void applyBrightnessForHour(uint8_t hour);

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  Serial.println(F("Advanced Animation RGB Clock"));
  Serial.println(F("Commands: n=next animation  +/-=brightness  f=toggle 12/24h  T<HHMMSS>=set time  s=separator on/off"));
  Wire.begin();
  ledControl.begin();

  if (!setupRTC()) {
    Serial.println(F("RTC unavailable - running on software clock."));
    seedSoftwareClock(12, 0, 0); // start at 12:00:00
  }
}

// --- Main Loop ---
void loop() {
  // Non-blocking frame timing for a smooth animation.
  static uint32_t lastFrame = 0;
  uint32_t now = millis();

  handleSerial();

  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;

    ledControl.updateAnimation();

    uint8_t hour, minute, second;
    readClock(hour, minute, second);
    applyBrightnessForHour(hour);

    // Only update the time digits when the second actually changes.
    if (second != lastSecond) {
      lastSecond = second;

      ledControl.setHourMinute(hour, minute, ledControl.is12hMode());

      if (ledControl.is12hMode()) {
        ledControl.setAmPm(hour >= 12);
      } else {
        ledControl.clearAmPm();
      }
    }

    // Render: combines the smooth animation with the mask of the time digits.
    ledControl.show();
  }
}

// --- Night Dimming ---
void applyBrightnessForHour(uint8_t hour) {
  if (!NIGHT_DIMMING) return;
  bool night = (hour >= 22) || (hour < 8);
  uint8_t current = ledControl.getBrightness();
  uint8_t target = night ? ((NIGHT_BRIGHTNESS < current) ? NIGHT_BRIGHTNESS : current)
                         : BRIGHTNESS;
  static uint8_t applied = 255;
  if (applied != target) {
    applied = target;
    ledControl.setBrightness(target);
  }
}

// --- Serial Commands ---
void handleSerial() {
  while (Serial.available() > 0) {
    int c = Serial.read();
    switch (c) {
      case 'n':
        cycleAnimation();
        break;
      // NOTE: every case that declares a variable MUST be wrapped in braces.
      // An unbraced declaration makes the jump to every later case label
      // undefined behaviour; -Os then silently deletes those labels.
      case '+':
      case '=': {
        int b = (int)ledControl.getBrightness() + 16;
        ledControl.setBrightness((uint8_t)(b > 255 ? 255 : b));
        Serial.print(F("Brightness: "));
        Serial.println(ledControl.getBrightness());
        break;
      }
      case '-':
      case '_': {
        int b = (int)ledControl.getBrightness() - 16;
        ledControl.setBrightness((uint8_t)(b < 0 ? 0 : b));
        Serial.print(F("Brightness: "));
        Serial.println(ledControl.getBrightness());
        break;
      }
      case 'f': {
        bool newMode = !ledControl.is12hMode();
        // Re-render current time in the new format immediately.
        uint8_t h, m, s;
        readClock(h, m, s);
        ledControl.setHourMinute(h, m, newMode);
        lastSecond = -1; // force digit refresh next frame
        Serial.println(newMode ? F("Format: 12h") : F("Format: 24h"));
        break;
      }
      case 'T': {
        // Set time: send "T" followed by 6 digits (HHMMSS), e.g. "T070000" = 7:00 AM.
        uint8_t digits[6];
        bool ok = true;
        for (int i = 0; i < 6 && ok; i++) {
          uint32_t start = millis();
          while (Serial.available() == 0) {
            if (millis() - start > 1000) { ok = false; break; }
          }
          if (ok) {
            int d = Serial.read();
            if (d < '0' || d > '9') ok = false;
            else digits[i] = d - '0';
          }
        }
        if (ok) {
          uint8_t h = digits[0] * 10 + digits[1];
          uint8_t m = digits[2] * 10 + digits[3];
          uint8_t s = digits[4] * 10 + digits[5];
          if (h <= 23 && m <= 59 && s <= 59) {
            if (!setClockTime(h, m, s)) {
              // Software clock was still seeded, so the display will follow;
              // the time just will not survive a power cycle.
              Serial.println(F("Warning: RTC write failed (software clock only)."));
            }
            Serial.print(F("Time set to "));
            if (h < 10) Serial.print('0');
            Serial.print(h);
            Serial.print(':');
            if (m < 10) Serial.print('0');
            Serial.print(m);
            Serial.print(':');
            if (s < 10) Serial.print('0');
            Serial.println(s);
          } else {
            Serial.println(F("Invalid time."));
          }
        } else {
          Serial.println(F("Usage: T<HHMMSS> e.g. T070000"));
        }
        lastSecond = -1;
        break;
      }
      case 's': {
        bool newState = !ledControl.isSeparatorEnabled();
        ledControl.setSeparatorEnabled(newState);
        lastSecond = -1; // force separator refresh next frame
        Serial.println(newState ? F("Separator: on") : F("Separator: off"));
        break;
      }
      default:
        break;
    }
  }
}

void cycleAnimation() {
  animationIndex = (animationIndex + 1) % NUM_ANIMATIONS;
  ledControl.setAnimation(animations[animationIndex]);
  Serial.print(F("Animation #"));
  Serial.println(animationIndex);
}
