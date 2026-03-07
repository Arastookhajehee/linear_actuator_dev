import threading
from typing import Any

from pydantic import BaseModel


DEFAULT_STARTUP_TARGET = 100


class ActuatorState(BaseModel):
    """Flat, serializable state for exactly four actuators.

    This schema intentionally avoids lists/arrays so serialization is a simple
    key-value JSON object that can be consumed easily on the Arduino side.
    """

    a1_current: float | None = None
    a1_target: int | None = int(DEFAULT_STARTUP_TARGET)

    a2_current: float | None = None
    a2_target: int | None = int(DEFAULT_STARTUP_TARGET)

    a3_current: float | None = None
    a3_target: int | None = int(DEFAULT_STARTUP_TARGET)

    a4_current: float | None = None
    a4_target: int | None = int(DEFAULT_STARTUP_TARGET)


class ThreadSafeActuatorState:
    """Thread-safe runtime wrapper around the flat actuator state model."""

    def __init__(self, state: ActuatorState):
        self._state = state
        self._lock = threading.Lock()

    @classmethod
    def from_json(cls, payload: str) -> "ThreadSafeActuatorState":
        parsed_state = ActuatorState.model_validate_json(payload)
        return cls(parsed_state)

    @classmethod
    def from_default_four(cls) -> "ThreadSafeActuatorState":
        default_state = ActuatorState(
            a1_current=None,
            a1_target=DEFAULT_STARTUP_TARGET,
            a2_current=None,
            a2_target=DEFAULT_STARTUP_TARGET,
            a3_current=None,
            a3_target=DEFAULT_STARTUP_TARGET,
            a4_current=None,
            a4_target=DEFAULT_STARTUP_TARGET,
        )
        return cls(default_state)

    def replace_state(self, state: ActuatorState) -> None:
        with self._lock:
            self._state = state

    def replace_from_json(self, payload: str) -> None:
        parsed_state = ActuatorState.model_validate_json(payload)
        self.replace_state(parsed_state)

    def update_currents_from_state(self, incoming_state: ActuatorState) -> None:
        """Update only current fields from telemetry.

        Targets remain server-authoritative and are not overwritten by telemetry.
        """

        with self._lock:
            self._state.a1_current = incoming_state.a1_current
            self._state.a2_current = incoming_state.a2_current
            self._state.a3_current = incoming_state.a3_current
            self._state.a4_current = incoming_state.a4_current

    def snapshot(self) -> ActuatorState:
        with self._lock:
            return self._state.model_copy(deep=True)

    def to_dict(self, **kwargs: Any) -> dict[str, Any]:
        snapshot_state = self.snapshot()
        return snapshot_state.model_dump(**kwargs)

    def to_json(self, **kwargs: Any) -> str:
        snapshot_state = self.snapshot()
        return snapshot_state.model_dump_json(**kwargs)
