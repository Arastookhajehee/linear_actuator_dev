#!/usr/bin/env python3
"""Send target position values to Arduino over serial.

Type a number (0-1023) and press Enter to set a new target.
Type q and press Enter to quit.
"""

import argparse
import time

import serial


def send_target(ser: serial.Serial, target: int) -> None:
    ser.write(f"{target}\n".encode("ascii"))
    ser.flush()
    print(f"sent {target}")


def print_pending_lines(ser: serial.Serial, window_seconds: float = 0.6) -> None:
    deadline = time.time() + window_seconds
    while time.time() < deadline:
        raw = ser.readline()
        if not raw:
            break
        message = raw.decode("utf-8", errors="replace").strip()
        if message:
            print(f"arduino: {message}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Interactive serial target sender")
    parser.add_argument("--port", required=True, help="Arduino serial port, e.g. COM5")
    parser.add_argument("--baud", type=int, default=9600, help="Serial baud rate")
    parser.add_argument("--timeout", type=float, default=0.2, help="Read timeout seconds")
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=args.timeout)

    # Opening serial often resets Arduino; wait before first command.
    time.sleep(2.0)

    try:
        print_pending_lines(ser)
        print("Enter target 0-1023, then Enter. Type q to quit.")

        while True:
            try:
                user_input = input("target> ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break

            if user_input.lower() == "q":
                break

            try:
                target = int(user_input)
            except ValueError:
                print("Please enter an integer from 0 to 1023, or q to quit.")
                continue

            if target < 0 or target > 1023:
                print("Target must be between 0 and 1023.")
                continue

            send_target(ser, target)
            print_pending_lines(ser)

    finally:
        ser.close()


if __name__ == "__main__":
    main()
