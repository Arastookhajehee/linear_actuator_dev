# 各軸の範囲と最大速度


1. 各軸の可動範囲
2. 経路上で必要となる最大速度

| 軸名 | 範囲 | 最大速度 |
| --- | --- | --- |
| `base.stp_base_to_shoulder` | -90.00 から 90.00 deg | 38.24 deg/s |
| `shoulder.stp_shoulder_to_upper_arm` | -100.00 から 100.00 deg | 56.03 deg/s |
| `upper_arm.stp_upper_arm_to_lower_arm` | -90.00 から 90.00 deg | 28.50 deg/s |
| `lower_arm.stp_lower_arm_to_slider` | -490.00 から 345.00 mm | 294.21 mm/s |
| `slider.stp_slider_to_upper_wrist` | -180.00 から 180.00 deg | 40.26 deg/s |
| `upper_wrist.stp_upper_wrist_to_lower_wrist` | -110.00 から 110.00 deg | 38.24 deg/s |

## 補足

`lower_arm.stp_lower_arm_to_slider` は回転軸ではなく直動軸のため、角度ではなくストローク範囲と最大直動速度で表記しています。

## 送付文面例

各軸の範囲と最大速度を以下に整理しました。  
回転軸は `deg / deg/s`、直動軸は `mm / mm/s` で記載しています。

- `base`: -90.00 から 90.00 deg、最大 38.24 deg/s
- `shoulder`: -100.00 から 100.00 deg、最大 56.03 deg/s
- `upper_arm`: -90.00 から 90.00 deg、最大 28.50 deg/s
- `lower_arm_to_slider`: -490.00 から 345.00 mm、最大 294.21 mm/s
- `slider_to_upper_wrist`: -180.00 から 180.00 deg、最大 40.26 deg/s
- `upper_wrist_to_lower_wrist`: -110.00 から 110.00 deg、最大 38.24 deg/s

