# SmartCharger (Sustainathon 2026, EV-02)
Goal: adaptive fast-charging controller. Simulate in Python (src/), demo on ESP32 (firmware/).
Rules:
- Safety: stop at >45 C or >4.2 V per cell. Emulator mode only, no bare Li-ion cells.
- Keep Python and ESP32 `adaptive` logic identical.
- Thresholds are toy values: say so in results. Stress score is an estimate, not measured aging.
- Make small changes, run `python src/run_experiments.py` after each one.
Pins: pot GPIO34, DS18B20 GPIO4, PWM GPIO25, I2C 21/22.
