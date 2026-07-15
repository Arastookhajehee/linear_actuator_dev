import argparse
import json
import math
import xml.etree.ElementTree as ET
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

plt.rcParams["font.family"] = ["Yu Gothic", "Meiryo", "MS Gothic", "DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False


def load_path(path_file: Path) -> tuple[list[str], list[float], list[list[float]]]:
    with path_file.open("r", encoding="utf-8-sig") as file:
        data = json.load(file)

    axes_path = data.get("axes_path", {})
    joint_names = axes_path.get("joint_names", [])
    points = axes_path.get("points", [])

    if not joint_names or not points:
        raise ValueError(f"Invalid or empty axes_path in {path_file}")

    times = []
    positions = [[] for _ in joint_names]

    for index, point in enumerate(points):
        time_from_start = point.get("time_from_start_sec")
        point_positions = point.get("positions")

        if not isinstance(time_from_start, (int, float)):
            raise ValueError(f"Point {index} has invalid time_from_start_sec")
        if not isinstance(point_positions, list) or len(point_positions) != len(joint_names):
            raise ValueError(
                f"Point {index} has {len(point_positions) if isinstance(point_positions, list) else 'invalid'} "
                f"positions but expected {len(joint_names)}"
            )

        times.append(float(time_from_start))
        for joint_index, position in enumerate(point_positions):
            positions[joint_index].append(float(position))

    return joint_names, times, positions


def load_urdf_joint_info(urdf_file: Path) -> dict[str, dict]:
    tree = ET.parse(urdf_file)
    root = tree.getroot()

    joint_info = {}
    for joint in root.findall("joint"):
        name = joint.get("name")
        joint_type = joint.get("type", "unknown")
        if not name:
            continue

        limit = joint.find("limit")
        lower = float(limit.get("lower")) if limit is not None and limit.get("lower") is not None else None
        upper = float(limit.get("upper")) if limit is not None and limit.get("upper") is not None else None

        joint_info[name] = {
            "joint_type": joint_type,
            "lower": lower,
            "upper": upper,
        }

    return joint_info


def to_display_units(values: list[float], joint_type: str, mode: str) -> tuple[list[float], str]:
    if mode == "si":
        if joint_type == "prismatic":
            return values, "m"
        return values, "rad"

    if joint_type == "prismatic":
        return [value * 1000.0 for value in values], "mm"

    return [math.degrees(value) for value in values], "deg"


def convert_limit(limit_value: float | None, joint_type: str, mode: str) -> float | None:
    if limit_value is None:
        return None
    if mode == "si":
        return limit_value
    if joint_type == "prismatic":
        return limit_value * 1000.0
    return math.degrees(limit_value)


def build_out_of_limit_mask(values: list[float], lower: float | None, upper: float | None) -> list[bool]:
    mask = []
    for value in values:
        if lower is not None and value < lower:
            mask.append(True)
        elif upper is not None and value > upper:
            mask.append(True)
        else:
            mask.append(False)
    return mask


def plot_joint_data(
    joint_names: list[str],
    times: list[float],
    positions: list[list[float]],
    joint_info: dict[str, dict],
    output_file: Path,
) -> None:
    row_count = len(joint_names)
    fig_height = max(2 * row_count + 2, 8)
    fig, axes = plt.subplots(row_count, 2, figsize=(16, fig_height), sharex=True)

    if row_count == 1:
        axes = [axes]

    fig.suptitle("時刻に対するジョイント位置（URDF制限重ね表示）", fontsize=16)

    for row_index, joint_name in enumerate(joint_names):
        metadata = joint_info.get(joint_name, {})
        joint_type = metadata.get("joint_type", "unknown")
        raw_values = positions[row_index]

        for col_index, mode in enumerate(("si", "converted")):
            ax = axes[row_index][col_index]
            display_values, display_unit = to_display_units(raw_values, joint_type, mode)
            lower = convert_limit(metadata.get("lower"), joint_type, mode)
            upper = convert_limit(metadata.get("upper"), joint_type, mode)
            outside_mask = build_out_of_limit_mask(display_values, lower, upper)

            ax.plot(
                times,
                display_values,
                color="#1f77b4",
                linewidth=0.7,
                linestyle="-",
                drawstyle="default",
                marker="o",
                markersize=1.2,
                markevery=1,
                antialiased=False,
                label="位置（ポリライン）",
            )

            y_candidates = list(display_values)
            if lower is not None:
                y_candidates.append(lower)
            if upper is not None:
                y_candidates.append(upper)
            y_min = min(y_candidates)
            y_max = max(y_candidates)
            if math.isclose(y_min, y_max):
                pad = 1e-3
            else:
                pad = (y_max - y_min) * 0.05
            ax.set_ylim(y_min - pad, y_max + pad)

            if lower is not None:
                ax.axhline(lower, color="#2ca02c", linestyle="--", linewidth=1.0, label="下限")
            if upper is not None:
                ax.axhline(upper, color="#ff7f0e", linestyle="--", linewidth=1.0, label="上限")

            out_times = [time for time, is_outside in zip(times, outside_mask) if is_outside]
            out_values = [value for value, is_outside in zip(display_values, outside_mask) if is_outside]
            if out_times:
                ax.scatter(out_times, out_values, color="red", s=10, label="制限外", zorder=3)

            in_limit = "制限内" if not any(outside_mask) else "制限外あり"
            mode_label = "rad/m" if mode == "si" else "deg/mm"
            ax.set_title(f"{joint_name} [{joint_type}] ({mode_label}) - 判定: {in_limit}", fontsize=9)
            ax.set_ylabel(display_unit)
            ax.xaxis.set_major_locator(MultipleLocator(1.0))

            yticks = list(ax.get_yticks())
            yticks.extend([y_min, y_max])
            if lower is not None:
                yticks.append(lower)
            if upper is not None:
                yticks.append(upper)
            yticks = sorted(set(yticks))
            ax.set_yticks(yticks)
            ax.tick_params(axis="y", labelsize=7)
            for tick_label in ax.get_yticklabels():
                tick_label.set_rotation(45)
                tick_label.set_verticalalignment("center")

            ax.grid(True, alpha=0.3)

            if row_index == row_count - 1:
                ax.set_xlabel("時間 (秒)")
                ax.tick_params(axis="x", labelsize=7)
                for tick_label in ax.get_xticklabels():
                    tick_label.set_rotation(45)
                    tick_label.set_horizontalalignment("right")

            handles, labels = ax.get_legend_handles_labels()
            unique = dict(zip(labels, handles))
            ax.legend(unique.values(), unique.keys(), fontsize=7, loc="best")

    plt.tight_layout(rect=(0.0, 0.0, 1.0, 0.985))
    fig.savefig(output_file, format="jpg", dpi=300)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="時刻付きジョイント位置をURDF制限と重ねて、SI単位と変換単位で描画します。"
    )
    parser.add_argument("--path", type=Path, default=Path("path.json"), help="経路JSONファイル")
    parser.add_argument("--urdf", type=Path, default=Path("my_robot.urdf"), help="URDFファイル")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("joint_positions_stacked.jpg"),
        help="出力JPG画像パス",
    )

    args = parser.parse_args()

    joint_names, times, positions = load_path(args.path)
    joint_info = load_urdf_joint_info(args.urdf)
    plot_joint_data(joint_names, times, positions, joint_info, args.output)

    print(f"積み上げジョイントグラフを保存しました: {args.output}")


if __name__ == "__main__":
    main()
