# Linear Actuator System Diagram

The rewrite is a single Windows WPF executable. It hosts the local WebAPI, SQLite serial-port configuration, in-memory state for 10 actuator modules, and optional serial connections to Arduino controllers.

Current development state: the API, WPF module grid, SQLite serial mapping, generated C# client, serial command formatting, telemetry ingestion, and MotionBatch step planner are implemented and covered by tests. MotionBatch is not yet wired into the API or serial dispatch loop, so posted target bundles still send their final target values directly to enabled Arduino modules.

Each module is one Arduino-controlled group of four actuators. Module IDs are `M01` through `M10`.

## Component View

```mermaid
flowchart TB
    subgraph exe[LinearActuator.App single exe]
        ui[WPF UI]
        api[Embedded Minimal API on 127.0.0.1:7500]
        store[ActuatorStateStore]
        bundle[ActuatorStateBundle: 10 modules]
        repo[PortMappingRepository]
        db[(SQLite: linear-actuator-modules.db)]
        serialManager[SerialModuleManager]
        serialConn[SerialActuatorConnection per enabled module]
    end

    client[Grasshopper / External HTTP Client]
    csharp[External C# app using LinearActuator.ClientAPI]
    arduino[Arduino module controllers]

    ui -->|serial on/off, COM, baud| repo
    repo --> db
    ui -->|start/stop runtime| api
    ui -->|start enabled serial rows| serialManager
    ui -->|display current/target state| store

    client -->|GET /actuators| api
    client -->|POST /actuators| api
    client -->|GET /actuator-bundles| api
    client -->|POST /actuator-bundles| api
    csharp -->|generated typed client| api

    api -->|read/write state| store
    store --> bundle
    api -->|dispatch targets by module| serialManager
    serialManager --> serialConn
    serialConn -->|CSV: T,a1,a2,a3,a4| arduino
    arduino -->|JSON telemetry lines| serialConn
    serialConn -->|module telemetry| serialManager
    serialManager -->|update selected module currents| store
```

## Project Boundaries

```mermaid
flowchart LR
    app[LinearActuator.App]
    core[LinearActuator.Core]
    infra[LinearActuator.Infrastructure]
    clientApi[LinearActuator.ClientAPI]
    tests[LinearActuator.Tests]

    app --> core
    app --> infra
    infra --> core
    clientApi --> core
    tests --> core
    tests --> infra

    app -. owns .-> wpf[WPF shell and module grid]
    core -. owns .-> domain[ActuatorState, ActuatorStateBundle, MotionBatch, state store, protocol rules]
    infra -. owns .-> adapters[SQLite, EF Core, serial ports, embedded API host]
    clientApi -. owns .-> generated[NSwag generated C# HTTP client]
```

## API Shape

```mermaid
flowchart TD
    singleGet[GET /actuators]
    singlePost[POST /actuators]
    bundleGet[GET /actuator-bundles]
    bundlePost[POST /actuator-bundles]
    m01[M01 ActuatorState]
    all[ActuatorStateBundle modules object]

    singleGet -->|compatibility view| m01
    singlePost -->|compatibility write| m01
    bundleGet --> all
    bundlePost --> all
```

Bundle JSON shape:

```json
{
  "modules": {
    "M01": { "a1_current": null, "a1_target": 50, "a2_current": null, "a2_target": 50, "a3_current": null, "a3_target": 50, "a4_current": null, "a4_target": 50 },
    "M02": { "a1_current": null, "a1_target": 50, "a2_current": null, "a2_target": 50, "a3_current": null, "a3_target": 50, "a4_current": null, "a4_target": 50 }
  }
}
```

## Bundle Target Dataflow

```mermaid
flowchart LR
    client[External client]
    request[POST /actuator-bundles]
    api[Minimal API]
    store[ActuatorStateStore]
    bundle[In-memory 10-module bundle]
    serialManager[SerialModuleManager]
    mappings[SQLite serial mappings]
    arduino[Enabled Arduino modules]
    response[Bundle response]

    client --> request
    request -->|modules M01..M10| api
    api -->|replace full bundle| store
    store --> bundle
    api -->|for each module target| serialManager
    mappings -->|serial enabled and connected decides dispatch| serialManager
    serialManager -->|CSV per enabled module| arduino
    store -->|snapshot bundle| response
    response --> client
```

This is the active runtime behavior today. `POST /actuator-bundles` stores the submitted final targets and immediately attempts one serial write per enabled, connected module. The MotionBatch planner does not currently intercept this path.

## MotionBatch Planner

```mermaid
flowchart TD
    from[From ActuatorStateBundle]
    to[To ActuatorStateBundle]
    planner[MotionBatch]
    steps[Interpolated MotionStep list]
    active[Active step target bundle]
    telemetry[Actual telemetry bundle]
    advance[AdvanceIfActiveStepReached]
    next[Next step active]
    done[Batch finished]

    from --> planner
    to --> planner
    planner --> steps
    steps --> active
    telemetry --> advance
    active --> advance
    advance -->|active targets reached within tolerance| next
    next --> active
    advance -->|last step reached| done
```

MotionBatch is a C# state-machine utility for staged movement. It creates intermediate target bundles between a starting bundle and final bundle, marks one step active, and advances only when telemetry currents match the active step targets within tolerance.

Example with `M01.a1_target` moving from `50` to `150` over four steps:

```text
Step 1: 75
Step 2: 100
Step 3: 125
Step 4: 150
```

Important limitation: MotionBatch does not send serial commands or subscribe to telemetry by itself. Arduino firmware remains unchanged and would still receive normal CSV commands such as `T,75,50,50,50\n` if a future controller loop dispatches each active step.

Current strict advancement rule: all modules in the active step bundle must have non-null current values that match their active targets within tolerance. Before wiring this into real serial operation, decide whether advancement should instead wait only for changed actuators, changed modules, or serial-enabled modules.

## Single-Module Compatibility Dataflow

```mermaid
sequenceDiagram
    participant Client as External Client
    participant API as Minimal API
    participant Store as ActuatorStateStore
    participant Serial as SerialModuleManager
    participant Arduino as M01 Arduino

    Client->>API: GET /actuators
    API->>Store: SnapshotModule(M01)
    Store-->>API: ActuatorState
    API-->>Client: flat four-actuator JSON

    Client->>API: POST /actuators
    API->>Store: ReplaceModule(M01, state)
    API->>Serial: SendTargetsAsync(M01, state)
    alt M01 serial enabled and connected
        Serial->>Arduino: T,a1,a2,a3,a4\n
    else disabled or unavailable
        Serial-->>API: skip serial write
    end
    API->>Store: SnapshotModule(M01)
    API-->>Client: flat four-actuator JSON
```

## Serial Telemetry Dataflow

```mermaid
flowchart LR
    arduino[Arduino module]
    line[JSON telemetry line]
    conn[SerialActuatorConnection]
    manager[SerialModuleManager]
    store[ActuatorStateStore]
    ui[WPF module row]
    api[GET endpoints]

    arduino --> line
    line --> conn
    conn -->|parse ActuatorState| manager
    manager -->|module ID + telemetry| store
    store -->|copy only a*_current into that module| store
    store -->|StateChanged bundle| ui
    store -->|snapshot| api
```

## Serial Toggle Dataflow

```mermaid
flowchart TD
    grid[WPF 10-module grid]
    toggle[SerialEnabled checkbox]
    com[COM port textbox]
    baud[Baud textbox]
    save[Save PortMapping rows]
    db[(SQLite)]
    start[Start button]
    manager[SerialModuleManager]
    skip[Skip disabled rows]
    open[Open enabled COM ports]

    grid --> toggle
    grid --> com
    grid --> baud
    toggle --> save
    com --> save
    baud --> save
    save --> db
    start --> manager
    manager --> skip
    manager --> open
```

## Persistence Dataflow

```mermaid
flowchart TD
    startup[WPF window loaded]
    repo[PortMappingRepository]
    db[(SQLite)]
    seed[Seed M01..M10 if missing]
    rows[WPF module rows]
    runtime[Runtime serial manager]

    startup --> repo
    repo --> db
    db -. missing rows .-> seed
    seed --> db
    repo --> rows
    rows -->|save before start| repo
    rows -->|enabled rows + COM + baud| runtime
```

## Compatibility Rules

- Fixed local API endpoint: `http://127.0.0.1:7500`.
- `GET /actuators` and `POST /actuators` remain the flat four-actuator M01 compatibility API.
- `GET /actuator-bundles` and `POST /actuator-bundles` handle all 10 modules in one payload.
- Each serial command remains CSV: `T,a1,a2,a3,a4\n`.
- Target range remains `0..800`.
- Serial dispatch is controlled by the WPF/SQLite module toggles.
- Disabled or unavailable serial ports do not block API state updates.
- Telemetry updates only `*_current` fields for the source module; targets remain server-authoritative.

## Development Checklist

- Implemented: .NET solution with WPF app, Core domain, Infrastructure adapters, Tests, and generated ClientAPI project.
- Implemented: fixed local API on `127.0.0.1:7500`.
- Implemented: M01 compatibility API at `GET /actuators` and `POST /actuators`.
- Implemented: 10-module bundle API at `GET /actuator-bundles` and `POST /actuator-bundles`.
- Implemented: explicit `Newtonsoft.Json` serialization for API DTOs and generated client behavior.
- Implemented: SQLite-backed module serial mappings for `M01` through `M10`.
- Implemented: WPF module layout ordered by physical module position.
- Implemented: serial writes using archive-compatible CSV command `T,a1,a2,a3,a4\n`.
- Implemented: JSON telemetry parsing that updates only current fields.
- Implemented: MotionBatch interpolation and step-advancement tests.
- Not implemented: MotionBatch integration with API target writes.
- Not implemented: background controller loop that dispatches active MotionBatch steps and advances from live telemetry.
- Not implemented: UI controls for creating, starting, cancelling, or monitoring MotionBatch sequences.
