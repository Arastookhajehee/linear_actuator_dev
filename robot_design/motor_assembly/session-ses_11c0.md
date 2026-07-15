# New session - 2026-06-20T07:37:38.102Z

**Session ID:** ses_11c0945c9ffeT1I5zCsO8z663D
**Created:** 6/20/2026, 4:37:38 PM
**Updated:** 6/20/2026, 4:38:05 PM

---

## User

read the @Power_Supply.pdf and save it's most important issue int your context

---

## Assistant (Plan · GPT-5.5 Fast · 26.8s)

Most important issue from `Power_Supply.pdf` to keep in context:

The PSA-24/PSA-24L can drop output voltage or enter over-current protection if multiple actuator controllers are powered on or released from emergency stop at the same time due to inrush current. If this happens, controllers may take longer to start or recover from E-stop. Mitigation: start/enable controllers one by one, or add another power supply.

Key related limits:
- PSA-24: 24 V, 8.5 A rated, 17 A peak, 204 W
- PSA-24L: 24 V, 13.8 A rated, 17 A peak, 330 W
- Peak current allowed only with duty cycle `D <= 0.3`, pulse duration `t <= 10 s`, and average current within derated continuous rating
- Do not parallel different models/capacities; only PSA-24 with PSA-24, or PSA-24L with PSA-24L

---

