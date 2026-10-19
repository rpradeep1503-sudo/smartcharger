# SmartCharger: Adaptive Fast-Charging Controller
Sustainathon 2026, VIT Vellore. Problem EV-02: The Fast-Charging vs Battery-Life Conflict.

## Idea
Fast charging heats and stresses the battery. Our controller reads SOC, temperature and voltage and picks
the highest safe current every second, instead of using one fixed profile.

## Structure
- `src/` battery model, 3 strategies (baseline CC-CV, naive fast, adaptive), experiments
- `firmware/` ESP32 demo (emulator mode, safe)
- `hardware/` wiring and safety notes
- `docs/` problem statement, pitch outline, team
- `results/` graphs and CSV (generated)

## Run
```
pip install -r requirements.txt
python src/run_experiments.py
```
Outputs `results/results.csv` and `results/compare_25C.png`, `compare_35C.png`.

## Limitations
Toy battery parameters; stress score is a proxy. Hardware runs in emulator mode for safety.

## Team
See docs/team.md.
