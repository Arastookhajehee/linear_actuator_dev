import json
import logging

from models import ActuatorState
from models import ThreadSafeActuatorState


LOGGER = logging.getLogger(__name__)


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
                LOGGER.debug("Ignored non-JSON serial line: %s", line)
                continue

            if isinstance(payload, dict) and "error" in payload:
                LOGGER.warning("Arduino reported error: %s", payload.get("error"))
            else:
                LOGGER.debug("Ignored JSON line with unexpected schema: %s", payload)
            continue

        state_store.update_currents_from_state(state)
