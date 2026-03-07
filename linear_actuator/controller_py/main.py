import argparse

from controller import start


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Linear actuator controller service")
    parser.add_argument("--port", required=True, help="Serial port, for example COM5")
    parser.add_argument("--baud", required=True, type=int, help="Serial baud rate")
    parser.add_argument("--api-host", default="127.0.0.1", help="FastAPI bind host")
    parser.add_argument("--api-port", default=8008, type=int, help="FastAPI bind port")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    start(
        serial_port=args.port,
        baud_rate=args.baud,
        host=args.api_host,
        api_port=args.api_port,
    )


if __name__ == "__main__":
    main()
