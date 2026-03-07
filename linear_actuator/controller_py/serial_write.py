from models import ActuatorState
from models import ThreadSafeActuatorState


def write_payload_json(ser, payload: str) -> None:
    ser.write(f"{payload}\n".encode("utf-8"))
    ser.flush()


def _target_for_csv(value: int | None) -> int:
    if value is None:
        raise ValueError("target value must be int for CSV command")
    return int(value)


def write_targets_csv(ser, state: ActuatorState) -> None:
    command = (
        f"T,{_target_for_csv(state.a1_target)},{_target_for_csv(state.a2_target)},"
        f"{_target_for_csv(state.a3_target)},{_target_for_csv(state.a4_target)}"
    )
    ser.write(f"{command}\n".encode("utf-8"))
    ser.flush()


def write_full_state(ser, state_store: ThreadSafeActuatorState) -> None:
    payload = state_store.to_json()
    write_payload_json(ser, payload)
