# SmartCharge rig: wiring and test guide (low voltage only, 5 V)

**No real battery is charged.** A power resistor is the "charging load" and gives off the heat that
fast charging would. The ESP32 decides the current, measures it, watches temperature and runs the fan.

## Parts
| Part | Use |
|---|---|
| ESP32 dev board | controller, Wi-Fi |
| DS18B20 x1 or x2 | sensor 1 taped to the resistor (hot spot), sensor 2 in the box air (optional) |
| INA219 module (CJMCU-219) | measures load voltage and current |
| 2 x logic-level N-MOSFET (IRLZ44N, IRLB8721, AO3400 or a 3.3 V-ready module) | one switches the load, one the fan |
| Power resistor 10 ohm, 5 W or more | the "battery heat" load (about 2.5 W at 5 V, full PWM) |
| 5 V DC fan | cooling |
| 1N4007 or 1N5819 diode | across the fan (flyback) |
| 10k potentiometer | start state of charge of the emulated battery |
| PIR sensor (HC-SR501) | "vehicle arrived": starts a charging session |
| 4.7k resistor, 2 x 10k, 2 x 100 ohm, breadboard, wires | pull-up, gate pull-downs, gate resistors |
| 5 V supply (USB power bank or adapter, 2 A) | power for fan, load and ESP32 |

IRF520 "MOSFET modules" and plain IRF540 or IRFZ44N parts do **not** switch fully at 3.3 V. Use logic-level types.

## Connections
| From | To |
|---|---|
| 5 V supply + | ESP32 VIN, fan +, INA219 VIN+, PIR VCC |
| 5 V supply - | ESP32 GND, every GND below (common ground is essential) |
| INA219 VIN- | resistor, one end |
| Resistor, other end | MOSFET 1 drain |
| MOSFET 1 source | GND |
| MOSFET 1 gate | ESP32 GPIO25 through 100 ohm, plus 10k to GND |
| Fan - | MOSFET 2 drain |
| MOSFET 2 source | GND |
| MOSFET 2 gate | ESP32 GPIO26 through 100 ohm, plus 10k to GND |
| Diode across fan | stripe (cathode) to fan +, other end to fan - |
| INA219 VCC / GND | ESP32 3V3 / GND |
| INA219 SDA / SCL | GPIO21 / GPIO22 |
| DS18B20 VCC / GND | 3V3 / GND |
| DS18B20 DATA | GPIO4, with 4.7k from DATA to 3V3 (both sensors share this wire) |
| Pot outer pins | 3V3 and GND |
| Pot wiper | GPIO34 |
| PIR OUT | GPIO27 |

## Test in this order (do not skip)
1. Upload the sketch with nothing but the ESP32. Open Serial Monitor at 115200.
2. Add the DS18B20. It should report "sensors found: 1" (or 2) and a sensible temperature.
3. Add the INA219. No "not found" message. Current should read about 0 mA.
4. Add MOSFET 2 and the fan. Press `f` until the fan mode is 1 (always on): the fan runs.
5. Add MOSFET 1 and the resistor. Press `s` (or wave at the PIR after 30 s). The load heats up slowly.
6. Press `c` to change strategy and `f` for fan mode, and log the Serial CSV for each run.

## Making the demo react
- Tape the DS18B20 to the resistor. It is only a few watts, so in a cool room it may not reach 38 C.
  Lower the thresholds at the top of the sketch (for example T_WARN 33, T_STOP 40, T_FAN_ON 28, T_FAN_FULL 36).
- 1 real second equals 60 simulated seconds of charging (TIMESCALE). A session takes about half a minute to a minute.
- Cell current in the model (up to 6.25 A) maps to PWM duty 0 to 100%. The INA219 shows the real electrical load (a few hundred mA).

## Safety
- 5 V only. No mains, no Li-ion cells in this rig.
- The resistor gets hot: mount it on a heatsink or a non-flammable surface, away from wires and plastic. Do not touch it.
- The code stops the load above T_STOP and also if the temperature sensor fails.
- Check polarity and common ground before power-up.

## Live dashboard
Join the Wi-Fi network `SmartCharge` (password `charge1234`). In `dashboard/index.html` choose ESP32 mode and connect to
`http://192.168.4.1/data`. While you are on that network you have no internet, so open the page first while online,
then switch Wi-Fi without refreshing it (or save the three.js file next to the page, see dashboard/README.md).
