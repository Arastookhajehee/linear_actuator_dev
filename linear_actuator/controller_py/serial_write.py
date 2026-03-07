from models import ThreadSafeActuatorState


def write_full_state(ser, state_store: ThreadSafeActuatorState) -> None:
    payload = state_store.to_json()
    ser.write(f"{payload}\n".encode("utf-8"))
    ser.flush()
