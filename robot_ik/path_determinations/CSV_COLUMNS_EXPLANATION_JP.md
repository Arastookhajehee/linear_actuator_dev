# CSV列の説明

このファイルでは、`joint_speeds.py` が出力するCSV列の意味を説明します。

## 出力ファイル

各実行で、タイムスタンプ付きフォルダの中に4つのCSVファイルが作成されます。

- `joint_speed_summary.csv`
- `joint_speed_summary_mm_deg.csv`
- `joint_speed_summary_simple.csv`
- `joint_speed_summary_simple_mm_deg.csv`

`*_mm_deg.csv` ファイルでは、単位が次のように変換されます。

- 回転ジョイントの位置値は `degree`
- 回転ジョイントの速度値は `degree/s`
- 直動ジョイントの位置値は `mm`
- 直動ジョイントの速度値は `mm/s`

変換なしのファイルでは、次の単位が使われます。

- 回転ジョイントの位置値は `rad`
- 回転ジョイントの速度値は `rad/s`
- 直動ジョイントの位置値は `m`
- 直動ジョイントの速度値は `m/s`

## すべてのファイルにある列

### `joint_name`

ロボットモデルと経路ファイルにあるジョイント名です。

### `joint_type`

URDFに定義されたジョイントタイプです。例: `revolute`、`prismatic`。

## 詳細サマリーCSVにある列

次のファイルに含まれます。

- `joint_speed_summary.csv`
- `joint_speed_summary_mm_deg.csv`

### `joint_position_unit`

そのファイルで使われているジョイント位置の単位です。

例:

- `rad`
- `degree`
- `m`
- `mm`

### `joint_speed_unit`

そのファイルで使われている必要ジョイント速度の単位です。

例:

- `rad/s`
- `degree/s`
- `m/s`
- `mm/s`

### `joint_position_limit_min`

そのジョイントのURDFにある位置下限です。

回転ジョイントでは最小許容角度、直動ジョイントでは最小許容移動量を表します。

### `joint_position_limit_max`

そのジョイントのURDFにある位置上限です。

回転ジョイントでは最大許容角度、直動ジョイントでは最大許容移動量を表します。

### `joint_position_limit_span`

ジョイントの位置下限から位置上限までの許容可動範囲です。

計算式:

```text
joint_position_limit_max - joint_position_limit_min
```

### `joint_velocity_limit`

URDFから読み取った速度上限です。

注意: この値はURDF内でプレースホルダになっている場合があり、実際のハードウェア上限ではないことがあります。

### `joint_effort_limit`

URDFから読み取った努力上限です。

回転ジョイントでは通常トルク関連、直動ジョイントでは通常推力関連の値です。

### `min_position_in_path`

経路の中で実際に使われた最小ジョイント位置です。

### `max_position_in_path`

経路の中で実際に使われた最大ジョイント位置です。

### `position_span_in_path`

経路の中で実際に使われたジョイント位置範囲です。

計算式:

```text
max_position_in_path - min_position_in_path
```

### `min_margin_to_lower_limit`

経路がジョイント下限に最も近づいたときの余裕量です。

値が小さいほど、経路が下限に近いことを意味します。

### `min_margin_to_upper_limit`

経路がジョイント上限に最も近づいたときの余裕量です。

値が小さいほど、経路が上限に近いことを意味します。

### `path_within_position_limits`

経路で使われた全ジョイント位置がURDFの位置制限内に収まっていれば `True` です。

### `path_min_required_joint_speed`

そのジョイントで経路実行に必要な最小の区間速度です。

方向は無視し、絶対値速度で計算しています。

### `path_max_required_joint_speed`

そのジョイントが経路のどこかで必要とする最大速度です。

アクチュエータ選定では、通常これが最も重要な速度値です。

### `path_average_required_joint_speed`

経路全体での必要ジョイント速度の平均値です。

通常時の速度負荷の目安になります。

### `path_median_required_joint_speed`

経路全体での必要ジョイント速度の中央値です。

区間速度の半分はこの値以下、半分はこの値以上です。

### `path_p95_required_joint_speed`

必要ジョイント速度の95パーセンタイル値です。

平易に言うと、経路の約95%ではこの速度以下で動作し、最も速い約5%だけがこの値を超えます。

### `path_rms_required_joint_speed`

経路全体での必要ジョイント速度の実効値です。

高速度区間の影響を平均より強く反映するため、速度スパイクがある経路では通常の平均値より参考になることがあります。

### `segment_count`

解析に使った経路セグメント数です。

速度は連続する2点間で計算するため、軌道点数より1少なくなります。

### `path_duration_sec`

経路全体の時間を秒で表したものです。

## 簡易CSVにある列

次のファイルに含まれます。

- `joint_speed_summary_simple.csv`
- `joint_speed_summary_simple_mm_deg.csv`

### `joint_speed_unit`

そのファイルの速度列に使われている単位です。

### `joint_position_limit_min`

URDFにあるジョイント位置下限です。

### `joint_position_limit_max`

URDFにあるジョイント位置上限です。

### `path_max_required_joint_speed`

経路に必要な最大ジョイント速度です。

「この経路を実行するには、このジョイントはどれくらい速く動ける必要があるか」を見るときの主な列です。

### `path_p95_required_joint_speed`

最大速度1点だけに引っ張られず、経路のほとんどをカバーする速度値です。

絶対最大値ではなく、ほぼ最悪条件に近い速度を見たいときに役立ちます。
