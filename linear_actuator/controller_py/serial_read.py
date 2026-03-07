import json

from models import ActuatorState
from models import ThreadSafeActuatorState

def _print_message_without_current(state: ActuatorState) -> None:
    sanitized_payload = {
        "a1_target": state.a1_target,
        "a2_target": state.a2_target,
        "a3_target": state.a3_target,
        "a4_target": state.a4_target,
    }

    print(f"[serial] {json.dumps(sanitized_payload)}")


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
            try:
                payload = json.loads(line)
            except Exception:
                print(f"[serial] {line}")
                continue

            if isinstance(payload, dict) and "error" in payload:
                print(f"[serial][arduino-error] {payload}")
            else:
                print(f"[serial] {payload}")
            continue

        _print_message_without_current(state)
        state_store.update_currents_from_state(state)
