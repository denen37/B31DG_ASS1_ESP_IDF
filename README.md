# ESP32 LED Toggle with Interrupts (ESP-IDF)

## 📌 Project Overview
This project demonstrates how to use **GPIO interrupts**, **FreeRTOS tasks**, and 
**software debouncing** on the ESP32 using ESP-IDF.

Two push buttons are used to:
- Toggle the ENABLE state
- Toggle the NORMAL/ALTERNATE signal mode

When enabled, the ESP32 generates a timed pulse sequence on two output GPIO pins.

---

## 🧠 Features

- GPIO interrupt handling (ISR)
- FreeRTOS queue communication from ISR to task
- Software debounce using FreeRTOS tick timing
- Separate signal generation task
- Debug timing scale option
- Watchdog-safe task design

---

## 🛠 Hardware Configuration

### Output Signals
- `GPIO16` → SIGNAL A
- `GPIO21` → SIGNAL B

### Input Buttons
- `GPIO32` → ENABLE button
- `GPIO33` → SELECT button

> Ensure proper pull-up or pull-down resistors are configured according to your hardware.

---

## ⚙️ How It Works

1. Button press triggers a GPIO interrupt.
2. ISR sends the GPIO number to a FreeRTOS queue.
3. `button_task` receives the event.
4. Debounce logic validates the press.
5. State variables are toggled.
6. `signal_task` generates the waveform when ENABLE is active.

---

## ⏱ Timing Parameters

| Parameter | Description |
|-----------|------------|
| `T_ON1` | Base ON pulse time |
| `T_OFF` | OFF time between pulses |
| `NUM` | Number of pulses |
| `T_SYNC_ON` | Sync pulse time |
| `TIME_SCALE` | Debug timing multiplier |

To enable debug timing:
```c
#define DEBUG_BUILD