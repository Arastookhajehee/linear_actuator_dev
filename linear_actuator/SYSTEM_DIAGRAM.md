# Linear Actuator System Diagram

The rewrite is a single Windows WPF executable. It hosts the local WebAPI, SQLite serial-port configuration, in-memory state for 10 actuator modules, and optional serial connections to Arduino controllers.

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
    tests[LinearActuator.Tests]

    app --> core
    app --> infra
    infra --> core
    tests --> core
    tests --> infra

    app -. owns .-> wpf[WPF shell and module grid]
    core -. owns .-> domain[ActuatorState, ActuatorStateBundle, state store, protocol rules]
    infra -. owns .-> adapters[SQLite, EF Core, serial ports, embedded API host]
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
