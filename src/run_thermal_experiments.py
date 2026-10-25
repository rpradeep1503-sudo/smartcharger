"""Charging strategy x fan mode, at hot ambient temperatures. Saves results/thermal_results.csv and a plot."""
import os
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from battery_model import ocv, CAP_AH
from strategies import baseline_cccv, naive_fast, adaptive
from thermal_fan import fan_duty, step, P_FAN_MAX

OUT = os.path.join(os.path.dirname(__file__), "..", "results")
STRATS = {"Baseline 1C": baseline_cccv, "Naive 2.5C": naive_fast, "Adaptive": adaptive}
FANS = ["off", "on", "smart"]


def run(strategy, fan_mode, t_amb, soc0=0.10, soc_end=0.80, dt=1.0):
    soc, temp, t, stress, fan_wh, peak, log = soc0, t_amb, 0.0, 0.0, 0.0, t_amb, []
    while soc < soc_end and t < 7200:
        duty = fan_duty(fan_mode, temp)
        soc, temp, i = step(soc, temp, strategy(soc, temp, ocv(soc)), t_amb, duty, dt)
        stress += (i / CAP_AH) ** 2 * (1 + max(0, temp - 30) / 10) * (1 + max(0, soc - 0.8)) * dt / 3600
        fan_wh += P_FAN_MAX * duty * dt / 3600
        peak = max(peak, temp)
        t += dt
        log.append((t / 60, soc, i, temp, duty))
    return dict(time_min=t / 60, peak_C=peak, fan_Wh=fan_wh, stress=stress), pd.DataFrame(log, columns=["min", "soc", "I", "T", "fan"])


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    rows = []
    for amb in (35, 40):
        for sname, sf in STRATS.items():
            for fm in FANS:
                r, _ = run(sf, fm, amb)
                rows.append(dict(ambient_C=amb, strategy=sname, fan=fm, **{k: round(v, 2) for k, v in r.items()}))
    df = pd.DataFrame(rows)
    df.to_csv(os.path.join(OUT, "thermal_results.csv"), index=False)
    print(df.to_string(index=False))
    fig, ax = plt.subplots(2, 1, figsize=(8, 6), sharex=True)
    for fm, col in zip(FANS, ["#8fa6b8", "#ff7a59", "#4cc9f0"]):
        _, d = run(adaptive, fm, 40)
        ax[0].plot(d["min"], d["T"], label=f"fan {fm}", color=col)
        ax[1].plot(d["min"], d["I"], color=col)
    ax[0].axhline(45, ls="--", color="red", lw=1); ax[0].set_ylabel("Temp (C)"); ax[1].set_ylabel("Current (A)")
    ax[1].set_xlabel("minutes"); ax[0].legend(); ax[0].set_title("Adaptive charging at 40 C ambient")
    plt.tight_layout(); plt.savefig(os.path.join(OUT, "thermal_fan_40C.png"), dpi=120)
