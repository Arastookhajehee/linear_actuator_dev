# Retrofit actuators for the existing concrete spray-gun valve

Research date: 2026-10-09.

## Goal and constraints

Add a device that turns the existing manual valve in response to a Raspberry Pi or Arduino signal, rather than replacing the valve.

- Application: concrete/mortar spray gun, potentially with glass fiber.
- Hose dimension: 25 mm; internal diameter and connection standard still need confirmation.
- Function: **open/close only**; no fine adjustment or positioner required.
- Budget target: **up to ¥40,000 per unit**. Obtain a quote including actuator, bracket/coupling, controls, tax, and shipping to determine whether the complete retrofit fits.
- Environment: abrasive material, splashes, and frequent cleaning.

**Photo limitation:** the supplied image appears to show a manual valve, but its model, operating angle, stem geometry, and required torque cannot be established from the image. The quarter-turn suggestions below apply only if it opens/closes with approximately 90° rotation. These are purchasing/design leads, not confirmed compatible parts.

## Recommended approaches

| Approach | Advantages | Limitations | Recommendation |
|---|---|---|---|
| **Pneumatic cylinder + linkage to existing handle** | Fast; keeps the existing valve and handle; potentially economical if compressed air is already available | Custom bracket/linkage required; must verify force, travel, air supply, and safe failure state | Preferred inquiry for a spray gun with available compressed air |
| **24 V DC quarter-turn electric actuator + stem coupling** | Straightforward electrical on/off control; no compressed air required; direct stem drive can be compact | Must match mounting/stem and torque; some models take several seconds per stroke | Preferred electric approach |
| **Clamp-on motorized valve controller** | Retains handle; potentially easy prototype installation | Household water/gas units may slip, lack torque, be slow, or lack wash protection | Prototype lead only; not the default production recommendation |

No complete, compatible retrofit has been verified below ¥40,000. Price, dimensions, stock, and Japan delivery need confirmation before purchase.

## Japanese purchasing leads

### 1. Electric actuator-only listings

- [24 V DC electric ball-valve actuator / motor with manual switch — Amazon Japan](https://www.amazon.co.jp/dp/B0D6VQQDYY).
- [12 V DC electric ball-valve actuator listing — Amazon Japan](https://www.amazon.co.jp/dp/B0GSVWHL15).
- [Valve actuator sourcing — MISUMI Japan](https://jp.misumi-ec.com/vona2/s_cate/バルブアクチュエータ).

The Amazon search listings describe actuator products, but their torque, mounting pattern, duty cycle, opening time, electrical interface, and wash resistance were not verified. Confirm that the selected variant is **actuator only**, includes open/closed limit switches, and can be mechanically adapted to this valve. Do not assume an actuator made for another valve will fit.

Specify:

- 24 V DC preferred, simple on/off commands.
- Torque appropriate to the valve's worst-case opening/closing requirement, using the valve/actuator supplier's sizing margin.
- Open and closed limit switches, overload or torque protection, manual override, and specified power-loss behavior.
- Required stroke time and cycling frequency for spray operation.
- Environmental protection appropriate to the actual cleaning method; an IP rating alone does not establish hot-water, chemical, or pressure-washer compatibility.

### 2. Clamp-on handle-turning controller

- [12 V clamp-on valve controller — Amazon Japan](https://www.amazon.co.jp/dp/B0D1GG4WHG).
- [12 V pipeline ball-valve controller — Amazon Japan](https://www.amazon.co.jp/dp/B0CPYC5QFX).

These are low-cost-category leads intended for investigation, not verified industrial recommendations. Listings mention household water/gas valve operation; that does not prove suitability for a dirty, vibrating spray gun. Prices were not verified during this research.

Check handle geometry, clamp size, alignment, rated torque, operating speed, manual release, signal wiring, and wash protection. Avoid relying on a cloud/Wi-Fi-only interface where dependable local machine control is required.

### 3. Pneumatic cylinder and handle linkage

- [Pneumatic-cylinder sourcing — MISUMI Japan](https://jp.misumi-ec.com/search/?Keyword=エアシリンダ).
- [Pneumatic-cylinder sourcing — MonotaRO](https://www.monotaro.com/s/q-エアシリンダ/).

These are component-search links, not a complete retrofit kit or a selected cylinder. A cylinder can push/pull the valve handle through a pivoted linkage. Ask a local machine shop or automation supplier to select the cylinder and build a rigid, guarded mounting bracket.

For a double-acting cylinder, the typical control is a supplier-selected **5/2 pneumatic solenoid valve**. A single-acting spring-return mechanism generally uses a **3/2 valve**, but its force and fail position must be validated. Do not choose bore/stroke from the hose diameter alone.

## Mounting requirements

1. Obtain a full photo, valve model, handle length, turning angle, and dimensions around the stem.
2. Check whether the valve has an **ISO 5211 actuator mounting pad**. If not, a custom bracket and coupling will probably be necessary.
3. Support reaction torque through a rigid bracket attached to an appropriate structural part of the assembly. Avoid transferring actuator loads into hose couplings or plastic fittings.
4. Align the actuator shaft with the valve stem, or design a linkage that avoids excessive side loading and unfavorable leverage near the endpoints.
5. Use adjustable endpoints/limit switches; do not continuously stall the motor against the valve stops.
6. Retain safe manual operation and removal for cleaning. Guard linkage pinch points.

**Torque measurement:** a technician can estimate operating torque from tangential handle force and lever length: `torque (N·m) = force (N) × radius (m)`. For example, 30 N applied perpendicular to a 0.10 m lever gives 3 N·m. This is only an illustration, not a torque estimate for the pictured valve. Do not perform unsafe tests on a live pressurized spray gun; use supplier data and controlled testing. Account for breakaway torque, material pressure, contamination, and cycling duty.

## Raspberry Pi / Arduino control

### Electric actuator

```text
Raspberry Pi GPIO
  -> 3.3 V-compatible isolated control interface
  -> separately powered 12/24 V actuator
  -> open / close valve
```

- Actual wiring depends on the ordered actuator: separate open/close inputs, polarity reversal, or another scheme. Follow its wiring diagram.
- Use appropriately rated relays or a motor driver; provide fusing, transient protection, and interlocking so incompatible commands cannot be applied simultaneously.
- Never power an actuator from GPIO or apply industrial voltage to GPIO. Isolate/translate endpoint feedback.
- Arduino can perform the same function with a driver matched to its logic voltage; buying one is not necessary just to switch a valve from the existing Pi.

### Pneumatic mechanism

```text
Pi -> isolated driver -> separately powered air-solenoid coil
Compressed air -> filter/regulator -> air solenoid -> cylinder -> handle linkage
```

Size the air circuit for the required speed and load. Ensure spray-air consumption does not deprive the actuator of operating pressure.

## Concrete-service and safety checks

**Automating the existing valve does not make it suitable for concrete.** Confirm its bore, material-pressure rating, abrasion tolerance, and cleanability before investing in an actuator.

- Identify maximum material pressure, including pump stall/dead-head pressure and transients. Atomizing-air pressure is not material pressure.
- Interlock valve closure with the pump using the pump manufacturer's approved stop/bypass/pressure-protection arrangement. Do not shut a valve against an unprotected running positive-displacement pump.
- Specify power-loss and air-loss behavior; ordinary electric actuators may stay in their last position, and double-acting pneumatic mechanisms are not inherently fail-closed.
- Provide hardware emergency-stop and pump protection independently of Raspberry Pi software.
- Do not use a larger actuator to force a cement-jammed valve. Stop, isolate, depressurize, and service it.
- Flush before cement sets. Isolate and relieve trapped pressure before dismantling any valve or coupling.
- Protect motors, solenoids, wiring, and controllers from cleaning spray. Make brackets accessible enough that hardened splashes do not obstruct movement.

## Information needed to select an actual actuator

- Valve manufacturer/model and confirmation of 90° operation.
- Full handle/stem photo, mounting-pad details, available space, and dimensions.
- Required operating torque, or supplier data enabling torque selection.
- Maximum material pressure and whether the existing valve is approved for the mix.
- Desired opening/closing time and cycles per minute.
- Available compressed-air pressure and electrical supply.
- Required safe failure position and cleaning method.

## Japanese quote request

> コンクリート／モルタル吹付ガンの既存手動バルブを自動開閉したいです。ホース寸法は25mmです。バルブ型式は___、回転角度は___度、操作トルクは___N·m、最大材料圧力は___MPaです。Raspberry Piから絶縁インターフェースを介してON/OFF制御する予定で、比例制御は不要です。電動の場合はDC24Vアクチュエータ、空圧の場合はシリンダーとハンドルリンク機構を検討しています。取付ブラケット・カップリング・制御部品を含めた1台分の予算目標は税込40,000円以内です。必要開閉時間は___秒、動作頻度は___回／分です。適合性、必要トルク／空気圧、リミットスイッチ、手動操作、過負荷保護、停電・エア喪失時の動作、洗浄への耐性、価格、送料、納期をご提案ください。

**Bottom line:** pneumatic handle actuation is the first approach to quote if compressed air is available; otherwise investigate a 24 V quarter-turn actuator with a proper mounting bracket. Do not purchase an actuator solely from the photo or the hose size.
