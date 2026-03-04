#!/usr/bin/env python3
"""Send 4-actuator target values to Arduino over JSON serial.

Type a JSON array like [100,200,300,400] and press Enter.
Type q and press Enter to quit.
"""

import argparse
import json
import time
from typing import Any, List

import serial
from pydantic import ValidationError

from message import ErrorMessage, Message


def parse_target_array(raw_input: str) -> List[int]:
    try:
        parsed = json.loads(raw_input)
    except json.JSONDecodeError as exc:
        raise ValueError("Input must be valid JSON like [100,200,300,400].") from exc

    if not isinstance(parsed, list):
        raise ValueError("Input must be a JSON array like [100,200,300,400].")

    if len(parsed) != 4:
        raise ValueError("Exactly 4 target values are required.")

    targets: List[int] = []
    for value in parsed:
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError("Targets must be numbers in the range 0..1023.")
        target = float(value)
        if target < 0 or target > 1023:
            raise ValueError("Each target must be between 0 and 1023.")
        targets.append(int(target))

    return targets


def send_message(ser: serial.Serial, message: Message) -> None:
    payload_object = {
        "id": int(message.id),
        "lin_acts": [int(value) for value in message.lin_acts],
        "sensor_values": [float(value) for value in message.sensor_values],
    }
    payload = json.dumps(payload_object, separators=(",", ":"))
    ser.write(f"{payload}\n".encode("utf-8"))
    ser.flush()
    print(f"sent id={message.id} lin_acts={message.lin_acts}")


def print_parsed_line(parsed: Any) -> None:
    if not isinstance(parsed, dict):
        print(f"arduino: unexpected payload: {parsed}")
        return

    if "error" in parsed:
        try:
            if hasattr(ErrorMessage, "model_validate"):
                error_message = ErrorMessage.model_validate(parsed)
            else:
                error_message = ErrorMessage.parse_obj(parsed)
            if error_message.details:
                print(
                    "arduino error: "
                    f"id={error_message.id} error={error_message.error} "
                    f"details={error_message.details}"
                )
            else:
                print(f"arduino error: id={error_message.id} error={error_message.error}")
        except ValidationError:
            print(f"arduino error (untyped): {parsed}")
        return

    try:
        if hasattr(Message, "model_validate"):
            message = Message.model_validate(parsed)
        else:
            message = Message.parse_obj(parsed)
        print(
            "arduino: "
            f"id={message.id} "
            f"lin_acts={message.lin_acts} "
            f"sensor_values={message.sensor_values}"
        )
    except ValidationError:
        print(f"arduino: unknown json={parsed}")


def print_pending_lines(ser: serial.Serial, window_seconds: float = 0.6) -> None:
    deadline = time.time() + window_seconds
    while time.time() < deadline:
        raw = ser.readline()
        if not raw:
            break
        line = raw.decode("utf-8", errors="replace").strip()
        if not line:
            continue
        try:
            parsed = json.loads(line)
        except json.JSONDecodeError:
            print(f"arduino: non-json line ignored: {line}")
            continue
        print_parsed_line(parsed)


def main() -> None:
    parser = argparse.ArgumentParser(description="Interactive JSON serial sender")
    parser.add_argument("--port", required=True, help="Arduino serial port, e.g. COM5")
    parser.add_argument("--baud", type=int, default=9600, help="Serial baud rate")
    parser.add_argument("--timeout", type=float, default=0.2, help="Read timeout seconds")
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    next_message_id = 1

    # Opening serial often resets Arduino; wait before first command.
    time.sleep(2.0)

    try:
        print_pending_lines(ser)
        print("Enter targets as [a,b,c,d] in range 0..1023. Type q to quit.")

        while True:
            try:
                user_input = input("target> ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break

            if user_input.lower() == "q":
                break

            try:
                targets = parse_target_array(user_input)
            except ValueError:
                print("Please enter targets as [a,b,c,d] with each value from 0 to 1023.")
                continue

            command = Message(
                id=next_message_id,
                lin_acts=[float(value) for value in targets],
                sensor_values=[0.0, 0.0, 0.0, 0.0],
            )
            send_message(ser, command)
            next_message_id += 1
            print_pending_lines(ser)

    finally:
        ser.close()


if __name__ == "__main__":
    main()
