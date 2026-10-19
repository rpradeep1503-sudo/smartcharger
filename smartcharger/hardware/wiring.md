# Wiring (ESP32 DevKit, typical pins; check your board)
| Signal | ESP32 pin | Connection |
|---|---|---|
| Fake battery SOC | GPIO34 | Pot middle pin; outer pins to 3.3V and GND |
| Temperature | GPIO4 | DS18B20 data; 4.7k resistor from data to 3.3V; VCC 3.3V; GND |
| Current command | GPIO25 | PWM to LED (with 220 ohm) or logic-level MOSFET gate |
| Optional INA219 | GPIO21 SDA, GPIO22 SCL | 3.3V, GND |

Safety: emulator mode only. No bare Li-ion cells. Hard cutoff at 45 C / 4.2 V.
Test order: LED blink, one sensor, pot, PWM, then the full loop.
