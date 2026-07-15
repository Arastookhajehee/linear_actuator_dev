## Robot Control Estimate Review Questions

This document summarizes the open questions about whether the current estimate is sufficient for controlling the robot and supporting the intended concrete spraying use case.

### Estimate Scope

1. Is the current estimate sufficient for controlling the robot?
1. Is the current component list complete enough for the intended system?
1. Should we review the component list again with the industry partner to confirm whether anything is missing?

### Environmental Requirements

1. Will the proposed system make the robot waterproof, or are additional waterproofing measures required?
1. Is the proposed system suitable for use during concrete spraying?

### Motion Control

1. If the robot only supports relative motion, how will zeroing and position recovery be handled?
1. How will the robot be controlled by the user?
1. Will control be based on streaming commands, one-shot commands, or both?
1. If streaming control is required, how can it be implemented safely and reliably?

### Power and Electrical Design

1. What are the expected power usage and power requirements?
1. How will power supply, distribution, and safety be handled?

### Controller Architecture

1. Where should the controller be placed?
1. Will there be a master PC?
1. Will the same PC control both the robot and the formwork?
1. What specifications are required for the computer that will run the control program?

### Safety

1. How will the emergency stop work based on the current estimate?
1. Does the estimate include all hardware and control logic required for emergency stop behavior?

### Arm and Formwork

1. How will the arm and formwork systems coordinate with each other?
1. Are any additional interfaces required between the robot controller, formwork controller, and user control PC?
