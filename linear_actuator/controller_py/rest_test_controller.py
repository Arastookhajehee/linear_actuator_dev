from fastapi import FastAPI
from fastapi.responses import Response

from models import ActuatorState
from models import ThreadSafeActuatorState


def build_app() -> FastAPI:
    app = FastAPI(title="Linear Actuator REST Test Controller")
    state_store = ThreadSafeActuatorState.from_default_four()

    @app.get("/actuators")
    def get_actuators() -> Response:
        return Response(content=state_store.to_json(), media_type="application/json")

    @app.post("/actuators")
    def post_actuators(state: ActuatorState) -> Response:
        state_store.replace_state(state)
        return Response(content=state_store.to_json(), media_type="application/json")

    return app


def start(host: str, api_port: int) -> None:
    app = build_app()
    uvicorn_module = __import__("uvicorn")
    uvicorn_module.run(app, host=host, port=api_port)
