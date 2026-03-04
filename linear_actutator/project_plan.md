# Linear Actuator Project Plan (Deadline: 2026-03-01)

Start date: 2026-02-20  
Final deadline: 2026-03-01

| ID | Task | Deadline | Difficulty (1-5) | Notes |
|---|---|---|---:|---|
| T1 | Inventory and verify all hardware (actuator, PSU, IBT-2, Arduino, sensor, cabling, relay, emergency stop) | 2026-02-20 | 2 | Confirm all core parts are present, compatible, and ready before integration begins. |
| T2 | Freeze and document baseline wiring for a single actuator system | 2026-02-21 | 2 | Lock one known-good wiring layout and label it so future tests are consistent. |
| T3 | Reduce sensor noise (OUT-, twisted pair, grounding, filter capacitor, software filtering) | 2026-02-23 | 4 | Apply and measure one noise fix at a time to identify the highest-impact changes. |
| T4 | Calibrate conversion from analog reading to length (mm) for CWP-S1000v1 | 2026-02-24 | 4 | Build a reliable ADC-to-mm mapping from measured stroke points across full travel. |
| T5 | Tune Arduino control behavior (deadband, PWM, sampling interval, direction consistency) | 2026-02-24 | 3 | Tune control constants for stable target holding and record final parameter values. |
| T6 | Characterize speed and determine safe operating limits | 2026-02-25 | 3 | Define practical speed limits and acceptable stopping behavior near the target. |
| T7 | Run thermal test for actuator, H-bridge, and PSU under repeated motion cycles | 2026-02-26 | 3 | Run duty-cycle tests to set safe operating and cool-down limits for hardware. |
| T8 | Integrate and validate emergency stop path | 2026-02-26 | 4 | Ensure emergency stop cuts motor drive immediately and requires deliberate reset. |
| T9 | Improve host-side control/testing script and logging protocol | 2026-02-27 | 2 | Add simple command/result logging so test runs are traceable and repeatable. |
| T10 | Validate mechanical installation (sensor mount, driver mounting, cable strain relief) | 2026-02-27 | 3 | Verify mounting and cable routing are stable, repeatable, and free of binding. |
| T11 | Define scaling architecture for multiple actuators (2-channel proof, then 10-channel plan) | 2026-02-28 | 5 | Validate two channels first, then finalize power and I/O plan for ten channels. |
| T12 | Final integration test, issue triage, and demo readiness package | 2026-03-01 | 4 | Deliver a demo-ready package with verified behavior, limits, and concise documentation. |

## Priority and Execution Notes

| Priority | Focus |
|---|---|
| Critical path | T2 -> T3 -> T4 -> T5 -> T8 -> T12 |
| High value if time allows | T7 and T11 |
| Documentation guardrail | Update `observations.md` after each test session with date, setup, result, and next action |

## Definition of Done (for March 1)

- Single actuator reaches commanded target positions repeatably with acceptable noise and overshoot.
- Emergency stop behavior is verified and documented.
- Conversion from sensor reading to mm is calibrated and recorded.
- Known speed/thermal limits are documented with safe operating guidance.
- Multi-actuator expansion approach is defined (with at least a 2-channel validation or clear implementation plan).
