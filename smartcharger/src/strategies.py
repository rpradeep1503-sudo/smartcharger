"""Three charging strategies. Each returns the requested current in amps."""
from battery_model import CAP_AH, V_MAX


def baseline_cccv(soc, temp, volt, c_rate=1.0):
    """A: standard constant current, tapering near full."""
    if volt < V_MAX and soc < 0.8:
        return c_rate * CAP_AH
    return max(0.05 * CAP_AH, c_rate * CAP_AH * (1 - (soc - 0.8) / 0.2))


def naive_fast(soc, temp, volt):
    """B: high constant current, ignores temperature."""
    return baseline_cccv(soc, temp, volt, c_rate=2.5)


def adaptive(soc, temp, volt, c_max=2.5):
    """C: OUR method. Current depends on SOC, temperature and voltage."""
    if soc < 0.30:
        c = c_max
    elif soc < 0.60:
        c = c_max * 0.7
    elif soc < 0.80:
        c = c_max * 0.4
    else:
        c = 0.15
    if temp > 45:
        return 0.0                       # too hot: stop
    if temp > 38:
        c *= (45 - temp) / 7             # derate smoothly
    if temp < 10:
        c = min(c, 0.2)                  # cold: avoid lithium plating
    if volt >= V_MAX:
        c *= 0.5
    return c * CAP_AH
