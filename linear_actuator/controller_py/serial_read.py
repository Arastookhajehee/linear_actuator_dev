from models import ActuatorState
from models import ThreadSafeActuatorState


def read_forever(ser, state_store: ThreadSafeActuatorState) -> None:
    while True:
        raw = ser.readline()
        if not raw:
            continue

        line = raw.decode("utf-8", errors="replace").strip()
        if not line:
            continue

        try:
            state = ActuatorState.model_validate_json(line)
        except Exception:
            continue

        state_store.replace_state(state)
