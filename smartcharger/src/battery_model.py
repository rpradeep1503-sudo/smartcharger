"""Simple battery model: electrical + thermal. Toy values: tune them later."""
import numpy as np

CAP_AH = 2.5      # cell capacity (Ah)
R0 = 0.06         # internal resistance (ohm)
C_TH = 40.0       # thermal mass (J/K)
H = 0.25          # cooling to ambient (W/K)
V_MAX = 4.2       # max cell voltage (V)


def ocv(soc):
    """Open-circuit voltage as a function of SOC (0..1)."""
    return 3.0 + 1.2 * soc - 0.15 * np.exp(-12 * soc) + 0.1 * soc ** 3


def step(soc, temp, current, t_amb, dt=1.0):
    """Advance the battery by dt seconds with charging current (A)."""
    v = ocv(soc) + current * R0
    if v >= V_MAX:                                   # voltage limit
        current = max(0.0, (V_MAX - ocv(soc)) / R0)
    soc += current * dt / (3600 * CAP_AH)
    temp += (current ** 2 * R0 - H * (temp - t_amb)) / C_TH * dt
    return soc, temp, current
