"""Run all strategies, print a table, save graphs and CSV to ../results."""
import os
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from battery_model import ocv, step, CAP_AH
from strategies import baseline_cccv, naive_fast, adaptive

OUT = os.path.join(os.path.dirname(__file__), "..", "results")
STRATS = {"A_baseline": baseline_cccv, "B_naive_fast": naive_fast, "C_adaptive": adaptive}


def run(strategy, t_amb=30.0, soc0=0.10, soc_end=0.80, dt=1.0):
    soc, temp, t, stress, log = soc0, t_amb, 0.0, 0.0, []
    while soc < soc_end and t < 7200:
        i_req = strategy(soc, temp, ocv(soc))
        soc, temp, i = step(soc, temp, i_req, t_amb, dt)
        # stress proxy: high C-rate, high temperature, high SOC all add stress
        stress += (i / CAP_AH) ** 2 * (1 + max(0, temp - 30) / 10) * (1 + max(0, soc - 0.8)) * dt / 3600
        t += dt
        log.append((t / 60, soc, i, temp))
    return pd.DataFrame(log, columns=["min", "soc", "current_A", "temp_C"]), stress


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    rows = []
    for t_amb in (25, 35):
        fig, ax = plt.subplots(3, 1, figsize=(8, 8), sharex=True)
        for name, f in STRATS.items():
            df, stress = run(f, t_amb=t_amb)
            rows.append(dict(ambient_C=t_amb, strategy=name, charge_time_min=round(df["min"].iloc[-1], 1),
                             peak_temp_C=round(df["temp_C"].max(), 1), stress_score=round(stress, 2)))
            for a, col in zip(ax, ["soc", "current_A", "temp_C"]):
                a.plot(df["min"], df[col], label=name)
        for a, l in zip(ax, ["SOC", "Current (A)", "Temperature (C)"]):
            a.set_ylabel(l)
        ax[2].set_xlabel("minutes"); ax[0].legend(); ax[0].set_title(f"Charging 10-80% at {t_amb} C ambient")
        plt.tight_layout(); plt.savefig(os.path.join(OUT, f"compare_{t_amb}C.png"), dpi=120); plt.close()
    table = pd.DataFrame(rows)
    table.to_csv(os.path.join(OUT, "results.csv"), index=False)
    print(table.to_string(index=False))
