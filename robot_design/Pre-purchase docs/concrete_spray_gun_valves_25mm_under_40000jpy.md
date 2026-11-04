# Concrete spray-gun shutoff valves: 25 mm, budget ¥40,000/unit

Researched: 2026-10-08. This is a new budget-specific shortlist; the earlier general valve report is unchanged.

## Requirements

- Application: concrete/mortar spray gun, including glass fiber as described earlier.
- Operation: **open/close only**, controlled by an existing Raspberry Pi or Arduino through an external driver.
- Hose: **25 mm**, provisionally interpreted as **internal diameter**. Confirm ID, OD, and connection standard before ordering.
- Budget: **maximum ¥40,000 per valve**. Treat this as a tax-included valve budget for selection; shipping, fittings, air solenoid, driver, and power supply must be priced separately. If ¥40,000 is the complete installed-unit limit, tell suppliers that explicitly.
- Cleaning: flushable before cement sets, replaceable wetted sleeve, and accessible connections for dismantling.

## Findings and recommendation

**I did not verify a ready-to-use, concrete/glass-fiber-qualified DN25 automatic valve with a confirmed Japan-delivered price below ¥40,000.** There are useful budget leads, listed below with their uncertainties rather than presented as guaranteed matches.

1. **First choice to request a Japanese quote: AKO VMP DN25 with abrasion-resistant natural-rubber sleeve.** Compact, lighter than a flanged industrial valve, documented pressure limits and spare parts. Ask Japan Machinery for a price within ¥40,000.
2. **Lowest published-price lead: Highend DN25 pneumatic pinch valve on Alibaba, displayed at ¥14,717.** Potentially within budget, but only buy after receiving adequate pressure/sleeve documentation, mixture approval, and a delivered-to-Japan quote.
3. **Additional domestic quote: OSCAR / Yamamoto Sangyo Series 80, nominal G1 connection.** Ask for an actual 25 mm unobstructed bore and a slurry-compatible sleeve. Price and concrete suitability are unconfirmed.

Pinch valves still look preferable to cheap ball valves for this application: no ball-seat cavity to collect cement, and the wetted sleeve is replaceable. They are not automatically suitable for high-pressure concrete pumping.

## Candidate table

| Candidate | Price evidence | Japan purchasing route | Budget / application status |
|---|---|---|---|
| **AKO VMP025.03XK.71** (white POM, G-thread) or supplier-selected DN25 VMP variant | Japanese quote not published. A related black-POM NPT variant is US$180.47 in the US shop | [Japan Machinery product](https://www.jmc.asia/products/ako) · [Japanese quote request](https://www.jmc.asia/contact/) · [Exact G-thread product/specifications](https://www.ako-shop.com/en/air-operated-pinch-valves/internal-thread/VMP025.03XK.71) | Best documented lead; **not confirmed below ¥40,000 in Japan**; supplier must approve mix and pressure |
| **Highend Intelligent Technology DN25 threaded pneumatic pinch valve** | Marketplace page displayed **¥14,717**, minimum order 1 | [Product listing](https://www.alibaba.com/product-detail/Industrial-Pneumatic-Pinch-Valve-DN25-Thread_1601887417642.html) · [Price-visible marketplace page](https://www.alibaba.com/pla/Industrial-Pneumatic-Pinch-Valve-DN25-Thread_1601887417642.html?mark=google_shopping&biz=pla&searchText=pneumatic+parts&product_id=1601887417642&pcy=ca_en) | Item price below cap; **Japan shipping, tax, exact configuration, ratings, and fiber-concrete qualification unverified** |
| **OSCAR / Yamamoto Sangyo Series 80**, ask for G1 / 25 mm bore | No public price found | [Product family](https://www.oscarvalve.com/product_category02.html) · [Series 80](https://www.oscarvalve.com/product_detail.html?id=84) · [Japanese inquiry](https://www.oscarvalve.com/inquiry.html?id=1) | Domestic inquiry lead only; **budget and actual bore unconfirmed** |

### A. AKO VMP DN25: preferred quote target

The [VMP025.03XK.71 manufacturer shop page](https://www.ako-shop.com/en/air-operated-pinch-valves/internal-thread/VMP025.03XK.71) specifies:

- Nominal bore **25 mm**; POM body and ends; abrasion-resistant, high-resilience natural-rubber sleeve.
- Maximum process pressure **5.5 bar = 0.55 MPa**.
- Maximum control-air pressure **8 bar = 0.8 MPa**.
- Minimum control/process pressure differential **2.5 bar = 0.25 MPa**.
- Approximate weight **0.50 kg**, length **120 mm**, width **70 mm**, height **75 mm** for this variant.
- Body, ends, and sleeve are separately replaceable; page lists a piggable flow path.
- [Replacement sleeve M025.03XK](https://www.ako-shop.com/en/spare-single-parts/air-operated-pinch-valves/M025.03XK).

**Pressure example:** at 0.40 MPa material pressure, this configuration requires at least approximately 0.65 MPa control pressure at the valve, subject to the manufacturer's operating instructions. A nominal 0.60 MPa air source is insufficient in that example. Check pressure while the spray system is consuming air, not just compressor idle pressure.

**Price reference, not a Japan offer:** [VMP025.03XK.72/NPT in the US AKO/Basecamp shop](https://shop.akopinchvalves.com/products/vmp025-03xk-72-npt) is displayed at **US$180.47**. For illustration only, at ¥150/USD this is approximately **¥27,071 before freight, taxes, fees, and accessories**. This is not a current exchange-rate claim or a price for the G-thread variant.

The US shop's [shipping policy](https://shop.akopinchvalves.com/policies/shipping-policy) limits eligible online orders to **US and Canada**, so it is **not a direct Japan purchase link**. Use the price as a negotiation/reference point and request the correct configuration through Japan Machinery. Do not assume the Japanese quote will match it.

Ask specifically for the **lowest-cost DN25 VMP with abrasion-resistant sleeve**, not a premium stainless hygienic valve, electric positioner, or large flanged actuator assembly. Confirm protection against impact and cleaning chemicals for the plastic body. Supplier approval for the actual fiber/mortar mix remains necessary.

### B. Highend DN25: inexpensive import/sample lead

- Seller identified by the price-visible page: **Highend Intelligent Technology (Zhejiang) Co., Ltd.**
- Product title: “Industrial Pneumatic Pinch Valve DN25 Thread End Bubble Tight Shut-off Valve for Corrosive Slurry Granule Liquid Media.”
- Observed price: **¥14,717**, MOQ 1. Earlier search results showed US$82.80; regional display/conversion/configuration can vary. Neither is a fixed landed quote.
- The title supports investigating slurry service, **not assuming suitability for concrete containing glass fiber**.
- The detailed product page did not yield usable specifications during this research. Maximum material pressure, control pressure, differential, sleeve material, spare-sleeve availability, true bore, and external-wash resistance were not verified.
- Request a written **DN25-specific** datasheet, pressure-test documentation, dimensional drawing, sleeve replacement instructions, and quotation including shipping/taxes to your Japanese address.
- Ask whether the displayed price is for the complete DN25 valve rather than a sleeve, another size, or optional configuration.

**Purchase gate:** no production use on a pressurized spray gun without documented ratings and supplier approval. A cheap item price is not evidence of pressure safety or ruggedness. If adequate documentation is supplied, qualify a sample with your exact mix and cleaning procedure before buying multiple units.

### C. OSCAR Series 80: additional Japanese supplier lead

- Series spans **G3/8 to G2** connections, aluminum-alloy body, and POM or SUS316 connections.
- Published process-pressure rating: **0.4 MPa**. Listed sleeve choices: NBR, EPDM, Viton.
- A G1 thread does not itself guarantee a clear 25 mm flow bore; request a drawing.
- The Japanese pilot-pressure wording is ambiguous; obtain the required closing differential and maximum pilot pressure directly from the supplier instead of interpreting it as a design rule.
- Request an abrasion-qualified sleeve for cement and glass fiber; do not assume an ordinary chemical-compatible sleeve is abrasion-resistant.
- Quote request telephone: **072-730-0202**.
- **No evidence yet that this fits ¥40,000.** Include only as a price-capped inquiry, not a verified budget option.

## Options excluded from this budget shortlist

| Option | Reason |
|---|---|
| [OKM 704-25](https://www.haikanbuhin.com/top/detail/asp/sku_detail.asp?scode=01410172&gcode=50256) | Previously checked online price **¥130,343 incl. tax**, before shipping/accessories |
| [CKD NPV2-25A-N](https://store.shopping.yahoo.co.jp/firstnet/yam-nnk28.html) | Search result listed **¥65,540**, already above cap; reconfirm if comparing later |
| Flowrox industrial electric-actuated valve | No verified sub-¥40,000 price; not a budget-qualified option |
| Miniature medical/lab pinch valves | Low prices often correspond to millimeter-scale tubing, not a 25 mm concrete hose |
| Ordinary 1-inch motorized water ball valves | Price can fit, but “1 inch” does not ensure full bore; seats/cavities collect and abrade with cement/fiber. Not recommended as a production concrete shutoff without explicit slurry qualification |

## Spray-gun installation and control

```text
Material: pump -> 25 mm rated material hose -> DN25 pinch valve -> spray nozzle
Control:  Raspberry Pi -> 3.3 V-compatible isolated driver -> 24 V DC air solenoid
Air:      compressor -> filter/regulator -> air solenoid -> pinch-valve air chamber
```

- For a direct air-pressure pinch valve, use a supplier-approved **3/2 pneumatic circuit** that both pressurizes and exhausts the sleeve chamber. The air solenoid does not carry concrete. No fine-control hardware is needed.
- Spray/atomizing air and sleeve-control air serve different functions; size/regulate their branches so spraying does not deprive the pinch valve of closing pressure.
- Confirm required open/close time under actual material pressure. Long control-air lines can slow operation. Locate hardware according to supplier guidance and protect electrical parts from spray.
- Keep all material fittings and couplings pressure-rated and approximately full bore. **G/BSPP, Rc/BSPT, and NPT are different thread systems**; order matching adapters and seals, not forced-fit connections.
- Support the valve independently where possible to avoid hose loads or impact on plastic threads. Material downstream of the valve can still drip after closure; minimize this volume while retaining safe service access.
- Keep GPIO away from 24 V. Provide a separate fused supply, correctly rated coil driver/suppression, and protected electronics. No Arduino is necessary just to switch one valve.

### Critical pressure and shutoff conditions

**Before selecting any model, identify maximum material pressure at the gun, including pump stall/dead-head pressure and transients.** Atomizing-air pressure is not material-line pressure. A 25 mm mortar hose can carry pressure far above the compact valve ratings above.

- Do not close a gun-mounted valve against a running positive-displacement pump without a manufacturer-approved pump-stop/bypass/pressure-protection scheme.
- Direct-air pinch valves normally reopen when closing air is exhausted/lost. They are **not inherently fail-closed**. Specify a safe air-loss/power-loss response and hardware pump interlock.
- Provide a hardware emergency stop; Raspberry Pi software is not the safety layer.
- Closing the valve or stopping the pump does not establish that the hose is depressurized. Never loosen couplings for cleaning until the system is isolated and residual pressure has been relieved according to its instructions.
- If peak material pressure exceeds the selected rating, reject that valve even if the price and diameter fit.

## Cleaning

- Flush with compatible water **before cement sets**, including the valve-to-nozzle section. Follow supplier guidance on cycling during flushing.
- Prefer removable connections and replaceable sleeves; keep a spare sleeve and inspect wear from sand/glass fiber.
- “Flushable inside” does not mean “pressure-washable outside.” Confirm permitted spray, temperature, and cleaning chemicals; protect the solenoid, connectors, and Pi.
- Do not solve cost by squeezing an ordinary reinforced concrete hose with a homemade clamp. It may not close fully and can damage the reinforcement. A DIY mechanism would need an approved pinch sleeve and engineered pressure/force/life validation; it is not a verified purchase option here.

## Quote-request text (Japanese)

> コンクリート／モルタル吹付ガン用の自動開閉バルブを探しています。材料ホース内径は25mmで、ガラス繊維を含むセメント系材料を使用します。開閉（ON/OFF）のみ必要で、比例制御やポジショナーは不要です。予算はバルブ本体1台あたり税込40,000円以内です。Raspberry Piから絶縁ドライバとDC24V電磁弁を介して操作する予定です。最大材料圧力は___MPa（ポンプ停止／閉塞時の最大圧力___MPa）、供給空気圧は___MPa、最大粒径は___mm、繊維長は___mm、繊維含有量は___です。耐摩耗スリーブ、実流路径25mm、必要制御空気圧、開閉時間、停電・エア喪失時の動作、洗浄方法、交換スリーブ価格、送料、納期をご提示ください。上記材料と圧力条件に対応可能か書面でご確認をお願いします。

## Next decision

**Request a price-capped AKO VMP DN25 quote first; evaluate the ¥14,717 import only if its documentation is adequate.** Neither is yet a confirmed under-budget production purchase for this exact spray gun. The most important missing information is **maximum material pressure**, followed by whether the stated 25 mm is ID and the fiber/aggregate dimensions.
