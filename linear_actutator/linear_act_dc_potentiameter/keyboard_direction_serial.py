#!/usr/bin/env python3
"""Send -1/0/1 direction commands to Arduino from arrow keys.

Left arrow  -> -1 (backward)
Down arrow  ->  0 (stop)
Right arrow ->  1 (forward)
Q           -> quit (sends stop before exit)
"""

import argparse
import time
import msvcrt

import serial


LEFT_KEY = b"K"
DOWN_KEY = b"P"
RIGHT_KEY = b"M"


def send_direction(ser: serial.Serial, direction: int) -> None:
    ser.write(f"{direction}\n".encode("ascii"))
    ser.flush()
    print(f"sent {direction}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Arrow-key serial direction sender")
    parser.add_argument("--port", required=True, help="Arduino serial port, e.g. COM5")
    parser.add_argument("--baud", type=int, default=9600, help="Serial baud rate")
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.2)

    # Opening serial often resets Arduino; wait before first command.
    time.sleep(2.0)

    current_direction = None
    print("Controls: Left=-1  Down=0  Right=1  Q=quit")

    try:
        send_direction(ser, 0)
        current_direction = 0

        while True:
            if msvcrt.kbhit():
                ch = msvcrt.getch()

                if ch in (b"\x00", b"\xe0"):
                    key = msvcrt.getch()
                    if key == LEFT_KEY and current_direction != -1:
                        send_direction(ser, -1)
                        current_direction = -1
                    elif key == DOWN_KEY and current_direction != 0:
                        send_direction(ser, 0)
                        current_direction = 0
                    elif key == RIGHT_KEY and current_direction != 1:
                        send_direction(ser, 1)
                        current_direction = 1
                elif ch in (b"q", b"Q"):
                    break

            time.sleep(0.01)

    finally:
        try:
            send_direction(ser, 0)
        finally:
            ser.close()


if __name__ == "__main__":
    main()
