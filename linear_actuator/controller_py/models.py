import threading
from typing import Any

from pydantic import BaseModel
from pydantic import Field
from pydantic import field_validator


ACTUATOR_COUNT = 4
FIRST_ACTUATOR_ID = 1
DEFAULT_STARTUP_TARGET = 170.0


class ActuatorData(BaseModel):
    """Pure data schema for one actuator.

    This model intentionally contains only serializable fields so it can be
    safely used for JSON serialization, validation, and cross-service exchange.
    """

    id: int | None = None
    current: float | None = None
    target: float | None = None


class ActuatorState(BaseModel):
    """Pure data schema for the full controller actuator state.

    The controller always manages exactly four actuators. This model enforces
    that invariant during validation.
    """

    actuators: list[ActuatorData] = Field(default_factory=list)

    @field_validator("actuators")
    @classmethod
    def validate_count(cls, actuators: list[ActuatorData]) -> list[ActuatorData]:
        if len(actuators) != ACTUATOR_COUNT:
            raise ValueError(f"actuators must contain exactly {ACTUATOR_COUNT} items")
        return actuators


class ThreadSafeActuator:
    """Thread-safe runtime wrapper around a single actuator model.

    Kept for compatibility with existing call sites that operate on one actuator
    object at a time.
    """

    def __init__(self, data: ActuatorData):
        self._data = data
        self._lock = threading.Lock()

    @classmethod
    def from_values(
        cls,
        actuator_id: int,
        current: float,
        target: float,
    ) -> "ThreadSafeActuator":
        model = ActuatorData(
            id=actuator_id,
            current=current,
            target=target,
        )
        return cls(model)

    @classmethod
    def from_json(cls, payload: str) -> "ThreadSafeActuator":
        parsed_model = ActuatorData.model_validate_json(payload)
        return cls(parsed_model)

    def set_current(self, value: float) -> None:
        with self._lock:
            self._data.current = value

    def set_target(self, value: float) -> None:
        with self._lock:
            self._data.target = value

    def set_id(self, value: int) -> None:
        with self._lock:
            self._data.id = value

    def get_current(self) -> float | None:
        with self._lock:
            return self._data.current

    def get_target(self) -> float | None:
        with self._lock:
            return self._data.target

    def get_id(self) -> int | None:
        with self._lock:
            return self._data.id

    def snapshot(self) -> ActuatorData:
        with self._lock:
            return self._data.model_copy(deep=True)

    def to_dict(self, **kwargs: Any) -> dict[str, Any]:
        snapshot_model = self.snapshot()
        return snapshot_model.model_dump(**kwargs)

    def to_json(self, **kwargs: Any) -> str:
        snapshot_model = self.snapshot()
        return snapshot_model.model_dump_json(**kwargs)

    def update_current(self, value: float) -> None:
        self.set_current(value)

    def update_target(self, value: float) -> None:
        self.set_target(value)


actuator = ThreadSafeActuator


class ThreadSafeActuatorState:
    """Thread-safe runtime wrapper around the full controller state.

    - Locking is used for all reads/writes.
    - Snapshots are copied while holding the lock.
    - Serialization is done on copied snapshots outside lock hold time.
    """

    def __init__(self, state: ActuatorState):
        self._state = state
        self._lock = threading.Lock()

    @classmethod
    def from_json(cls, payload: str) -> "ThreadSafeActuatorState":
        parsed_state = ActuatorState.model_validate_json(payload)
        return cls(parsed_state)

    @classmethod
    def from_default_four(cls) -> "ThreadSafeActuatorState":
        actuators: list[ActuatorData] = []

        start_id = FIRST_ACTUATOR_ID
        end_id = FIRST_ACTUATOR_ID + ACTUATOR_COUNT

        for actuator_id in range(start_id, end_id):
            actuators.append(
                ActuatorData(
                    id=actuator_id,
                    current=None,
                    target=DEFAULT_STARTUP_TARGET,
                )
            )

        default_state = ActuatorState(actuators=actuators)
        return cls(default_state)

    def replace_state(self, state: ActuatorState) -> None:
        with self._lock:
            self._state = state

    def replace_from_json(self, payload: str) -> None:
        parsed_state = ActuatorState.model_validate_json(payload)
        self.replace_state(parsed_state)

    def snapshot(self) -> ActuatorState:
        with self._lock:
            return self._state.model_copy(deep=True)

    def to_dict(self, **kwargs: Any) -> dict[str, Any]:
        snapshot_state = self.snapshot()
        return snapshot_state.model_dump(**kwargs)

    def to_json(self, **kwargs: Any) -> str:
        snapshot_state = self.snapshot()
        return snapshot_state.model_dump_json(**kwargs)
