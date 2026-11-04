# Rugged, cleanable valves available in Japan

Research date: 2026-10-08. Intended control: existing Raspberry Pi, or Arduino.

**Confirmed requirement: open/close only. No proportional control, fine adjustment, or positioner is needed.** Request an on/off actuator configuration; for pneumatic models, a simple electrically switched air solenoid is sufficient for commanding open/close.

## Recommendation

**For wet concrete/mortar containing chopped glass fiber, start with an industrial full-bore pinch valve, not an ordinary water solenoid valve or hobby motorized ball valve.** A pinch valve squeezes a replaceable rubber sleeve; the abrasive mixture does not pass through a metal closing mechanism or small pilot passages.

My first inquiries would be **SB Valve** (explicit fresh-concrete application), **AKO through Japan Machinery** (sleeve/material options), and **Flowrox through RIX** (heavy-duty service and electric-actuator option). **OKM 704** is a useful domestically purchasable option with published online prices.

Assumption: the valve carries the wet mixture. If it controls only clean mixing water, cheaper water-rated valves can be suitable. Dry cement or chopped-fiber feed needs a separate bulk-solids assessment: fibers can bridge, and a liquid valve is not automatically an effective dry-fiber feeder.

**Important uncertainty:** these suppliers document concrete, cement, or abrasive-slurry applications, but I did not find explicit qualification for your particular concrete + glass-fiber recipe. Treat these as candidates for supplier selection and testing, not confirmed drop-in parts. Pipe diameter, aggregate size, fiber length/concentration, and pump pressure are still unknown.

## Shortlist and Japanese purchasing links

| Option                                               | Why consider it                                                                                               | Actuation / Pi or Arduino interface                                                | Japan purchase route                                                                                                                                                                                                                             | Published price |
| ---------------------------------------------------- | ------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------- |
| **SB Valve S-150 / S-151 / S-156**                   | Japanese manufacturer explicitly lists slurry and fresh concrete (生コン); rubber-sleeve construction         | Direct air pressure; controller switches an external air solenoid through a driver | [Product](https://sbvalve.jp/product/スラリー・汚泥・ヘドロ・汚水・生コン向けピン) · [Request quote](https://sbvalve.jp/contact)                                                                                                                 | Quote required  |
| **AKO pneumatic pinch valve, via Japan Machinery**   | Full bore, easy maintenance, abrasion-resistant sleeve choices; cement and glass applications listed          | Air-operated; external solenoid + driver                                           | [Japanese distributor product](https://www.jmc.asia/products/ako) · [Request quote](https://www.jmc.asia/contact/)                                                                                                                               | Quote required  |
| **Valmet Flowrox automated pinch valve, via RIX**    | Designed for highly abrasive slurries; modular replaceable sleeve; pneumatic and electric actuation available | Pneumatic solenoid, or electric actuator with supplier-specified isolated controls | [Japanese product](https://www.rix.co.jp/products_services/products/category/flowrox_valve/post_76/) · [Product inquiry](https://www.rix.co.jp/products_services/contact/?product=329)                                                           | Quote required  |
| **OKM 704 air-operated pinch valve**                 | Straight-through slurry valve; manufacturer explicitly lists cement; replaceable sleeve                       | Direct air pressure; external solenoid + driver                                    | [Manufacturer](https://sol.okm-net.jp/product/pinch/pinch-valve/item_704-703) · [Buy from Haikanbuhin.com](https://www.haikanbuhin.com/top/detail/asp/detail.asp?gcode=50256) · [Ishiguro listing](https://ec.ishiguro-gr.co.jp/shop/g/g1134569) | Examples below  |
| **GEMÜ Q50 eSyStep — conditional small-line option** | Motorized external tube-pinch design; quick tube replacement; 24 V DC and IP65 listed                         | Electric actuator; isolated interface appropriate to ordered input configuration   | [GEMÜ Japan product / inquiry](https://pr.mono.ipros.com/en/gemu-group/product/detail/2000619205)                                                                                                                                                | Quote required  |

Availability here means a Japanese supplier or storefront was identified. Current stock, delivery time, exact configuration, and acceptance of the mixture must be confirmed. No purchases or quote requests have been submitted.

### 1. SB Valve: strongest explicit concrete-use lead

- The product page specifically names **fresh concrete**, sludge, and slurry.
- It describes a rubber sleeve inside a cast-iron casing, operated remotely by applying air approximately **3 kgf/cm² (~0.294 MPa) above the fluid pressure**. This is an actuation differential, not a published maximum process-pressure rating.
- Also ask about their **S-152 / S-153 air-cylinder versions**, linked from the same product page, if direct air squeezing is unsuitable.
- A [SUS304 stainless-body pinch-valve option](https://sbvalve.jp/product/ステンレス製ピンチバルブ/) exists. A stainless exterior may help corrosion resistance and cleanup, but does not itself prove washdown suitability or sleeve compatibility.
- [Full catalog PDF](https://sbvalve.jp/wp-content/themes/sb-valve/assets/images/pdf/catalog.pdf).
- Ask for the correct bore, pressure rating, abrasion-resistant sleeve, spare sleeves, disassembly procedure, and washdown restrictions.

### 2. AKO through Japan Machinery: good configurable option

- The Japanese distributor lists full-bore opening, low pressure loss, and easy maintenance.
- Listed body choices: stainless steel, aluminum, POM, and PVC. Sleeve choices include natural rubber, EPDM, Viton, silicone, and neoprene, with abrasion-resistant options.
- Distributor states **0.2 MPa actuation differential** and maximum working pressures of **0.2–0.6 MPa depending on configuration**. Do not treat 0.6 MPa as universal across sizes/materials.
- Ask the distributor to select a reinforced sleeve for **abrasion + alkaline cement + glass-fiber loading**, rather than choosing elastomer by name alone.
- Ask about **VMC / removable connection configurations** if frequent dismantling and flushing are priorities; the distributor lists an aseptic VMC option, but your slurry still needs approval.
- Distributor product telephone: **03-3730-5991**.
- [AKO's distributor directory](https://www.pinch-valves.com/distributors) independently lists Japan Machinery as its Japan distributor.

### 3. Flowrox through RIX: heavy-duty or no-compressor route

- RIX documents full-bore opening and sleeve-only fluid contact, with pneumatic, hydraulic, and **electric-actuator** options.
- The general family starts at **25 mm** bore. RIX's broad family pressure/temperature ranges are not ratings for any particular ordered valve.
- Electric versions are described as having limit switches, torque switches, and manual emergency operation. **Ask for actuator supply voltage and control wiring before ordering; 24 V DC is not established by this listing.**
- If compressed air is unavailable, this is the stronger industrial electric-actuation inquiry compared with a small laboratory tube valve.
- RIX also lists the pneumatic **PVEG** in 50/80/100/150 mm sizes: actuated process-pressure range **0–6 bar**, with supply air **at least 6 bar**. Have RIX verify the pressure/force margin at your maximum differential pressure.
- The supplier describes a sleeve-flexing self-cleaning effect. **This does not mean cured cement will clean itself out.** Immediate flushing is still required.
- [Catalog request/download](https://www.rix.co.jp/products_services/download/form.php?docids=165) · [Electric-actuator sleeve-replacement manual request](https://www.rix.co.jp/products_services/download/form.php?docids=171).

### 4. OKM 704: concrete purchasing and price reference

Prices displayed by Haikanbuhin.com when checked; tax included, excluding listed special shipping and all automation accessories:

| Model | Nominal bore | Displayed price | Direct purchase link |
|---|---|---|---|
| 704-25 | 25 mm | **¥130,343** | [704-25](https://www.haikanbuhin.com/top/detail/asp/sku_detail.asp?scode=01410172&gcode=50256) |
| 704-40 | 40 mm | **¥168,377** | [704-40](https://www.haikanbuhin.com/top/detail/asp/sku_detail.asp?scode=01410173&gcode=50256) |
| 704-50 | 50 mm | **¥187,008** | [704-50](https://www.haikanbuhin.com/top/detail/asp/sku_detail.asp?scode=01410174&gcode=50256) |

- Each of these listings shows **¥2,200 special shipping**; reconfirm at checkout.
- Current manufacturer information lists **704: 25–100 mm, JIS10K flanges, maximum working pressure 0.4 MPa, 0–80°C**, air-pressure operation.
- **Source discrepancy:** the Haikanbuhin listing describes 0.3 MPa and 0–70°C, and a wider size range. Obtain the exact current datasheet for the ordered model; do not design to the more generous rating without written confirmation. JIS10K flange designation is not the sleeve's process-pressure rating.
- The retailer specifies air pressure **0.20–0.30 MPa above fluid pressure** and a replaceable CR rubber sleeve.
- Manufacturer lists cement, ore slurry, sludge, and pulp applications. Glass-fiber mixture performance still needs verification.
- [Manufacturer operating manual](https://sol.okm-net.jp/wp/wp-content/uploads/2025/08/manual-pinch-valve.pdf) · [Handling precautions](https://sol.okm-net.jp/wp/wp-content/uploads/2025/08/handling-pinch-valve.pdf).

### 5. GEMÜ Q50 eSyStep: electric, easy tube change, but not the default concrete choice

- Japanese supplier listing specifies **24 V DC**, motorized actuation, on/off or integrated-positioner versions, and **IP65**.
- Good access to the wetted path: remove/replace the process tube instead of cleaning internal valve seats.
- Marketed primarily for single-use pharmaceutical/biotech applications, **not heavy abrasive concrete**. Only consider it for a small, low-pressure, fine mortar/additive line if GEMÜ approves the actual tube and mixture.
- Pressure and temperature depend on the approved tube/valve combination. Ordinary silicone laboratory tubing should not be assumed suitable for pumped concrete.
- The translated Japanese listing mixes in pneumatic information; obtain the actual **Q50 datasheet and wiring diagram** rather than relying on its generic control-function/air-pressure text.
- IP65 does **not** mean pressure-washer, immersion, or hot/chemical washdown approval.

## Controlling these with your existing Raspberry Pi

You do not need to buy an Arduino just to operate one valve. None of these industrial actuators should be powered directly from GPIO.

### Pneumatic valve layout

```text
Raspberry Pi GPIO (3.3 V)
  -> compatible isolated output / relay or solenoid driver
  -> separately powered 24 V DC pneumatic solenoid
  -> regulated compressed air
  -> industrial pinch valve
```

- Direct air-pressure sleeve valves generally need a **3/2 air valve** that can pressurize and exhaust the sleeve chamber; a double-acting air cylinder generally needs a **5/2 valve**. Confirm the circuit with the supplier.
- Additional items: compressor or plant air, filter/regulator/gauge, appropriate air tubing/fittings, air solenoid, DC power supply, protected electrical enclosure, and preferably valve-state/pressure feedback.
- Match solenoid port size and flow capacity to the required opening/closing time. Air supply pressure must exceed process pressure by the specified margin **without exceeding valve/air-component limits**.
- For Arduino, use the same external driver approach, matched to the board's logic voltage. Pi GPIO is not 5 V tolerant; isolate/translate industrial 24 V feedback.
- Use correctly rated coil transient suppression and fusing; do not assume a generic relay module accepts 3.3 V logic or safely switches the coil.

### Electric actuator layout

```text
Raspberry Pi / Arduino
  -> isolated interface matched to actuator controls
  -> separately powered electric pinch-valve actuator
```

- Request an **on/off version with discrete open/close inputs or dry-contact controls**. No analog positioning interface, DAC, integrated positioner, or IO-Link master is needed if the ordered version supports these simple inputs. Open/closed feedback remains useful even without fine adjustment.
- Power voltage, motor current, wiring, interlocks, and feedback depend on the ordered actuator. If it requires mains power, use a qualified electrician and enclosed, appropriately rated equipment.
- Do not use Raspberry Pi/Linux timing as a safety system. Provide a hardware emergency stop and pump interlock.
- **Fail state matters:** direct air-pressure pinch valves commonly open when control air is lost. Ask explicitly for the required power-loss/air-loss state; do not assume “normally closed” just because the solenoid is normally closed.
- Interlock closing with the concrete pump to avoid dead-heading/overpressure. Include properly engineered pressure protection.

## Cleaning and durability requirements

“Washable” has two separate meanings: **the internal flow path can be flushed/disassembled**, and **the outside actuator/electrics can tolerate washing**. The shortlist favors accessible, replaceable wetted sleeves, but washdown ratings are not established for most configurations.

1. Flush promptly after use, before cement sets; run compatible rinse water through the open valve and cycle it only as the manufacturer permits.
2. Provide removable connections and a drain/flush arrangement without unnecessary pockets. Flange removal requires depressurizing and safely supporting the line.
3. Confirm cleaning-water pressure/temperature, allowed chemicals, external wash method, and actuator/connector protection. Keep the Pi in a protected enclosure away from spray.
4. Keep spare sleeves. Sand and glass fibers are abrasive; sleeves are consumables, not lifetime parts.
5. Never assume sleeve squeezing can break up hardened cement safely. Isolate, depressurize, and service the valve according to its manual.
6. Capture alkaline cement/fiber rinse waste; do not assume it can be discharged into a drain.

Avoid ordinary pilot-operated water solenoid valves, reduced-bore fittings, and inexpensive irrigation ball valves in the wet mixture line. These can clog, trap cement around seats/cavities, and suffer abrasive wear. They can still be useful on a **separate clean-water line**, within their ratings.

## Information needed before ordering

- Hose/pipe **internal diameter** and connection type (JIS flange, hose coupling, etc.).
- Whether this is wet fiber-reinforced mortar, coarse-aggregate concrete, clean water, additives, or dry fiber feed.
- Maximum aggregate size; fiber length, concentration, and tendency to clump. No universal bore-to-fiber ratio is assumed here.
- Maximum operating pressure, pump dead-head/transient pressure, suction/vacuum conditions, and flow rate.
- **Open/close only (confirmed)**; specify no proportional control or positioner. If batching by quantity is added later, use measured mass/flow feedback rather than assuming elapsed opening time gives a repeatable concrete dose.
- Available compressed-air pressure, required cycle speed/frequency, and safe failure state.
- Cleaning procedure, external spray exposure, footprint/weight, budget, and lead time.

### Japanese inquiry text to send suppliers

Include this requirement with the inquiry below:

> 開閉（ON/OFF）のみ必要で、比例制御・微調整・ポジショナーは不要です。

> ガラス繊維を混合したセメント系モルタル／コンクリート用の自動ピンチバルブを探しています。Raspberry PiのGPIOから絶縁インターフェースを介して制御する予定です。配管内径は___mm、最大粒径は___mm、繊維長は___mm、繊維含有量は___、最大使用圧力は___MPa、流量は___L/minです。使用後は硬化前に水洗し、必要に応じてスリーブを取り外して清掃したいです。この混合物に適した型式、耐摩耗スリーブ、圧力定格、洗浄方法、外部洗浄への耐性、交換部品、価格、納期をご提案ください。空圧式の場合は必要な供給空気圧と電磁弁仕様、電動式の場合は電源電圧と制御入力仕様、また停電・エア喪失時の動作もご教示ください。

**Bottom line:** seek a supplier-approved industrial pinch valve for the wet mixture. SB Valve / AKO are good first quote requests; OKM gives a real online price reference; Flowrox is the priority inquiry if you want industrial electric actuation without compressed air.
