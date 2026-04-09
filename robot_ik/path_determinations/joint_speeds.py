import argparse
import csv
from datetime import datetime
import json
import math
import shutil
import statistics
import xml.etree.ElementTree as ET
from pathlib import Path


ENGLISH_TO_JAPANESE_HEADERS = {
    "joint_name": "ジョイント名",
    "joint_type": "ジョイントタイプ",
    "joint_position_unit": "ジョイント位置単位",
    "joint_speed_unit": "ジョイント速度単位",
    "joint_position_limit_min": "ジョイント位置下限",
    "joint_position_limit_max": "ジョイント位置上限",
    "joint_position_limit_span": "ジョイント可動範囲",
    "joint_velocity_limit": "ジョイント速度上限",
    "joint_effort_limit": "ジョイント努力上限",
    "min_position_in_path": "経路内最小ジョイント位置",
    "max_position_in_path": "経路内最大ジョイント位置",
    "position_span_in_path": "経路内ジョイント位置範囲",
    "min_margin_to_lower_limit": "下限までの最小余裕",
    "min_margin_to_upper_limit": "上限までの最小余裕",
    "path_within_position_limits": "経路はジョイント位置制限内",
    "path_min_required_joint_speed": "経路で必要な最小ジョイント速度",
    "path_max_required_joint_speed": "経路で必要な最大ジョイント速度",
    "path_average_required_joint_speed": "経路で必要な平均ジョイント速度",
    "path_median_required_joint_speed": "経路で必要な中央値ジョイント速度",
    "path_p95_required_joint_speed": "経路で必要な95パーセンタイルジョイント速度",
    "path_rms_required_joint_speed": "経路で必要な実効ジョイント速度",
    "segment_count": "セグメント数",
    "path_duration_sec": "経路時間_秒",
}

ENGLISH_TO_JAPANESE_JOINT_TYPES = {
    "revolute": "回転",
    "prismatic": "直動",
    "continuous": "連続回転",
    "fixed": "固定",
    "unknown": "不明",
}


def load_path(path_file: Path) -> tuple[list[str], list[dict]]:
    with path_file.open("r", encoding="utf-8-sig") as file:
        data = json.load(file)

    if not isinstance(data, dict) or "axes_path" not in data:
        raise ValueError(f"Missing 'axes_path' in {path_file}")

    axes_path = data["axes_path"]
    if not isinstance(axes_path, dict):
        raise ValueError(f"'axes_path' must be an object in {path_file}")

    joint_names = axes_path.get("joint_names")
    points = axes_path.get("points")

    if not isinstance(joint_names, list) or not joint_names:
        raise ValueError(f"'axes_path.joint_names' must be a non-empty list in {path_file}")
    if not isinstance(points, list) or len(points) < 2:
        raise ValueError(f"'axes_path.points' must contain at least two points in {path_file}")

    for index, joint_name in enumerate(joint_names):
        if not isinstance(joint_name, str) or not joint_name:
            raise ValueError(f"Invalid joint name at index {index} in {path_file}")

    expected_joint_count = len(joint_names)
    previous_time = None
    for index, point in enumerate(points):
        if not isinstance(point, dict):
            raise ValueError(f"Point {index} is not an object in {path_file}")

        positions = point.get("positions")
        time_from_start = point.get("time_from_start_sec")

        if not isinstance(positions, list) or len(positions) != expected_joint_count:
            raise ValueError(
                f"Point {index} positions length {len(positions) if isinstance(positions, list) else 'invalid'} "
                f"does not match joint count {expected_joint_count}"
            )
        if not isinstance(time_from_start, (int, float)):
            raise ValueError(f"Point {index} has invalid 'time_from_start_sec'")

        for position_index, position in enumerate(positions):
            if not isinstance(position, (int, float)):
                raise ValueError(f"Point {index} position {position_index} is not numeric")

        if previous_time is not None and time_from_start <= previous_time:
            raise ValueError(
                f"Non-increasing time at points {index - 1} and {index}: "
                f"{previous_time} -> {time_from_start}"
            )
        previous_time = float(time_from_start)

    return joint_names, points


def parse_float(value: str | None) -> float | None:
    if value is None:
        return None
    return float(value)


def infer_units(joint_type: str) -> tuple[str, str]:
    if joint_type in {"revolute", "continuous"}:
        return "rad", "rad/s"
    if joint_type == "prismatic":
        return "m", "m/s"
    return "unknown", "unknown"


def load_urdf_joint_info(urdf_file: Path) -> dict[str, dict]:
    tree = ET.parse(urdf_file)
    root = tree.getroot()
    if root.tag != "robot":
        raise ValueError(f"Unexpected URDF root tag '{root.tag}' in {urdf_file}")

    joint_info = {}
    for joint_element in root.findall("joint"):
        joint_name = joint_element.get("name")
        joint_type = joint_element.get("type", "unknown")
        if not joint_name:
            continue

        limit_element = joint_element.find("limit")
        lower_limit = parse_float(limit_element.get("lower")) if limit_element is not None else None
        upper_limit = parse_float(limit_element.get("upper")) if limit_element is not None else None
        velocity_limit = parse_float(limit_element.get("velocity")) if limit_element is not None else None
        effort_limit = parse_float(limit_element.get("effort")) if limit_element is not None else None
        position_unit, speed_unit = infer_units(joint_type)

        joint_info[joint_name] = {
            "joint_type": joint_type,
            "position_unit": position_unit,
            "speed_unit": speed_unit,
            "lower_limit": lower_limit,
            "upper_limit": upper_limit,
            "velocity_limit": velocity_limit,
            "effort_limit": effort_limit,
        }

    return joint_info


def calculate_segment_speed_magnitudes(joint_names: list[str], points: list[dict]) -> dict[str, list[float]]:
    speeds_by_joint = {joint_name: [] for joint_name in joint_names}

    for index in range(1, len(points)):
        previous_point = points[index - 1]
        current_point = points[index]

        dt = float(current_point["time_from_start_sec"]) - float(previous_point["time_from_start_sec"])
        if dt <= 0:
            raise ValueError(f"Non-increasing time at points {index - 1} and {index}: dt={dt}")

        for joint_name, previous_position, current_position in zip(
            joint_names,
            previous_point["positions"],
            current_point["positions"],
        ):
            speed = abs((float(current_position) - float(previous_position)) / dt)
            speeds_by_joint[joint_name].append(speed)

    return speeds_by_joint


def percentile(values: list[float], fraction: float) -> float:
    if not values:
        raise ValueError("Cannot compute percentile of empty data")

    sorted_values = sorted(values)
    if len(sorted_values) == 1:
        return sorted_values[0]

    position = (len(sorted_values) - 1) * fraction
    lower_index = math.floor(position)
    upper_index = math.ceil(position)
    if lower_index == upper_index:
        return sorted_values[lower_index]

    lower_value = sorted_values[lower_index]
    upper_value = sorted_values[upper_index]
    weight = position - lower_index
    return lower_value + (upper_value - lower_value) * weight


def rms(values: list[float]) -> float:
    if not values:
        raise ValueError("Cannot compute RMS of empty data")
    return math.sqrt(sum(value * value for value in values) / len(values))


def build_summary_rows(
    joint_names: list[str],
    points: list[dict],
    urdf_joint_info: dict[str, dict],
    speeds_by_joint: dict[str, list[float]],
) -> list[dict]:
    positions_by_joint = {joint_name: [] for joint_name in joint_names}
    for point in points:
        for joint_name, position in zip(joint_names, point["positions"]):
            positions_by_joint[joint_name].append(float(position))

    segment_count = len(points) - 1
    path_duration_sec = float(points[-1]["time_from_start_sec"]) - float(points[0]["time_from_start_sec"])
    rows = []

    for joint_name in joint_names:
        joint_metadata = urdf_joint_info.get(
            joint_name,
            {
                "joint_type": "unknown",
                "position_unit": "unknown",
                "speed_unit": "unknown",
                "lower_limit": None,
                "upper_limit": None,
                "velocity_limit": None,
                "effort_limit": None,
            },
        )

        positions = positions_by_joint[joint_name]
        speeds = speeds_by_joint[joint_name]

        lower_limit = joint_metadata["lower_limit"]
        upper_limit = joint_metadata["upper_limit"]
        velocity_limit = joint_metadata["velocity_limit"]
        effort_limit = joint_metadata["effort_limit"]

        min_position = min(positions)
        max_position = max(positions)
        position_span = max_position - min_position
        limit_span = None
        if lower_limit is not None and upper_limit is not None:
            limit_span = upper_limit - lower_limit

        min_margin_to_lower_limit = None
        if lower_limit is not None:
            min_margin_to_lower_limit = min(position - lower_limit for position in positions)

        min_margin_to_upper_limit = None
        if upper_limit is not None:
            min_margin_to_upper_limit = min(upper_limit - position for position in positions)

        path_within_position_limits = None
        if lower_limit is not None and upper_limit is not None:
            path_within_position_limits = all(lower_limit <= position <= upper_limit for position in positions)

        average_speed = statistics.fmean(speeds)
        max_speed = max(speeds)

        rows.append(
            {
                "joint_name": joint_name,
                "joint_type": joint_metadata["joint_type"],
                "joint_position_unit": joint_metadata["position_unit"],
                "joint_speed_unit": joint_metadata["speed_unit"],
                "joint_position_limit_min": lower_limit,
                "joint_position_limit_max": upper_limit,
                "joint_position_limit_span": limit_span,
                "joint_velocity_limit": velocity_limit,
                "joint_effort_limit": effort_limit,
                "min_position_in_path": min_position,
                "max_position_in_path": max_position,
                "position_span_in_path": position_span,
                "min_margin_to_lower_limit": min_margin_to_lower_limit,
                "min_margin_to_upper_limit": min_margin_to_upper_limit,
                "path_within_position_limits": path_within_position_limits,
                "path_min_required_joint_speed": min(speeds),
                "path_max_required_joint_speed": max_speed,
                "path_average_required_joint_speed": average_speed,
                "path_median_required_joint_speed": statistics.median(speeds),
                "path_p95_required_joint_speed": percentile(speeds, 0.95),
                "path_rms_required_joint_speed": rms(speeds),
                "segment_count": segment_count,
                "path_duration_sec": path_duration_sec,
            }
        )

    return rows


def format_csv_value(value: object, rounding_places: int) -> object:
    if isinstance(value, float):
        return f"{value:.{rounding_places}f}"
    return value


def translate_joint_type(joint_type: str, language: str) -> str:
    if language == "jp":
        return ENGLISH_TO_JAPANESE_JOINT_TYPES.get(joint_type, joint_type)
    return joint_type


def translate_fieldnames(fieldnames: list[str], language: str) -> list[str]:
    if language == "jp":
        return [ENGLISH_TO_JAPANESE_HEADERS.get(fieldname, fieldname) for fieldname in fieldnames]
    return fieldnames


def build_csv_row(row: dict, fieldnames: list[str], rounding_places: int, language: str) -> dict:
    csv_row = {}
    for fieldname in fieldnames:
        value = row[fieldname]
        if fieldname == "joint_type":
            value = translate_joint_type(value, language)
        csv_row[fieldname] = format_csv_value(value, rounding_places)
    return csv_row


def convert_position_and_speed_units(
    joint_type: str,
    lower_limit: float | None,
    upper_limit: float | None,
    max_speed: float,
    p95_speed: float,
) -> dict:
    if joint_type in {"revolute", "continuous"}:
        factor = 180.0 / math.pi
        return {
            "unit": "degree/s",
            "joint_limit_min": None if lower_limit is None else lower_limit * factor,
            "joint_limit_max": None if upper_limit is None else upper_limit * factor,
            "path_max_speed": max_speed * factor,
            "p95_speed": p95_speed * factor,
        }
    if joint_type == "prismatic":
        factor = 1000.0
        return {
            "unit": "mm/s",
            "joint_limit_min": None if lower_limit is None else lower_limit * factor,
            "joint_limit_max": None if upper_limit is None else upper_limit * factor,
            "path_max_speed": max_speed * factor,
            "p95_speed": p95_speed * factor,
        }
    return {
        "unit": "unknown",
        "joint_limit_min": lower_limit,
        "joint_limit_max": upper_limit,
        "path_max_speed": max_speed,
        "p95_speed": p95_speed,
    }


def convert_summary_row_units(row: dict) -> dict:
    converted = dict(row)
    joint_type = row["joint_type"]

    if joint_type in {"revolute", "continuous"}:
        position_factor = 180.0 / math.pi
        speed_factor = 180.0 / math.pi
        converted["joint_position_unit"] = "degree"
        converted["joint_speed_unit"] = "degree/s"
    elif joint_type == "prismatic":
        position_factor = 1000.0
        speed_factor = 1000.0
        converted["joint_position_unit"] = "mm"
        converted["joint_speed_unit"] = "mm/s"
    else:
        position_factor = 1.0
        speed_factor = 1.0

    position_fields = [
        "joint_position_limit_min",
        "joint_position_limit_max",
        "joint_position_limit_span",
        "min_position_in_path",
        "max_position_in_path",
        "position_span_in_path",
        "min_margin_to_lower_limit",
        "min_margin_to_upper_limit",
    ]
    speed_fields = [
        "joint_velocity_limit",
        "path_min_required_joint_speed",
        "path_max_required_joint_speed",
        "path_average_required_joint_speed",
        "path_median_required_joint_speed",
        "path_p95_required_joint_speed",
        "path_rms_required_joint_speed",
    ]

    for field in position_fields:
        if converted[field] is not None:
            converted[field] *= position_factor

    for field in speed_fields:
        if converted[field] is not None:
            converted[field] *= speed_factor

    return converted


def write_summary_csv(output_file: Path, rows: list[dict], rounding_places: int, language: str) -> None:
    fieldnames = [
        "joint_name",
        "joint_type",
        "joint_position_unit",
        "joint_speed_unit",
        "joint_position_limit_min",
        "joint_position_limit_max",
        "joint_position_limit_span",
        "joint_velocity_limit",
        "joint_effort_limit",
        "min_position_in_path",
        "max_position_in_path",
        "position_span_in_path",
        "min_margin_to_lower_limit",
        "min_margin_to_upper_limit",
        "path_within_position_limits",
        "path_min_required_joint_speed",
        "path_max_required_joint_speed",
        "path_average_required_joint_speed",
        "path_median_required_joint_speed",
        "path_p95_required_joint_speed",
        "path_rms_required_joint_speed",
        "segment_count",
        "path_duration_sec",
    ]
    output_fieldnames = translate_fieldnames(fieldnames, language)

    with output_file.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=output_fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(
                {
                    translated_key: value
                    for translated_key, value in zip(
                        output_fieldnames,
                        build_csv_row(row, fieldnames, rounding_places, language).values(),
                    )
                }
            )


def write_converted_summary_csv(output_file: Path, rows: list[dict], rounding_places: int, language: str) -> None:
    converted_rows = [convert_summary_row_units(row) for row in rows]
    write_summary_csv(output_file, converted_rows, rounding_places, language)


def write_simple_summary_csv(output_file: Path, rows: list[dict], rounding_places: int, language: str) -> None:
    fieldnames = [
        "joint_name",
        "joint_type",
        "joint_speed_unit",
        "joint_position_limit_min",
        "joint_position_limit_max",
        "path_max_required_joint_speed",
        "path_p95_required_joint_speed",
    ]
    output_fieldnames = translate_fieldnames(fieldnames, language)

    with output_file.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=output_fieldnames)
        writer.writeheader()
        for row in rows:
            simple_row = {
                "joint_name": row["joint_name"],
                "joint_type": row["joint_type"],
                "joint_speed_unit": row["joint_speed_unit"],
                "joint_position_limit_min": row["joint_position_limit_min"],
                "joint_position_limit_max": row["joint_position_limit_max"],
                "path_max_required_joint_speed": row["path_max_required_joint_speed"],
                "path_p95_required_joint_speed": row["path_p95_required_joint_speed"],
            }
            writer.writerow(
                {
                    translated_key: value
                    for translated_key, value in zip(
                        output_fieldnames,
                        build_csv_row(simple_row, fieldnames, rounding_places, language).values(),
                    )
                }
            )


def write_converted_simple_summary_csv(output_file: Path, rows: list[dict], rounding_places: int, language: str) -> None:
    fieldnames = [
        "joint_name",
        "joint_type",
        "joint_speed_unit",
        "joint_position_limit_min",
        "joint_position_limit_max",
        "path_max_required_joint_speed",
        "path_p95_required_joint_speed",
    ]
    output_fieldnames = translate_fieldnames(fieldnames, language)

    with output_file.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=output_fieldnames)
        writer.writeheader()
        for row in rows:
            converted = convert_position_and_speed_units(
                row["joint_type"],
                row["joint_position_limit_min"],
                row["joint_position_limit_max"],
                row["path_max_required_joint_speed"],
                row["path_p95_required_joint_speed"],
            )
            converted_row = {
                "joint_name": row["joint_name"],
                "joint_type": row["joint_type"],
                "joint_speed_unit": converted["unit"],
                "joint_position_limit_min": converted["joint_limit_min"],
                "joint_position_limit_max": converted["joint_limit_max"],
                "path_max_required_joint_speed": converted["path_max_speed"],
                "path_p95_required_joint_speed": converted["p95_speed"],
            }
            writer.writerow(
                {
                    translated_key: value
                    for translated_key, value in zip(
                        output_fieldnames,
                        build_csv_row(converted_row, fieldnames, rounding_places, language).values(),
                    )
                }
            )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Analyze joint speeds from a path JSON and URDF.")
    parser.add_argument("--path", type=Path, default=Path("path.json"), help="Input path JSON file")
    parser.add_argument("--urdf", type=Path, default=Path("my_robot.urdf"), help="Input robot URDF file")
    parser.add_argument(
        "--csv",
        type=Path,
        default=Path("joint_speed_summary.csv"),
        help="Output full summary CSV file",
    )
    parser.add_argument(
        "--round",
        type=int,
        default=9,
        help="Number of decimal places to write in CSV outputs",
    )
    args = parser.parse_args()
    if args.round < 0:
        parser.error("--round must be 0 or greater")
    return args


def derive_simple_csv_path(full_csv_path: Path) -> Path:
    return full_csv_path.with_name(f"{full_csv_path.stem}_simple{full_csv_path.suffix}")


def derive_converted_simple_csv_path(full_csv_path: Path) -> Path:
    return full_csv_path.with_name(f"{full_csv_path.stem}_simple_mm_deg{full_csv_path.suffix}")


def derive_converted_summary_csv_path(full_csv_path: Path) -> Path:
    return full_csv_path.with_name(f"{full_csv_path.stem}_mm_deg{full_csv_path.suffix}")


def build_output_directory(base_directory: Path) -> Path:
    timestamp = datetime.now().strftime("%Y%m%d%H%M")
    output_directory = base_directory / f"output_{timestamp}"
    output_directory.mkdir(parents=True, exist_ok=True)
    return output_directory


def resolve_output_file(csv_argument: Path) -> Path:
    output_directory = build_output_directory(csv_argument.parent)
    return output_directory / csv_argument.name


def build_language_directory(output_directory: Path, language: str) -> Path:
    language_directory = output_directory / language
    language_directory.mkdir(parents=True, exist_ok=True)
    return language_directory


def copy_explanation_file(target_directory: Path, source_file: Path) -> None:
    shutil.copy2(source_file, target_directory / "CSV_COLUMNS_EXPLANATION.md")


def main() -> int:
    args = parse_args()
    path_file = args.path
    urdf_file = args.urdf
    output_file = resolve_output_file(args.csv)
    en_directory = build_language_directory(output_file.parent, "en")
    jp_directory = build_language_directory(output_file.parent, "jp")
    en_output_file = en_directory / output_file.name
    jp_output_file = jp_directory / output_file.name
    en_converted_output_file = derive_converted_summary_csv_path(en_output_file)
    jp_converted_output_file = derive_converted_summary_csv_path(jp_output_file)
    en_simple_output_file = derive_simple_csv_path(en_output_file)
    jp_simple_output_file = derive_simple_csv_path(jp_output_file)
    en_converted_simple_output_file = derive_converted_simple_csv_path(en_output_file)
    jp_converted_simple_output_file = derive_converted_simple_csv_path(jp_output_file)
    rounding_places = args.round
    explanation_file_en = Path(__file__).with_name("CSV_COLUMNS_EXPLANATION.md")
    explanation_file_jp = Path(__file__).with_name("CSV_COLUMNS_EXPLANATION_JP.md")

    joint_names, points = load_path(path_file)
    urdf_joint_info = load_urdf_joint_info(urdf_file)
    speeds_by_joint = calculate_segment_speed_magnitudes(joint_names, points)
    summary_rows = build_summary_rows(joint_names, points, urdf_joint_info, speeds_by_joint)
    write_summary_csv(en_output_file, summary_rows, rounding_places, "en")
    write_converted_summary_csv(en_converted_output_file, summary_rows, rounding_places, "en")
    write_simple_summary_csv(en_simple_output_file, summary_rows, rounding_places, "en")
    write_converted_simple_summary_csv(en_converted_simple_output_file, summary_rows, rounding_places, "en")
    write_summary_csv(jp_output_file, summary_rows, rounding_places, "jp")
    write_converted_summary_csv(jp_converted_output_file, summary_rows, rounding_places, "jp")
    write_simple_summary_csv(jp_simple_output_file, summary_rows, rounding_places, "jp")
    write_converted_simple_summary_csv(jp_converted_simple_output_file, summary_rows, rounding_places, "jp")
    copy_explanation_file(en_directory, explanation_file_en)
    copy_explanation_file(jp_directory, explanation_file_jp)

    print(f"Wrote outputs to {output_file.parent}")
    print(f"Wrote English outputs to {en_directory}")
    print(f"Wrote Japanese outputs to {jp_directory}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
