import logging
import threading

from fastapi import FastAPI
import serial

from models import ActuatorState
from models import ThreadSafeActuatorState
from serial_read import read_forever
from serial_write import write_targets_csv


LOGGER = logging.getLogger(__name__)


def build_app(serial_port: str, baud_rate: int) -> FastAPI:
    app = FastAPI(title="Linear Actuator Controller")
    state_store = ThreadSafeActuatorState.from_default_four()

    @app.on_event("startup")
    def startup() -> None:
        ser = serial.Serial(serial_port, baud_rate, timeout=1)
        app.state.serial = ser
        app.state.reader_thread = threading.Thread(
            target=read_forever,
            args=(ser, state_store),
            daemon=True,
        )
        app.state.reader_thread.start()

    @app.on_event("shutdown")
    def shutdown() -> None:
        ser = getattr(app.state, "serial", None)
        if ser is not None and ser.is_open:
            ser.close()

    @app.get("/actuators", response_model=ActuatorState)
    def get_actuators() -> ActuatorState:
        return state_store.snapshot()

    @app.post("/actuators", response_model=ActuatorState)
    def post_actuators(state: ActuatorState) -> ActuatorState:
        # Optimistic server truth: update server state first.
        state_store.replace_state(state)

        ser = getattr(app.state, "serial", None)
        if ser is None or not ser.is_open:
            LOGGER.warning("Serial connection unavailable; state updated server-side only")
            return state_store.snapshot()

        try:
            write_targets_csv(ser, state)
        except Exception:
            LOGGER.exception("Failed to write actuator state to serial")

        return state_store.snapshot()

    return app


def start(serial_port: str, baud_rate: int, host: str, api_port: int) -> None:
    app = build_app(serial_port=serial_port, baud_rate=baud_rate)
    uvicorn_module = __import__("uvicorn")
    uvicorn_module.run(app, host=host, port=api_port)
