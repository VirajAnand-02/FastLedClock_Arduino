// ClockUtils.h
// RTC + software-clock-fallback helpers.
// Kept out of the .ino so the Arduino auto-prototype generator
// cannot create conflicting declarations for them.
#ifndef CLOCKUTILS_H
#define CLOCKUTILS_H

#include <Arduino.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <TimeLib.h>

// Software-clock fallback: keeps ticking if the RTC fails or is absent.
namespace {
// The DS1307 only changes its time registers once per second.  Limit I2C
// transactions to that cadence while the LED animation continues at 60 fps.
constexpr uint16_t RTC_POLL_MS = 1000;
uint32_t lastValidRtcMs = 0;
uint8_t lastValidHour = 0, lastValidMinute = 0, lastValidSecond = 0;
}

// Advance the fallback clock from elapsed real time.
static void tickSoftwareClock() {
  uint32_t elapsed = millis() - lastValidRtcMs;
  while (elapsed >= 1000) {
    lastValidRtcMs += 1000;
    elapsed -= 1000;
    lastValidSecond++;
    if (lastValidSecond >= 60) { lastValidSecond = 0; lastValidMinute++; }
    if (lastValidMinute >= 60) { lastValidMinute = 0; lastValidHour = (lastValidHour + 1) % 24; }
  }
}

// Seed the software clock with a starting time.
static void seedSoftwareClock(uint8_t hour, uint8_t minute, uint8_t second) {
  lastValidHour = hour;
  lastValidMinute = minute;
  lastValidSecond = second;
  lastValidRtcMs = millis() - 1000;
}

// Read the current time. The cached time is returned between RTC polls, so this
// is safe to call every display frame without issuing an I2C transaction each
// time. Prefers the RTC; falls back to the software clock, so this always
// returns true once seeded (or after a successful RTC write).
static bool readClock(uint8_t& hourOut, uint8_t& minuteOut, uint8_t& secondOut) {
  static tmElements_t rtcTm;
  static uint32_t lastRtcPollMs = 0;
  static bool hasPolledRtc = false;

  uint32_t now = millis();
  bool pollDue = !hasPolledRtc || (uint32_t)(now - lastRtcPollMs) >= RTC_POLL_MS;

  if (pollDue) {
    hasPolledRtc = true;
    lastRtcPollMs = now;

    if (RTC.read(rtcTm)) {
      lastValidHour = rtcTm.Hour;
      lastValidMinute = rtcTm.Minute;
      lastValidSecond = rtcTm.Second;
      lastValidRtcMs = now;
      hourOut = rtcTm.Hour;
      minuteOut = rtcTm.Minute;
      secondOut = rtcTm.Second;
      return true;
    }
  }

  static bool reportedError = false;
  if (!reportedError && millis() > 10000) {
    Serial.println(F("RTC read failed, using software clock."));
    reportedError = true;
  }

  tickSoftwareClock();
  hourOut = lastValidHour;
  minuteOut = lastValidMinute;
  secondOut = lastValidSecond;
  return true;
}

static int monthStringToNumber(const char* monthStr) {
  static const char months[][4] = {"Jan","Feb","Mar","Apr","May","Jun",
                                   "Jul","Aug","Sep","Oct","Nov","Dec"};
  for (int i = 0; i < 12; i++) {
    if (strcmp(monthStr, months[i]) == 0) return i + 1;
  }
  return 0;
}

// Fill a tmElements_t with the compile-time date.
static bool setupRTCDate(tmElements_t& tm) {
  const char* dateStr = __DATE__;
  char monthStr[4];
  int day, year;
  if (sscanf(dateStr, "%s %d %d", monthStr, &day, &year) != 3) {
    return false;
  }
  tm.Day = day;
  tm.Month = monthStringToNumber(monthStr);
  tm.Year = CalendarYrToTm(year);
  return tm.Month != 0;
}

// Force-set the clock time. Writes to the RTC even if it is halted or
// unreadable (write also restarts the oscillator); falls back to the
// software clock when no RTC responds at all.
static bool setClockTime(uint8_t hour, uint8_t minute, uint8_t second) {
  seedSoftwareClock(hour, minute, second);

  tmElements_t tm;
  bool haveDate = RTC.read(tm);
  if (!haveDate) {
    // RTC.read() also returns false for a halted (but present) chip, in which
    // case tm is already populated. Fill in a date either way so we never
    // write uninitialised stack bytes to the chip.
    if (!setupRTCDate(tm)) {
      return false; // no RTC -> software clock keeps time
    }
    tm.Wday = 1; // DS1307 expects 1-7; not tracked by this sketch.
  }
  tm.Hour = hour;
  tm.Minute = minute;
  tm.Second = second;
  return RTC.write(tm);
}

// Initialize the RTC. If it is unset, seed it from the compile time.
static bool setupRTC() {
  static tmElements_t rtcTm;

  if (RTC.read(rtcTm)) {
    return true;
  }
  Serial.println(F("RTC not set. Setting to compile time..."));
  tmElements_t tm;
  const char* timeStr = __TIME__;
  Serial.println(String(F("compile time: ")) + timeStr);

  // Parse into ints: the tmElements_t fields are uint8_t, and "%d" writes a
  // full int, which would spill into the adjacent struct members.
  int h, m, s;
  if (sscanf(timeStr, "%d:%d:%d", &h, &m, &s) != 3 || !setupRTCDate(tm)) {
    Serial.println(F("Failed to parse compile time."));
    return false;
  }
  tm.Hour = (uint8_t)h;
  tm.Minute = (uint8_t)m;
  tm.Second = (uint8_t)s;
  tm.Wday = 1; // DS1307 expects 1-7; not tracked by this sketch.
  if (!RTC.write(tm)) {
    Serial.println(F("Failed to write to RTC."));
    return false;
  }
  return true;
}

#endif // CLOCKUTILS_H
