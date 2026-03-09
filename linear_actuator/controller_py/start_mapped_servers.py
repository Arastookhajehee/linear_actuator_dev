import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Start multiple mapped actuator servers")
    parser.add_argument(
        "--map-file",
        default="port_map.json",
        help="Path to JSON mapping file (default: port_map.json)",
    )
    parser.add_argument("--api-host", default="127.0.0.1", help="FastAPI bind host")
    parser.add_argument("--baud", default=9600, type=int, help="Serial baud rate")
    parser.add_argument(
        "--api-test-only",
        action="store_true",
        help="Run all mapped servers in API test-only mode (no serial)",
    )
    parser.add_argument(
        "--only",
        nargs="*",
        help="Optional subset of mapping keys to launch (for example: API00 API01)",
    )
    return parser.parse_args()


def load_mapping(map_file: Path) -> dict[str, dict[str, Any]]:
    with map_file.open("r", encoding="utf-8") as f:
        payload = json.load(f)

    if not isinstance(payload, dict):
        raise ValueError("Mapping file must contain a JSON object at top level")

    normalized: dict[str, dict[str, Any]] = {}
    for key, value in payload.items():
        if not isinstance(value, dict):
            raise ValueError(f"Mapping for {key!r} must be an object")
        if "api_port" not in value:
            raise ValueError(f"Mapping for {key!r} is missing 'api_port'")
        normalized[key] = value

    return normalized


def main() -> None:
    args = parse_args()

    this_dir = Path(__file__).resolve().parent
    map_path = Path(args.map_file)
    if not map_path.is_absolute():
        map_path = this_dir / map_path

    mappings = load_mapping(map_path)

    selected_keys = args.only if args.only else sorted(mappings.keys())

    unknown_keys = [k for k in selected_keys if k not in mappings]
    if unknown_keys:
        raise SystemExit(f"Unknown mapping keys: {', '.join(unknown_keys)}")

    used_api_ports: set[int] = set()
    launch_plan: list[tuple[str, str | None, int]] = []

    for key in selected_keys:
        record = mappings[key]
        api_port_value = record.get("api_port")
        if api_port_value is None:
            raise SystemExit(f"Missing api_port for {key!r}")
        try:
            api_port = int(api_port_value)
        except Exception as exc:
            raise SystemExit(f"Invalid api_port for {key!r}: {api_port_value!r}") from exc

        if api_port in used_api_ports:
            raise SystemExit(f"Duplicate api_port found: {api_port}")
        used_api_ports.add(api_port)

        com_port = record.get("com_port")
        com_port_str = str(com_port) if com_port is not None else None

        if not args.api_test_only and (com_port_str is None or not com_port_str.strip()):
            raise SystemExit(f"Missing com_port for {key!r} in serial mode")

        launch_plan.append((key, com_port_str, api_port))

    main_py = this_dir / "main.py"
    processes: list[tuple[str, subprocess.Popen[bytes]]] = []

    for key, com_port, api_port in launch_plan:
        command = [
            sys.executable,
            str(main_py),
            "--api-host",
            args.api_host,
            "--api-port",
            str(api_port),
        ]

        if args.api_test_only:
            command.append("--api-test-only")
        else:
            command.extend(["--port", com_port or "", "--baud", str(args.baud)])

        process = subprocess.Popen(command, cwd=str(this_dir))
        processes.append((key, process))

        mode = "api-test-only" if args.api_test_only else "serial"
        print(
            f"Started {key}: pid={process.pid}, mode={mode}, "
            f"com_port={com_port}, api_port={api_port}"
        )

    print(f"Launched {len(processes)} server process(es).")


if __name__ == "__main__":
    main()
