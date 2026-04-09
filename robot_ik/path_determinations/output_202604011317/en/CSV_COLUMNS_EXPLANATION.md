# CSV Column Explanations

This file explains the columns written by `joint_speeds.py`.

## Output Files

Each run creates four CSV files inside a timestamped folder:

- `joint_speed_summary.csv`
- `joint_speed_summary_mm_deg.csv`
- `joint_speed_summary_simple.csv`
- `joint_speed_summary_simple_mm_deg.csv`

The `*_mm_deg.csv` files convert:

- revolute joint position values to `degree`
- revolute joint speed values to `degree/s`
- prismatic joint position values to `mm`
- prismatic joint speed values to `mm/s`

The non-converted files use:

- revolute joint position values in `rad`
- revolute joint speed values in `rad/s`
- prismatic joint position values in `m`
- prismatic joint speed values in `m/s`

## Columns In All Files

### `joint_name`

The joint name from the robot model and path file.

### `joint_type`

The URDF joint type, such as `revolute` or `prismatic`.

## Columns In The Full Summary Files

These columns appear in:

- `joint_speed_summary.csv`
- `joint_speed_summary_mm_deg.csv`

### `joint_position_unit`

The unit used for joint position values in that file.

Examples:

- `rad`
- `degree`
- `m`
- `mm`

### `joint_speed_unit`

The unit used for required joint speed values in that file.

Examples:

- `rad/s`
- `degree/s`
- `m/s`
- `mm/s`

### `joint_position_limit_min`

The lower position limit from the URDF for that joint.

For a revolute joint, this is the minimum allowed angle.
For a prismatic joint, this is the minimum allowed linear travel.

### `joint_position_limit_max`

The upper position limit from the URDF for that joint.

For a revolute joint, this is the maximum allowed angle.
For a prismatic joint, this is the maximum allowed linear travel.

### `joint_position_limit_span`

The total allowed joint travel between the minimum and maximum position limits.

Formula:

```text
joint_position_limit_max - joint_position_limit_min
```

### `joint_velocity_limit`

The velocity limit read from the URDF.

Note: this value may be a placeholder in your URDF and not a real hardware limit.

### `joint_effort_limit`

The effort limit read from the URDF.

For revolute joints this is typically torque-related. For prismatic joints this is typically force-related.

### `min_position_in_path`

The smallest joint position actually used anywhere in the path.

### `max_position_in_path`

The largest joint position actually used anywhere in the path.

### `position_span_in_path`

The total joint position range actually used by the path.

Formula:

```text
max_position_in_path - min_position_in_path
```

### `min_margin_to_lower_limit`

The closest the path gets to the lower joint limit.

Smaller numbers mean the path gets closer to the lower limit.

### `min_margin_to_upper_limit`

The closest the path gets to the upper joint limit.

Smaller numbers mean the path gets closer to the upper limit.

### `path_within_position_limits`

`True` if every position used in the path stays within the URDF joint limits.

### `path_min_required_joint_speed`

The smallest nonzero segment speed required by the path for that joint.

This is based on absolute speed, not direction.

### `path_max_required_joint_speed`

The fastest that joint ever needs to move anywhere in the path.

This is usually the most important speed value for actuator selection.

### `path_average_required_joint_speed`

The average required joint speed over the full path.

This gives a general idea of normal speed demand.

### `path_median_required_joint_speed`

The middle required joint speed value over the full path.

Half the sampled segment speeds are below this value and half are above it.

### `path_p95_required_joint_speed`

The 95th percentile required joint speed.

In plain English: the joint stays at or below this speed for about 95% of the path, and only exceeds it during the fastest 5% of the path.

### `path_rms_required_joint_speed`

The root-mean-square speed over the path.

This gives extra weight to high-speed portions and is often more informative than a plain average when the path includes speed spikes.

### `segment_count`

The number of path segments used in the analysis.

This is one less than the number of trajectory points, because each speed is calculated between two consecutive points.

### `path_duration_sec`

The total duration of the path in seconds.

## Columns In The Simple Files

These columns appear in:

- `joint_speed_summary_simple.csv`
- `joint_speed_summary_simple_mm_deg.csv`

### `joint_speed_unit`

The unit used for the speed columns in that file.

### `joint_position_limit_min`

The lower URDF joint position limit.

### `joint_position_limit_max`

The upper URDF joint position limit.

### `path_max_required_joint_speed`

The peak required joint speed for the path.

If you are asking, "How fast must this joint be able to move for this path to work?", this is the main column to look at.

### `path_p95_required_joint_speed`

The speed that covers almost the whole path without using the single highest spike.

This is useful when you want a near-worst-case speed instead of the absolute maximum.
