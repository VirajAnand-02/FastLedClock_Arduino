# FastLedClock

A 7-segment digital clock built from a single WS2812B (NeoPixel) strip, driven by
[FastLED](https://github.com/FastLED/FastLED) and a DS1307 real-time clock. The digit
segments are lit by a continuously running full-strip animation, so the time appears
to be painted in moving colour.

---

## Table of contents

- [Hardware](#hardware)
- [LED layout](#led-layout)
- [Repository layout](#repository-layout)
- [Building and uploading](#building-and-uploading)
- [Configuration](#configuration)
- [Serial command reference](#serial-command-reference)
- [Agent reference (machine-readable)](#agent-reference-machine-readable)
- [Animations](#animations)
- [Timekeeping behaviour](#timekeeping-behaviour)
- [Known issues and gotchas](#known-issues-and-gotchas)
- [Troubleshooting](#troubleshooting)

---

## Hardware

| Part | Notes |
|---|---|
| Arduino Nano (ATmega328P) | Target in `platformio.ini` is `nanoatmega328`. Any 328P board works. |
| WS2812B strip, 29 LEDs | GRB colour order. |
| DS1307 RTC module | Optional — the sketch falls back to a software clock. |
| 5 V supply | See the power note below. |

### Wiring

| Signal | Pin |
|---|---|
| WS2812B data in | **D2** (`LED_PIN`) |
| RTC SDA | **A4** |
| RTC SCL | **A5** |
| RTC / LED power | 5 V, GND (common ground with the Arduino) |

A 300–500 Ω resistor in series with the data line and a 1000 µF capacitor across
the strip's 5 V/GND are the usual recommendations for WS2812B strips.

### Power

29 WS2812B pixels at full white draw roughly **1.7 A**, well beyond what a USB port
will supply. The sketch therefore calls:

```cpp
FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
```

FastLED scales the output brightness down at `show()` time to keep the *estimated*
draw under 500 mA. This is a safety cap for USB-powered bench testing. If you run the
strip from an adequate external 5 V supply, raise or remove this line in
`LEDControl.h` — otherwise the cap, not `BRIGHTNESS`, sets your ceiling.

---

## LED layout

29 LEDs total: **four digits × seven segments = LEDs 0–27**, plus **LED 28** as the
colon/separator. Digit `d` occupies LEDs `d*7` through `d*7 + 6`.

| Digit | LED range | Shows |
|---|---|---|
| 0 | 0–6 | hours, tens |
| 1 | 7–13 | hours, ones |
| 2 | 14–20 | minutes, tens |
| 3 | 21–27 | minutes, ones |
| — | 28 | separator / AM-PM indicator |

Within a digit, the LED order is set by `defaultSegmentMap` in `LEDControl.h`:

| LED offset | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|---|
| **Segment** | f | a | b | c | d | e | g |

Using standard 7-segment naming (`a` top, `b` top-right, `c` bottom-right, `d` bottom,
`e` bottom-left, `f` top-left, `g` middle). If your physical strip is wired
differently, edit `defaultSegmentMap` — `defaultDigitMap` is written in logical
`a..g` order and should not need changing.

### How rendering works

Each frame:

1. The active animation paints **all 29 LEDs**.
2. `render()` builds a mask of the LEDs belonging to lit segments and **blanks
   everything else to black**.
3. The separator is drawn last, over the top.

So the lit digit segments carry the animation colours and the rest of the strip is
dark. (The comment above the animations in `LEDControl.h` describing digits as
"holes cut out of the animation" is stale — it describes the inverse of what
`render()` actually does.)

---

## Repository layout

```
FastLedClock_Arduino/
├── FastLedClock_Arduino/
│   ├── FastLedClock_Arduino.ino   Setup, main loop, serial command handling
│   ├── LEDControl.h               LEDControl<> template, digit/segment maps, animations
│   └── ClockUtils.h               RTC access + software-clock fallback
├── setRTC/
│   └── setRTC.ino                 Standalone utility to set the DS1307 once
├── platformio.ini
└── README.md
```

`ClockUtils.h` and `LEDControl.h` are headers rather than `.cpp` files so the Arduino
IDE's automatic prototype generator cannot produce conflicting declarations for them.

---

## Building and uploading

### Arduino IDE

Open `FastLedClock_Arduino/FastLedClock_Arduino.ino`. Install **FastLED**,
**DS1307RTC**, and **Time** (Paul Stoffregen) via the Library Manager. Select your
board and upload. Set the Serial Monitor to **115200 baud**.

### PlatformIO

```sh
pio run -t upload
pio device monitor
```

Dependencies and `monitor_speed` are already declared in `platformio.ini`.

### Compile-checking without PlatformIO

If you only need to verify that a change builds, you can drive the Arduino IDE's
bundled AVR toolchain directly:

```sh
AVR="$LOCALAPPDATA/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/avr-g++"
CORE="$LOCALAPPDATA/Arduino15/packages/arduino/hardware/avr/1.8.6"
LIBS="$HOME/Documents/Arduino/libraries"

cp FastLedClock_Arduino/FastLedClock_Arduino.ino /tmp/sketch.cpp
cp FastLedClock_Arduino/*.h /tmp/

"$AVR" -c -Os -Wall -std=gnu++11 -fpermissive -fno-exceptions \
  -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=10819 \
  -DARDUINO_AVR_NANO -DARDUINO_ARCH_AVR \
  -I"$CORE/cores/arduino" -I"$CORE/variants/eightanaloginputs" \
  -I"$CORE/libraries/Wire/src" \
  -I"$LIBS/FastLED/src" -I"$LIBS/DS1307RTC" -I"$LIBS/Time" \
  /tmp/sketch.cpp -o /tmp/sketch.o
```

**A clean build is not proof of correctness here.** See
[the `switch` trap](#1-declaring-a-variable-in-an-unbraced-switch-case) below —
`avr-objdump -d` is sometimes the only way to confirm your code survived
optimisation.

---

## Configuration

All compile-time settings live at the top of `FastLedClock_Arduino.ino`:

| Constant | Default | Meaning |
|---|---|---|
| `NUM_LEDS` | `29` | Total pixels (4×7 digits + 1 separator). |
| `LED_PIN` | `2` | WS2812B data pin. |
| `BRIGHTNESS` | `255` | Startup brightness, and the daytime target. |
| `USE_12H` | `true` | **Currently unused** — see [known issues](#5-use_12h-is-dead-code). |
| `NIGHT_DIMMING` | `true` | Enable automatic dimming at night. |
| `NIGHT_BRIGHTNESS` | `127` | Brightness ceiling between 22:00 and 07:59. |
| `FRAME_MS` | `16` | Frame interval (~60 fps). |

In `LEDControl.h`:

| Constant | Default | Meaning |
|---|---|---|
| `SEPARATOR_GLOW` | `25` | Separator brightness at the dim end of its pulse. |
| `PULSE_BPM` | `15` | Separator pulse rate (~4 s per cycle). |

---

## Serial command reference

**115200 baud, 8N1.** Commands are **single bytes** — no terminator is needed and
none is expected. Any unrecognised byte (including `\r` and `\n`) is silently
discarded, so it is safe to leave your terminal's line ending on any setting.

| Input | Alias | Effect | Reply |
|---|---|---|---|
| `n` | | Advance to the next animation | `Animation #<0-4>` |
| `+` | `=` | Brightness +16 (saturates at 255) | `Brightness: <0-255>` |
| `-` | `_` | Brightness −16 (saturates at 0) | `Brightness: <0-255>` |
| `f` | | Toggle 12-hour / 24-hour format | `Format: 12h` or `Format: 24h` |
| `s` | | Toggle the separator LED on/off | `Separator: on` or `Separator: off` |
| `T<HHMMSS>` | | Set the clock (24-hour input) | see below |

### The `T` command

Send `T` immediately followed by exactly **six ASCII digits**, `HHMMSS`, in
**24-hour** form. The hour is always given as 00–23 regardless of whether the display
is currently in 12-hour mode.

```
T070000   ->  07:00:00  (7:00 AM)
T133000   ->  13:30:00  (1:30 PM)
T000000   ->  midnight
```

Replies:

| Condition | Reply |
|---|---|
| Accepted | `Time set to HH:MM:SS` |
| Accepted, but the RTC would not take the write | `Warning: RTC write failed (software clock only).` followed by `Time set to HH:MM:SS` |
| Parsed, but out of range (`HH>23`, `MM>59`, `SS>59`) | `Invalid time.` |
| A non-digit arrived, or a digit took >1000 ms | `Usage: T<HHMMSS> e.g. T070000` |

**Every path replies with exactly one of the above.** Silence means the `T` byte
never reached the board.

The six digits must follow `T` with nothing in between. The reader blocks the render
loop while it waits, with a 1000 ms timeout **per digit** — so a malformed command can
freeze the animation for up to 6 seconds. Send all seven bytes in one write rather
than typing them by hand.

### Boot output

On reset the sketch prints:

```
Advanced Animation RGB Clock
Commands: n=next animation  +/-=brightness  f=toggle 12/24h  T<HHMMSS>=set time  s=separator on/off
```

optionally followed by one or more of:

| Line | Meaning |
|---|---|
| `RTC not set. Setting to compile time...` | The RTC did not read back; seeding it from `__TIME__`/`__DATE__`. |
| `compile time: HH:MM:SS` | The compile timestamp being written. |
| `Failed to parse compile time.` | `__TIME__` did not parse. |
| `Failed to write to RTC.` | The I²C write was not acknowledged. |
| `RTC unavailable - running on software clock.` | Falling back; clock starts at 12:00:00. |
| `RTC read failed, using software clock.` | Printed once, after 10 s of uptime, if reads keep failing. |

There is **no periodic or unsolicited output** after boot. The device only speaks in
response to a command.

---

## Agent reference (machine-readable)

This section is the I/O contract for programs and agents driving the clock over a
serial port.

### Transport

```yaml
baud: 115200
data_bits: 8
parity: none
stop_bits: 1
flow_control: none
newline_required: false      # commands are raw bytes, not lines
device_line_ending: "\r\n"   # replies use Serial.println()
reset_on_open: true          # DTR toggle resets the board; expect the boot banner
unsolicited_output:          # the only output not triggered by a command
  - regex: '^RTC read failed, using software clock\.$'
    when: "once, at ~10 s uptime, only if RTC reads are failing"
```

After opening the port, **wait ~2 s** for the bootloader and the banner before
sending anything.

### Command table

```yaml
commands:
  - send: "n"
    aliases: []
    effect: "advance to next animation"
    reply_regex: '^Animation #[0-4]$'
    blocking_ms: 0

  - send: "+"
    aliases: ["="]
    effect: "brightness += 16, clamped to 255"
    reply_regex: '^Brightness: (25[0-5]|2[0-4]\d|1\d\d|\d{1,2})$'
    blocking_ms: 0

  - send: "-"
    aliases: ["_"]
    effect: "brightness -= 16, clamped to 0"
    reply_regex: '^Brightness: (25[0-5]|2[0-4]\d|1\d\d|\d{1,2})$'
    blocking_ms: 0

  - send: "f"
    aliases: []
    effect: "toggle 12h/24h display format"
    reply_regex: '^Format: (12h|24h)$'
    blocking_ms: 0

  - send: "s"
    aliases: []
    effect: "toggle separator LED"
    reply_regex: '^Separator: (on|off)$'
    blocking_ms: 0

  - send: "T<HHMMSS>"
    aliases: []
    effect: "set clock; HH is 24-hour (00-23), MM/SS 00-59"
    replies:
      success:      '^Time set to \d{2}:\d{2}:\d{2}$'
      degraded:     '^Warning: RTC write failed \(software clock only\)\.$'  # precedes success
      out_of_range: '^Invalid time\.$'
      malformed:    '^Usage: T<HHMMSS> e\.g\. T070000$'
    blocking_ms: 6000   # worst case: 1000 ms timeout per missing digit
    write_atomically: true
```

### Rules for driving this device

1. **Send `T` and its six digits in a single write.** The parser consumes the next
   six bytes from the stream directly; anything interleaved between them fails the
   command.
2. **`T` always replies.** If nothing arrives within ~6.5 s, the byte was lost in
   transit — retry rather than assuming success.
3. **Other commands always reply too**, but there is **no negative acknowledgement**.
   An unrecognised byte produces no output at all, which is indistinguishable from a
   dropped byte. Only send bytes from the table above.
4. **Do not pipeline.** Send one command, read its reply, then send the next.
   Back-to-back bursts risk the byte loss described below.
5. **Byte loss is possible at 115200.** `FastLED.show()` disables interrupts for
   ~870 µs every 16 ms (29 LEDs), and the ATmega328P buffers only 2 bytes in
   hardware. A multi-byte burst that overlaps a `show()` can lose bytes. The `T`
   command is the only multi-byte command and is therefore the only one exposed to
   this. Retry on timeout; consider dropping to 9600 baud if you need reliability
   (see [known issues](#2-serial-byte-loss-at-115200)).
6. **State is not queryable.** There is no "read current state" command. Brightness,
   animation index, format and separator state can only be tracked by observing the
   replies to your own commands, and are reset by any board reset.
7. **`f` reports the format it switched *to***, so it can be used to reach a known
   state: send `f`, read the reply, and send `f` again if it is not what you wanted.

### Example session

```
<- Advanced Animation RGB Clock
<- Commands: n=next animation  +/-=brightness  f=toggle 12/24h  T<HHMMSS>=set time  s=separator on/off
-> T133000
<- Time set to 13:30:00
-> f
<- Format: 24h
-> -
<- Brightness: 239
-> n
<- Animation #1
```

---

## Animations

Cycled with `n`. Indices match the `animations[]` array in the `.ino`.

| # | Name | Description |
|---|---|---|
| 0 | `animateRainbow` | Full-spectrum gradient sweeping along the strip. |
| 1 | `animateOcean` | Blue/green sine wave. |
| 2 | `animateLava` | Warm glow from FastLED's `HeatColors_p` palette. |
| 3 | `animateConfetti` | Random sparkles over a fading background. **Currently broken** — see [known issues](#3-confetti-is-wiped-every-frame). |
| 4 | `animateBreathe` | Slow single-colour pulse via `beatsin8`. |

An animation is a plain function pointer:

```cpp
using AnimationFunction = void (*)(CRGB* leds, int numLeds, uint8_t hue);
```

To add one, write the function in `LEDControl.h` and append it to `animations[]` in
the `.ino`; `NUM_ANIMATIONS` is computed automatically. `hue` advances by 1 every
third frame (~20 steps/second).

---

## Timekeeping behaviour

### Sources

`readClock()` prefers the DS1307 and polls it at most once per second; between
polls it returns a cached time. This keeps the LED animation running at ~60 fps
without repeatedly issuing I2C reads. On every successful read it also refreshes a
software clock; if the RTC later stops responding, that software clock keeps ticking
from `millis()` so the display never freezes. If no RTC is present at boot, the
software clock is seeded to 12:00:00.

Note that `RTC.read()` returns false both when the chip is **absent** and when it is
**present but halted** (the `CH` bit). `setClockTime()` handles both: it writes
unconditionally, which also restarts the oscillator.

### Display rules

- **12-hour mode:** midnight shows as `12`, 13:00–23:00 map to `1`–`11`, and a
  leading zero on the hour is blanked (`9:05`, not `09:05`).
- **24-hour mode:** the leading zero is kept, so `04:30` reads unambiguously.
- **AM/PM** is shown by the separator colour in 12-hour mode: **white = AM,
  blue = PM**. In 24-hour mode the separator is always white.
- The separator breathes sinusoidally between `SEPARATOR_GLOW` and full brightness
  rather than blinking, so the AM/PM colour is legible at every moment.

### Night dimming

When `NIGHT_DIMMING` is enabled, brightness is capped at `NIGHT_BRIGHTNESS` between
22:00 and 07:59 and restored to `BRIGHTNESS` at 08:00. See
[known issues](#4-night-dimming-overrides-manual-brightness) for how this interacts
with the `+`/`-` commands.

### Setting the time

Three options, in order of convenience:

1. **The `T` command** — no re-upload needed, works at runtime.
2. **`setRTC/setRTC.ino`** — a standalone utility that sets date *and* time. Upload
   it, open the Serial Monitor at **9600 baud** with line ending set to Newline, and
   send `YYYY,MM,DD,hh,mm,ss` (e.g. `2026,8,23,16,30,15`). Then re-upload the clock
   sketch. Use this when the date matters, since `T` only sets the time of day.
3. **Compile time** — if the RTC reads back as unset at boot, it is seeded from
   `__TIME__` / `__DATE__`. This is only as accurate as the gap between compiling and
   the board booting.

---

## Known issues and gotchas

### 1. Declaring a variable in an unbraced `switch` case

This is the trap that silently disabled most of the serial commands, and it is worth
understanding before editing `handleSerial()`.

```cpp
case '+':
case '=':
  int b = ...;      // no braces
  break;
case '-':           // <-- this jump crosses b's initialisation
```

Jumping to a case label past a variable's initialisation is **undefined behaviour**.
The Arduino AVR core compiles with `-fpermissive`, which downgrades the diagnostic
from an error to a warning — and then `-Os` assumes the UB never happens and
**deletes every subsequent case label from the binary**. The result was a sketch that
compiled and ran, but in which `-`, `_`, `f`, `T`, `s` and `default` did not exist:
`handleSerial()` was 0x66 bytes and dispatched only `n`, `+` and `=`. With braces
added it is 0x2f2 bytes and dispatches all eight.

**Always brace a `case` that declares a variable**, and treat
`warning: jump to case label [-fpermissive]` as an error.

### 2. Serial byte loss at 115200

FastLED sets `FASTLED_ALLOW_INTERRUPTS 0` on AVR, so `FastLED.show()` blocks
interrupts for the whole transmission — about 870 µs for 29 LEDs, every 16 ms. At
115200 baud a byte arrives every 87 µs and the ATmega328P holds only two in hardware,
so a burst overlapping a `show()` can be truncated. This affects `T` (the only
multi-byte command) roughly 10% of the time.

Dropping both `Serial.begin()` and `monitor_speed` to **9600** makes a byte take
1.04 ms — longer than the interrupt blackout — which removes the failure entirely.

### 3. Confetti is wiped every frame

`animateConfetti` relies on `fadeToBlackBy` accumulating trails across frames, but
`render()` blanks every non-segment LED to black on each frame before the next
animation call. The trails never survive, so animation #3 shows at most a single
sparkle, and only when it lands on a lit segment. Fixing it properly means giving the
animation its own persistent buffer, or applying the digit mask to a copy instead of
to `leds[]` in place.

### 4. Night dimming overrides manual brightness

`applyBrightnessForHour()` tracks a single `static uint8_t applied` value. During the
day its target is hard-coded to `BRIGHTNESS`, so any level you set with `+`/`-` is
silently reset at the 08:00 transition. At night the target is
`min(NIGHT_BRIGHTNESS, current)`, which behaves differently again. A cleaner design
would keep the user's chosen base brightness separately and apply the night factor on
top of it.

### 5. `USE_12H` is dead code

`constexpr bool USE_12H = true;` is never read. `LEDControl`'s constructor hard-codes
`use12hMode(true)`, so changing the constant has no effect — use the `f` command, or
wire the constant through to the constructor.

### 6. Brightness steps are perceptually small

`+`/`-` move in steps of 16/255 (~6%). Near the top of the range a single press is
barely visible, especially with the 500 mA power cap also scaling the output. Expect
to press several times before the change is obvious.

---

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| A serial command does nothing and prints nothing | An unbraced declaration in `handleSerial()` deleted the case — see [issue 1](#1-declaring-a-variable-in-an-unbraced-switch-case). Rebuild and check for `jump to case label`. |
| All commands ignored, output is garbage | Serial Monitor baud mismatch. The clock sketch uses 115200; `setRTC.ino` uses 9600. |
| `T` works intermittently | Byte loss during `FastLED.show()` — see [issue 2](#2-serial-byte-loss-at-115200). |
| Clock displays `00:00` after a fresh RTC seed | An older revision corrupted `tmElements_t` by passing `uint8_t*` to `sscanf("%d")`. Fixed; use `T` once to correct the stored time. |
| Display stuck, no animation | The `T` handler is blocking on a partial command — it clears itself after at most 6 s. |
| Display dimmer than expected | Either night dimming (22:00–07:59) or the 500 mA `setMaxPowerInVoltsAndMilliamps` cap. |
| Segments light in the wrong pattern | `defaultSegmentMap` does not match your physical wiring — see [LED layout](#led-layout). |
| Colours wrong (red/green swapped) | Change `GRB` to `RGB` in `LEDControl::begin()`. |
| Time resets on every power cycle | RTC not responding — check A4/A5 and the backup battery. The board reports this at boot. |
