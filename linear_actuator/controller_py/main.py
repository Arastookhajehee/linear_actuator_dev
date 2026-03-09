import argparse

from controller import start as start_controller
from rest_test_controller import start as start_rest_test


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Linear actuator controller service")
    parser.add_argument("--port", help="Serial port, for example COM5")
    parser.add_argument("--baud", default=9600, type=int, help="Serial baud rate")
    parser.add_argument("--api-host", default="127.0.0.1", help="FastAPI bind host")
    parser.add_argument("--api-port", default=8008, type=int, help="FastAPI bind port")
    parser.add_argument(
        "--api-test-only",
        action="store_true",
        help="Run API test-only controller without serial/Arduino",
    )
    parser.add_argument(
        "--rest-test",
        action="store_true",
        help="Alias for --api-test-only",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.api_test_only or args.rest_test:
        start_rest_test(host=args.api_host, api_port=args.api_port)
        return

    if args.port is None:
        raise SystemExit("--port is required unless --api-test-only/--rest-test is used")

    start_controller(
        serial_port=args.port,
        baud_rate=args.baud,
        host=args.api_host,
        api_port=args.api_port,
    )


if __name__ == "__main__":
    main()
