# SmartCharger: Adaptive Fast-Charging Controller
Sustainathon 2026, VIT Vellore. Problem EV-02: The Fast-Charging vs Battery-Life Conflict.

## Idea
Fast charging heats and stresses the battery. Our controller reads SOC, temperature and voltage and picks
the highest safe current every second, instead of using one fixed profile.

## Structure
- `src/` battery model, 3 strategies (baseline CC-CV, naive fast, adaptive), experiments
- `firmware/smartcharger_rig/` ESP32 rig: adaptive charging, INA219, DS18B20, smart fan, Wi-Fi data
- `hardware/` wiring and safety notes
- `dashboard/` 3D digital twin (open index.html)
- `docs/` problem statement, pitch outline, team
- `results/` graphs and CSV (generated)

## Run
```
pip install -r requirements.txt
python src/run_experiments.py
```
Outputs `results/results.csv` and `results/compare_25C.png`, `compare_35C.png`.

Cooling experiment (fan modes x strategies at 35 and 40 C ambient):
```
python src/run_thermal_experiments.py
```
Outputs `results/thermal_results.csv` and `results/thermal_fan_40C.png`.

## Limitations
Toy battery and fan parameters; stress score is a proxy. The rig uses a power resistor as the heat source and does not charge a real battery.

## Team
See docs/team.md.
