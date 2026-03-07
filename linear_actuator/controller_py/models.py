import threading
from typing import Any

from pydantic import BaseModel


class ActuatorData(BaseModel):
    id: int
    current: float
    target: float


class ThreadSafeActuator:
    def __init__(self, data: ActuatorData):
        self._data = data
        self._lock = threading.Lock()

    @classmethod
    def from_values(
        cls, actuator_id: int, current: float, target: float
    ) -> "ThreadSafeActuator":
        return cls(ActuatorData(id=actuator_id, current=current, target=target))

    @classmethod
    def from_json(cls, payload: str) -> "ThreadSafeActuator":
        data = ActuatorData.model_validate_json(payload)
        return cls(data)

    def set_current(self, value: float) -> None:
        with self._lock:
            self._data.current = value

    def set_target(self, value: float) -> None:
        with self._lock:
            self._data.target = value

    def set_id(self, value: int) -> None:
        with self._lock:
            self._data.id = value

    def get_current(self) -> float:
        with self._lock:
            return self._data.current

    def get_target(self) -> float:
        with self._lock:
            return self._data.target

    def get_id(self) -> int:
        with self._lock:
            return self._data.id

    def snapshot(self) -> ActuatorData:
        with self._lock:
            return self._data.model_copy(deep=True)

    def to_dict(self, **kwargs: Any) -> dict[str, Any]:
        state = self.snapshot()
        return state.model_dump(**kwargs)

    def to_json(self, **kwargs: Any) -> str:
        state = self.snapshot()
        return state.model_dump_json(**kwargs)

    def update_current(self, value: float) -> None:
        self.set_current(value)

    def update_target(self, value: float) -> None:
        self.set_target(value)


actuator = ThreadSafeActuator
