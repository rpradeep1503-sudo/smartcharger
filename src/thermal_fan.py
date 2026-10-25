"""Battery model with an active cooling fan (toy numbers, same battery as battery_model.py)."""
from battery_model import CAP_AH, R0, C_TH, H, V_MAX, ocv

K_FAN = 3.0        # full fan adds 3x the passive cooling (toy value, tune to your rig)
P_FAN_MAX = 1.0    # W drawn by the fan at 100% duty (measure yours)
T_FAN_ON, T_FAN_FULL = 32.0, 40.0


def fan_duty(mode, temp):
    """mode: 'off', 'on' (always full) or 'smart' (ramps with temperature)."""
    if mode == "off":
        return 0.0
    if mode == "on":
        return 1.0
    d = min(1.0, max(0.0, (temp - T_FAN_ON) / (T_FAN_FULL - T_FAN_ON)))
    return 0.35 if 0 < d < 0.35 else d      # fans stall at very low duty


def step(soc, temp, current, t_amb, duty, dt=1.0):
    v = ocv(soc) + current * R0
    if v >= V_MAX:
        current = max(0.0, (V_MAX - ocv(soc)) / R0)
    soc += current * dt / (3600 * CAP_AH)
    cooling = H * (1 + K_FAN * duty) * (temp - t_amb)
    temp += (current ** 2 * R0 - cooling) / C_TH * dt
    return soc, temp, current
