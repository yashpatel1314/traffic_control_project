# Arduino 4-Way Traffic Control System

An Arduino-based simulation of a real 4-way intersection, complete with pedestrian crosswalk signals, button-activated crossings, emergency override, night mode, and an enhanced variant with an LCD countdown display and audio alerts.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Pin Reference](#pin-reference)
- [Wiring](#wiring)
- [Getting Started](#getting-started)
- [Code Versions](#code-versions)
- [Operation Modes](#operation-modes)
- [Timing Sequence](#timing-sequence)
- [Customizing Timings](#customizing-timings)
- [Testing](#testing)
- [Known Issues](#known-issues)
- [Project Structure](#project-structure)

---

## Overview

```
         NORTH
           |
   [R][Y][G]   [G][Y][R]
   ─────────── ─ ─ ─ ───────────
WEST             ┼            EAST
   ─────────── ─ ─ ─ ───────────
   [G][Y][R]   [R][Y][G]
           |
         SOUTH
```

This project models a standard signalized intersection with:
- Two-phase traffic signal cycle (North-South / East-West)
- Pedestrian crosswalk signals synchronized to traffic phases
- Walk-request buttons that extend the current green phase
- Emergency override that flashes all-red until released
- Night mode that switches to a slow flashing yellow on all directions

Two `.ino` sketches are provided:

| Sketch | Target Use |
|---|---|
| `traffic_control.ino` | Basic build — LEDs and buttons only |
| `traffic_control_enhanced.ino` | Full build — adds 16×2 LCD, speaker, night-mode button |

---

## Features

- **Automatic two-phase cycle** — 15 s green → 3 s yellow → 2 s all-red, then repeat for the other direction
- **Pedestrian crosswalk signals** — STOP (red) / GO (green) LEDs synchronized to traffic phases
- **Walk-request buttons** — Pressing during the opposing green phase adds 10 s of protected crossing time; last 3 s flash as a warning
- **Emergency override** — Hold the emergency button to flash all red; release to resume
- **Night mode** *(enhanced only)* — Slow flashing yellow on all lights; crosswalk lights off
- **LCD countdown** *(enhanced only)* — 16×2 display shows current phase name and seconds remaining
- **Audio alerts** *(enhanced only)* — Distinct tones for button presses, state changes, and crosswalk activation

---

## Hardware Requirements

### Basic Build (`traffic_control.ino`)

| Qty | Component |
|-----|-----------|
| 1 | Arduino Uno (or compatible) |
| 1 | 830-point breadboard |
| 1 | Jumper wire kit |
| 10 | 220 Ω resistors |
| 2 | Red LEDs (traffic lights) |
| 2 | Yellow LEDs (traffic lights) |
| 2 | Green LEDs (traffic lights) |
| 2 | Red LEDs (crosswalk STOP) |
| 2 | Green LEDs (crosswalk GO) |
| 3 | Momentary push buttons (NO) |
| 1 | USB cable |

### Additional Components for Enhanced Build

| Qty | Component |
|-----|-----------|
| 1 | 16×2 LCD (HD44780 compatible) |
| 1 | Small passive or active speaker/buzzer |
| 1 | 10 kΩ potentiometer (LCD contrast) |
| 4 | Additional 220 Ω resistors (crosswalk signals) |
| 1 | Extra momentary push button (night-mode) |

---

## Pin Reference

### Basic Build — `traffic_control.ino`

| Pin | Signal | Direction |
|-----|--------|-----------|
| 2 | NS Red | OUT |
| 3 | NS Yellow | OUT |
| 4 | NS Green | OUT |
| 5 | EW Red | OUT |
| 6 | EW Yellow | OUT |
| 7 | EW Green | OUT |
| 8 | NS Crosswalk STOP | OUT |
| 9 | NS Crosswalk GO | OUT |
| 10 | EW Crosswalk STOP | OUT |
| 11 | EW Crosswalk GO | OUT |
| 12 | NS Walk Button | IN (PULLUP) |
| 13 | EW Walk Button | IN (PULLUP) |
| A0 | Emergency Button | IN (PULLUP) |

### Enhanced Build — `traffic_control_enhanced.ino`

| Pin | Signal | Direction |
|-----|--------|-----------|
| 2 | NS Red | OUT |
| 3 | NS Yellow | OUT |
| 4 | NS Green | OUT |
| 5 | EW Red | OUT |
| 6 | EW Yellow | OUT |
| 7 | LCD RS | OUT |
| 8 | LCD EN | OUT |
| 9 | LCD D4 | OUT |
| 10 | LCD D5 | OUT |
| 11 | LCD D6 | OUT |
| 12 | LCD D7 | OUT |
| 13 | EW Green | OUT |
| A0 | Emergency Button | IN (PULLUP) |
| A1 | NS Crosswalk STOP | OUT |
| A2 | NS Crosswalk GO | OUT |
| A3 | EW Crosswalk STOP | OUT |
| A4 | EW Crosswalk GO | OUT |
| A5 | NS Walk Button | IN (PULLUP) |
| A6 | EW Walk Button | IN (PULLUP) |

> **Note:** In the enhanced sketch `SPEAKER_PIN = 0` and `NIGHT_MODE_BUTTON = 1` are assigned to the hardware serial (RX/TX) pins. If you need serial debugging and audio/night-mode simultaneously, move the speaker and night-mode button to unused analog pins (e.g. A6/A7 on Uno are analog-input only; use a different unused digital pin instead).

---

## Wiring

See [circuit_diagram.txt](circuit_diagram.txt) for the full ASCII wiring diagram and physical layout suggestions.

**LED wiring pattern** (same for every LED in the project):

```
Arduino pin ──[220 Ω]──[LED anode]──[LED cathode]── GND
```

**Button wiring pattern** (all buttons use internal pull-up):

```
Arduino pin ──[one side of button]
GND         ──[other side of button]
```

No external pull-up resistors are needed for buttons — `INPUT_PULLUP` is used in software.

---

## Getting Started

### 1. Install Arduino IDE

Download from [arduino.cc/en/software](https://www.arduino.cc/en/software) (version 1.8+ or 2.x).

### 2. Open the sketch

- Basic:    `File → Open → traffic_control.ino`
- Enhanced: `File → Open → traffic_control_enhanced.ino`

For the enhanced build, install the `LiquidCrystal` library if not already present:
`Tools → Manage Libraries → search "LiquidCrystal" → Install`

### 3. Select board and port

`Tools → Board → Arduino Uno`  
`Tools → Port → COMx` (Windows) or `/dev/ttyUSBx` (Linux/macOS)

### 4. Upload

Click **Upload** (Ctrl+U). Open `Tools → Serial Monitor` at **9600 baud** to watch state transitions in real time.

### 5. Verify behaviour

- All six traffic LEDs should cycle automatically
- Press a walk button during the correct green phase to trigger a crosswalk extension
- Hold the emergency button to enter flashing-red mode; release to resume

---

## Code Versions

### `traffic_control.ino` — Basic

Pure state-machine implementation with no external library dependencies. Good starting point for understanding the logic. All pins fit within digital pins 2–13 and A0.

### `traffic_control_enhanced.ino` — Enhanced

Adds:
- `LiquidCrystal` 16×2 LCD showing phase name and countdown in seconds
- `tone()` audio feedback on button press, state change, and crosswalk activation
- Night mode — toggled by a dedicated button; flashes both yellow lights at 0.5 Hz
- Analog pins used for crosswalk LEDs to free up digital pins for the LCD

Both sketches share the same state-machine design and identical timing constants.

---

## Operation Modes

### Normal Cycle

```
┌─────────────────────┬──────────┬───────────────────────────────────┐
│ Phase               │ Duration │ Description                       │
├─────────────────────┼──────────┼───────────────────────────────────┤
│ NS Green / EW Red   │ 15 s     │ North-South traffic flows         │
│ NS Yellow / EW Red  │  3 s     │ North-South clearance warning     │
│ All Red             │  2 s     │ Safety clearance interval         │
│ EW Green / NS Red   │ 15 s     │ East-West traffic flows           │
│ EW Yellow / NS Red  │  3 s     │ East-West clearance warning       │
│ All Red             │  2 s     │ Safety clearance interval         │
└─────────────────────┴──────────┴───────────────────────────────────┘
Total cycle: ~40 seconds
```

### Crosswalk Extension

Pressing a walk button while the opposing direction has the green light:
1. Activates the GO signal for pedestrians
2. Extends the current green phase by 10 seconds
3. Flashes the GO signal during the last 3 seconds as a warning

Pressing the button when the opposing direction does **not** have green has no effect (the request is logged but not queued).

### Emergency Mode

Hold the emergency button → all red LEDs flash at 2 Hz, crosswalk STOP signals flash in sync. Release the button to resume the cycle from NS Green.

### Night Mode *(enhanced only)*

Toggle the night-mode button → both yellow LEDs flash at 0.5 Hz; all other lights and crosswalk signals turn off. Toggle again to return to the normal day cycle.

---

## Timing Sequence

```
Time (s)  0        15       18  20       35       38  40
          │        │         │  │        │         │  │
NS light  ├──GREEN─┤─YELLOW─┤RD├───RED──┤───RED───┤GR┤
EW light  ├───RED──┤───RED──┤RD├──GREEN─┤─YELLOW─┤RD┤
```

---

## Customizing Timings

All timing values are defined as constants at the top of each sketch:

```cpp
const unsigned long GREEN_TIME        = 15000; // ms
const unsigned long YELLOW_TIME       =  3000; // ms
const unsigned long RED_TIME          =  2000; // ms
const unsigned long CROSSWALK_TIME    = 10000; // ms
const unsigned long CROSSWALK_WARNING =  3000; // ms
```

Example — rush-hour profile (longer green, longer yellow):

```cpp
const unsigned long GREEN_TIME  = 25000;
const unsigned long YELLOW_TIME =  4000;
```

---

## Testing

A Python simulation of the state machine is included in `tests/test_state_machine.py`. It runs without any hardware and validates:

- All six state transitions in the normal cycle
- Crosswalk activation and expiry logic
- Emergency mode entry and exit
- Night mode toggling (enhanced logic)

Run with Python 3.8+:

```bash
python tests/test_state_machine.py
```

See [setup_guide.md](setup_guide.md) for hands-on hardware test procedures (timing verification, LED checks, button response tests).

---

## Known Issues

1. **Enhanced sketch — serial pins conflict:** `SPEAKER_PIN = 0` (RX) and `NIGHT_MODE_BUTTON = 1` (TX) overlap with the hardware UART used by `Serial.begin(9600)`. Reassign these to unused digital pins if you need serial output and audio simultaneously.

2. **Walk-button request not queued:** If a pedestrian presses the button while traffic for their direction already has the green, the request is ignored rather than held for the next cycle.

3. **`millis()` rollover after ~49 days:** The timing arithmetic is not rollover-safe. For a permanent installation, apply a rollover-safe comparison or reset the Arduino periodically.

---

## Project Structure

```
traffic_control_project/
├── traffic_control.ino          # Basic sketch (no external libs)
├── traffic_control_enhanced.ino # Enhanced sketch (LCD + audio + night mode)
├── circuit_diagram.txt          # ASCII wiring diagrams for both builds
├── setup_guide.md               # Detailed setup, testing & troubleshooting guide
├── tests/
│   └── test_state_machine.py    # Python simulation & unit tests (no hardware needed)
└── LICENSE
```

---

