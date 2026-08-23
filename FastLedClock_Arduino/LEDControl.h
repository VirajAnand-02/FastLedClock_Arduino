// LEDControl.h
#ifndef LEDCONTROL_H
#define LEDCONTROL_H

#include <Arduino.h>
#include <FastLED.h>
#include <stdint.h>

// --- Default Digit Maps ---
// Segment order: index 0..6 mapped onto physical LEDs via segMap.
static constexpr int defaultSegmentMap[7] = { 1, 2, 3, 4, 5, 6, 0 };
static constexpr uint8_t defaultDigitMap[10][7] = {
  {1,1,1,1,1,0,1}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,1,0}, // 2
  {1,1,1,1,0,1,0}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}  // 9
};

// Forward declaration of the LEDControl class for the function pointer type
template<int NUM_LEDS, int LED_PIN> class LEDControl;

// Define a type for our animation functions
using AnimationFunction = void (*)(CRGB* leds, int numLeds, uint8_t hue);

// --- Animation Implementations ---
// All animations paint the whole strip; the time digits are masked on top
// in render(), so digits appear as "holes" cut out of the animation.

// Animation 1: full-spectrum rainbow gradient sweeping across the strip
// (hue delta per LED kept low so the gradient looks stretched/smooth)
void animateRainbow(CRGB* leds, int numLeds, uint8_t hue) {
  for (int i = 0; i < numLeds; i++) {
    leds[i] = CHSV(hue + (uint8_t)(((int32_t)i * 80) / numLeds), 220, 255);
  }
}

// Animation 2: blue/green ocean waves
void animateOcean(CRGB* leds, int numLeds, uint8_t hue) {
  for (int i = 0; i < numLeds; i++) {
    uint8_t wave = sin8(hue + i * 9);
    leds[i] = CHSV(90 + (wave >> 3), 240, 128 + (wave >> 1));
  }
}

// Animation 3: warm lava glow using FastLED heat colors
void animateLava(CRGB* leds, int numLeds, uint8_t hue) {
  for (int i = 0; i < numLeds; i++) {
    uint8_t heat = sin8(sin8(hue * 2 + i * 12) + i * 5);
    CRGB c = ColorFromPalette(HeatColors_p, heat);
    leds[i] = c.nscale8(180);
  }
}

// Animation 4: confetti sparkles over a dark background
void animateConfetti(CRGB* leds, int numLeds, uint8_t hue) {
  fadeToBlackBy(leds, numLeds, 20);
  int pos = random16(numLeds);
  leds[pos] += CHSV(hue + random8(64), 200, 255);
}

// Animation 5: slow breathing single-color pulse
void animateBreathe(CRGB* leds, int numLeds, uint8_t hue) {
  uint8_t breath = beatsin8(15, 40, 255);
  for (int i = 0; i < numLeds; i++) {
    leds[i] = CHSV(hue, 190, breath);
  }
}

template<int NUM_LEDS, int LED_PIN>
class LEDControl {
public:
  static constexpr int NUM_DIGITS = 4;
  static constexpr int LEDS_PER_DIGIT = 7;
  static constexpr int SEPARATOR_LED = NUM_LEDS - 1;
  // Brightness (0-255) of the separator during its "off" phase so that
  // the AM/PM color stays readable at all times.
  static constexpr uint8_t SEPARATOR_GLOW = 25;
  // Pulse speed of the separator in beats per minute (~4 s per cycle).
  static constexpr uint8_t PULSE_BPM = 15;

  LEDControl(uint8_t brightness, AnimationFunction animation)
    : brightness(brightness),
      hue(0),
      currentAnimation(animation),
      isPM(false),
      use12hMode(true),
      separatorEnabled(true)
  {}

  void begin() {
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    // Safety: cap power draw at ~500 mA @ 5 V to protect USB ports.
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
    FastLED.setBrightness(brightness);
    FastLED.clear();
    FastLED.show();
  }

  void updateAnimation() {
    // Advance the hue every 3rd frame (~20 steps/s) for a slower sweep.
    static uint8_t frameCount = 0;
    if (++frameCount >= 3) {
      frameCount = 0;
      hue++;
    }
    if (currentAnimation) {
      currentAnimation(leds, NUM_LEDS, hue);
    }
  }

  void setAnimation(AnimationFunction newAnimation) {
    currentAnimation = newAnimation;
    FastLED.clear(); // avoid leftover frames from the previous animation
  }

  AnimationFunction getAnimation() const { return currentAnimation; }

  void setHourMinute(int hour, int minute, bool use12h) {
    use12hMode = use12h;

    hour = constrain(hour, 0, 23);
    minute = constrain(minute, 0, 59);

    int dispHour = hour;
    if (use12h) {
      if (hour == 0) dispHour = 12;            // Midnight is 12 AM
      else if (hour > 12) dispHour = hour - 12; // Convert 13-23 to 1-11 PM
    }

    displayDigits[0] = dispHour / 10;
    displayDigits[1] = dispHour % 10;
    displayDigits[2] = minute / 10;
    displayDigits[3] = minute % 10;

    // Blank the leading zero (e.g. "9:05" instead of "09:05").
    // Kept visible in 24h mode so times like "04:30" read unambiguously.
    if (displayDigits[0] == 0 && use12h) {
      displayDigits[0] = -1;
    }
  }

  void setAmPm(bool pm) { isPM = pm; }
  void clearAmPm() { isPM = false; }

  void setSeparatorEnabled(bool enabled) { separatorEnabled = enabled; }
  bool isSeparatorEnabled() const { return separatorEnabled; }

  void setBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
    FastLED.setBrightness(brightness);
  }
  uint8_t getBrightness() const { return brightness; }

  bool is12hMode() const { return use12hMode; }

  void show() {
    render();
    FastLED.show();
  }

private:
  CRGB leds[NUM_LEDS];
  uint8_t brightness;
  uint8_t hue;

  AnimationFunction currentAnimation;
  const int* segMap = defaultSegmentMap;
  const uint8_t (*digitMap)[7] = defaultDigitMap;

  int displayDigits[NUM_DIGITS] = {-1, -1, -1, -1};
  bool isPM;
  bool use12hMode;
  bool separatorEnabled;

  void render() {
    bool ledMask[NUM_LEDS] = {false};

    for (int d = 0; d < NUM_DIGITS; ++d) {
      int digitValue = displayDigits[d];
      if (digitValue >= 0 && digitValue <= 9) {
        for (int s = 0; s < LEDS_PER_DIGIT; ++s) {
          if (digitMap[digitValue][s]) {
            int ledIndex = d * LEDS_PER_DIGIT + segMap[s];
            if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
              ledMask[ledIndex] = true;
            }
          }
        }
      }
    }

    // Blank everything that is not part of a digit.
    for (int i = 0; i < NUM_LEDS; ++i) {
      if (!ledMask[i]) {
        leds[i] = CRGB::Black;
      }
    }

    // Separator: smooth pulse (sinusoidal breathing) instead of hard on/off.
    // White = AM, Blue = PM, so AM/PM is readable at any moment.
    if (separatorEnabled) {
      CRGB sepColor = (use12hMode && isPM) ? CRGB::Blue : CRGB::White;
      uint8_t pulse = beatsin8(PULSE_BPM, SEPARATOR_GLOW, 255);
      leds[SEPARATOR_LED] = sepColor.nscale8(pulse);
    }
  }
};

#endif // LEDCONTROL_H
