import argparse
import json
from pathlib import Path


def count_steps(path_file: Path) -> int:
    with path_file.open("r", encoding="utf-8-sig") as file:
        data = json.load(file)

    axes_path = data.get("axes_path")
    if not isinstance(axes_path, dict):
        raise ValueError(f"Missing or invalid 'axes_path' in {path_file}")

    points = axes_path.get("points")
    if not isinstance(points, list):
        raise ValueError(f"Missing or invalid 'axes_path.points' in {path_file}")

    return len(points)


def main() -> None:
    parser = argparse.ArgumentParser(description="Count total steps in a path JSON file.")
    parser.add_argument("path", nargs="?", type=Path, default=Path("path.json"), help="Path JSON file")
    args = parser.parse_args()

    total_steps = count_steps(args.path)
    print(f"Total steps: {total_steps}")


if __name__ == "__main__":
    main()
