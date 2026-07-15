import argparse
from pathlib import Path

from plot_joint_positions import load_path, load_urdf_joint_info, plot_joint_data


def main() -> None:
    parser = argparse.ArgumentParser(description="Batch export path JSON files to JPG plots.")
    parser.add_argument("--input-dir", type=Path, default=Path("plot_json"), help="Folder with JSON files")
    parser.add_argument("--output-dir", type=Path, default=Path("plot_json/jpg"), help="Folder for JPG files")
    parser.add_argument("--urdf", type=Path, default=Path("my_robot.urdf"), help="URDF file")
    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    joint_info = load_urdf_joint_info(args.urdf)

    json_files = sorted(args.input_dir.glob("*.json"))
    if not json_files:
        print(f"No JSON files found in {args.input_dir}")
        return

    for json_file in json_files:
        output_file = args.output_dir / f"{json_file.stem}.jpg"
        if output_file.exists():
            print(f"Skipped existing: {output_file}")
            continue

        joint_names, times, positions = load_path(json_file)
        plot_joint_data(joint_names, times, positions, joint_info, output_file)
        print(f"Saved: {output_file}")

    print(f"Exported {len(json_files)} JPG files to {args.output_dir}")


if __name__ == "__main__":
    main()
