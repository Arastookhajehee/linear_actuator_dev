# orchestrated linear actuators dev plan

## Current Issues

1. Linear Actuators run without any awareness of other actuators

## Goals

1. Linear Actuators must run with relation to the other actuators

### Concepts

| Term                 | Definition                                                                      |
| -------------------- | ------------------------------------------------------------------------------- |
| Node                 | Set of 4 Linear Actuators controlled by the same `Serial Python Server`         |
| Serial Python Server | Python Server that controls 4 linear actuator via one Serial port               |

## Milestones

### 1. Pending Assembly Work

- [ ] Assembly and test run the current 4 actuator new Node

### 2. Orchestrated Linear Actuator Runtime Control

- [ ] Understand how Grasshopper can Send Targets and Read Current Status
  - [ ] Look at the GetRequest `GetActuators`
  - [ ] Look at the PostRequest `PostTargets`
- [ ] Create a Current Surface to Target Surface Interpolation Algorithm in GH
- [ ] Implement a `Check Current Status` and Send Next Target after last
- [ ] Write documentation about explaining the problem with unexpected shapes and stresses mid-motion
  - [ ] refer to the diagrams and the drawings on the iPad
  - [ ] Explain the concept: Start OK -> End OK --> Middle Probably OK

### 3. Linear Actuator/Sensor Position Mapping

- [ ] Finish the Linear Actuator/Sensor Position Mapping

## Nice Improvements for later

1. Linear actuators' speed follows a distance-to-target logic
   - the farther away from the target the faster
