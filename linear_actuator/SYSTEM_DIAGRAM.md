# Linear Actuator System Diagram

This rewrite is a single Windows WPF executable that hosts the local WebAPI, SQLite configuration storage, in-memory actuator state, and Arduino serial communication in one process.

## Component View

```mermaid
flowchart TB
    subgraph exe[LinearActuator.App single exe]
        ui[WPF UI]
        api[Embedded ASP.NET Core Minimal API]
        state[ActuatorStateStore]
        db[(SQLite database)]
        repo[PortMappingRepository]
        serial[SerialActuatorConnection]
    end

    grasshopper[Grasshopper / External HTTP Client]
    arduino[Arduino Mega Firmware]

    ui -->|start/stop, port settings| api
    ui -->|load/save mapping| repo
    repo --> db
    ui -->|display current/target state| state

    grasshopper -->|GET /actuators| api
    grasshopper -->|POST /actuators| api
    api -->|read/replace state| state
    api -->|target command| serial
    serial -->|CSV: T,a1,a2,a3,a4| arduino
    arduino -->|JSON telemetry lines| serial
    serial -->|update current fields only| state
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

    app -. owns .-> wpf[WPF shell]
    core -. owns .-> model[DTOs, state store, serial protocol rules]
    infra -. owns .-> integrations[SQLite, EF Core, serial port, embedded API host]
```

## API Request Flow

```mermaid
sequenceDiagram
    participant Client as External HTTP Client
    participant API as Minimal API
    participant State as ActuatorStateStore
    participant Serial as SerialActuatorConnection
    participant Arduino as Arduino Firmware

    Client->>API: GET /actuators
    API->>State: Snapshot()
    State-->>API: current state
    API-->>Client: flat actuator JSON

    Client->>API: POST /actuators flat actuator JSON
    API->>State: ReplaceState(request)
    API->>Serial: SendTargetsAsync(state)
    alt Serial connected
        Serial->>Arduino: T,a1,a2,a3,a4\n
    else Serial unavailable or write fails
        API->>API: keep server-side state anyway
    end
    API->>State: Snapshot()
    API-->>Client: flat actuator JSON
```

## Serial Telemetry Flow

```mermaid
sequenceDiagram
    participant Arduino as Arduino Firmware
    participant Serial as SerialActuatorConnection
    participant Protocol as SerialProtocol
    participant State as ActuatorStateStore
    participant UI as WPF UI

    Arduino->>Serial: JSON line with a*_current and a*_target
    Serial->>Protocol: ParseTelemetry(line)
    alt valid actuator JSON
        Protocol-->>Serial: ActuatorState
        Serial->>State: UpdateCurrents(telemetry)
        State-->>UI: StateChanged event
        UI->>UI: refresh current/target display
    else invalid JSON or Arduino error
        Serial-->>UI: MessageReceived
        UI->>UI: update status text
    end
```

## SQLite Configuration Flow

```mermaid
flowchart TD
    startup[WPF window loaded]
    load[LoadOrCreateDefaultAsync]
    sqlite[(linear-actuator.db)]
    form[COM port / API port / baud fields]
    start[Start button]
    save[SaveAsync]
    api[Start embedded API]
    serial[Open serial port if COM port is set]

    startup --> load
    load --> sqlite
    load --> form
    start --> save
    save --> sqlite
    start --> serial
    start --> api
```

## Dataflow Diagrams

### Startup Dataflow

```mermaid
flowchart LR
    launch[User launches LinearActuator.App.exe]
    ui[WPF MainWindow]
    repo[PortMappingRepository]
    db[(SQLite: linear-actuator.db)]
    defaults[Default mapping values]
    fields[UI fields]

    launch --> ui
    ui -->|request saved mapping| repo
    repo -->|query first mapping| db
    db -->|mapping exists| repo
    db -. no rows .-> defaults
    defaults -->|insert API01 / COM4 / 7500 / 9600| db
    repo -->|PortMapping| fields
```

### Target Command Dataflow

```mermaid
flowchart LR
    client[External client]
    request[POST /actuators JSON]
    api[Minimal API]
    store[ActuatorStateStore]
    protocol[SerialProtocol]
    serial[SerialActuatorConnection]
    arduino[Arduino]
    response[Response JSON]

    client --> request
    request -->|flat a*_current / a*_target body| api
    api -->|replace complete state| store
    api -->|same request state| protocol
    protocol -->|validate targets 0..800| protocol
    protocol -->|T,a1,a2,a3,a4\n| serial
    serial -->|write line if connected| arduino
    store -->|snapshot| response
    response --> client
```

### Telemetry Dataflow

```mermaid
flowchart LR
    arduino[Arduino]
    line[JSON line]
    serial[SerialActuatorConnection read loop]
    protocol[SerialProtocol.ParseTelemetry]
    telemetry[ActuatorState telemetry]
    store[ActuatorStateStore]
    ui[WPF current/target display]
    api[GET /actuators]

    arduino --> line
    line --> serial
    serial --> protocol
    protocol --> telemetry
    telemetry -->|copy only a*_current| store
    store -->|StateChanged| ui
    store -->|Snapshot| api
```

### Server-Authoritative Target Dataflow

```mermaid
flowchart TD
    post[POST /actuators]
    postedTargets[Posted a*_target values]
    store[(In-memory state)]
    telemetry[Arduino telemetry]
    telemetryTargets[Telemetry a*_target values]
    currents[Telemetry a*_current values]
    snapshot[GET /actuators snapshot]

    post --> postedTargets
    postedTargets -->|replace targets| store
    telemetry --> telemetryTargets
    telemetry --> currents
    telemetryTargets -. ignored for server truth .-> store
    currents -->|update currents only| store
    store --> snapshot
```

### Persistence Dataflow

```mermaid
flowchart TD
    settings[UI mapping fields]
    mapping[PortMapping entity]
    sqlite[(SQLite)]
    runtime[Runtime services]
    api[ActuatorApiHost]
    serial[SerialActuatorConnection]
    state[ActuatorStateStore]

    settings --> mapping
    mapping -->|save COM/API/baud config| sqlite
    sqlite -->|load config on startup| mapping
    mapping --> runtime
    runtime --> api
    runtime --> serial
    state -. live actuator state is not persisted yet .-> runtime
```

## Preserved Archive Contract

```mermaid
classDiagram
    class ActuatorState {
        double? a1_current
        int? a1_target = 50
        double? a2_current
        int? a2_target = 50
        double? a3_current
        int? a3_target = 50
        double? a4_current
        int? a4_target = 50
    }

    class SerialProtocol {
        FormatTargetCommand(ActuatorState) string
        ParseTelemetry(string) ActuatorState?
        IsValidTarget(int?) bool
    }

    ActuatorStateStore --> ActuatorState
    SerialProtocol --> ActuatorState
```

Key compatibility rules:

- HTTP JSON remains the flat four-actuator schema.
- Default target remains `50`.
- Target range remains `0..800`.
- Serial commands to Arduino are CSV, not JSON.
- Arduino telemetry updates only `*_current`; server targets remain authoritative.
