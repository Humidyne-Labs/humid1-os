# APP_AUDIT_01: Full-Stack Infrastructure Audit & API Extension Guide (ThingsBoard, Web Dashboard & H1OS)

* **Audit ID:** `APP-AUDIT-001` (2026-10-27)
* **Audited Components:**
  1. **ThingsBoard CE Rule Chains & Device Profile** (`_thingsboard/rule_chains/*.json`, `device_profile/f00235c0-a123-11f1-b530-9b9631e0c365.json`)
  2. **Customer Web Dashboard Source** (`_web_dash_src/src/services/thingsboard.ts`, `alarmThresholds.ts`, `ControlPanel.tsx`, `HistoricalChart.tsx`, `ClaimDeviceModal.tsx`)
  3. **H1OS v2.4.4-dev Operating System Middleware** (`_h1os_src_code/CMakeLists.txt`, `include/h1os/*.h`, `src/h1os_lifecycle.c`)
  4. **Humid1-OS Repository & Agent Tooling** (`docs/architecture/*`, `firmware/*`, `.agents/*`)
* **Final Disposition:** **REMEDIATION & EXTENSION GUIDE ISSUED** (7 Critical Infrastructure Findings + Complete Patch Specifications)

---

## 1. Executive Summary of Findings

A cross-layer audit comparing `excecutive_summery.txt` against the exported ThingsBoard rule chains, Web Dashboard TypeScript code, H1OS v2.4.4-dev source code, and the initial `humid1-os` repository revealed **seven cross-system discrepancies and defects**. Resolving these ensures that the firmware, cloud rule engine, and customer web dashboard operate on a unified contract:

| Finding ID | Target Layer | Severity | Summary of Defect / Architectural Gap |
| :--- | :--- | :---: | :--- |
| **`INFRA-01`** | ThingsBoard `Device Setup Chain` & `Alarms` Rule Chain | `S1` | Missing `flow_a_period_sec`, `device_nickname`, `telemetry_schema_mask`, and `alarm_state` Shared Attributes (needed so deep-sleeping devices receive active server alarms and dynamic Flow A/B periods on wake). |
| **`INFRA-02`** | ThingsBoard `Alarms` Rule Chain (`Master Evaluator` Node 5) | `S1` | Hysteresis (`rhHist`, `tempHist`, `battHist`) is applied only on the `WARNING <-> NORMAL` boundary, not on the `CRITICAL <-> WARNING` boundary, causing rapid alarm severity flapping and notification spam. |
| **`INFRA-03`** | ThingsBoard `POST FCM Alerts` Rule Chain | `S2` | `Create Alert Payload` contains logic for `metadata.action === "ClearAlarm"`, but the `Alarms` Rule Chain only wires `Created` and `Updated` links (omitting `Cleared`) to `SEND FCM Alarm`. |
| **`INFRA-04`** | ThingsBoard `Boot RPC Trigger` Rule Chain | `S1` | Node 1 (`prep`) connects to `TbSendRPCRequestNode` (server-to-device request) instead of `TbSendRPCReplyNode` (device-initiated RPC response), and SNTP is still required prior to TLS handshake. |
| **`INFRA-05`** | Web Dashboard (`ControlPanel.tsx` & `thingsboard.ts`) | `S1` | Three code paths hardcode Fahrenheit-to-Kelvin/Celsius math `(temp - 32) * 5/9` without calling `toKelvinTemp()`, producing $\sim 418\,\text{K}$ ($293^\circ\text{F}$) when the device publishes canonical Kelvin (`294.26 K`). |
| **`INFRA-06`** | H1OS v2.4.4-dev Middleware (`CMakeLists.txt` & `h1os.h`) | `S0` / `S1` | Missing `.ttf`/`.woff` font files in `firmware/tools/fonts/` breaks CMake build (`MANDATE-01`); missing public `h1os_set_flow_periods()` API prevents runtime Flow A/B updates without including `orch_state.h` (`MANDATE-02`). |
| **`INFRA-07`** | `humid1-os` Repo Docs & `.agents/` Tooling | `S1` | Initial `APP_02` docs omitted two-phase auto-provisioning + claiming and used non-standard telemetry keys; `.agents/skills/` referenced `examples/H1OS` instead of `firmware/` and lacked `firmware/tools/agent/` scripts. |

---

## 2. ThingsBoard Rule Chain & Schema Extensions (`INFRA-01` .. `INFRA-04`)

### 2.1 `INFRA-01`: Extending `Device Setup Chain` (`838f4410-a3f7-11f1-9ad5-91546bc9dd79.json`) & Alarm Sync

#### Problem
1. The user specification states:
   - *"The wake times for flow A&B will be dynamic per user setting in the dashboard, we need to make sure the device follows the config."*
   - *"All alarms will be handled by the things board server... TB offers a mqtt channel for alarms, we can monitor that, and alert the user as needed, if at all."*
2. Currently, `Device Setup Chain` Node 1 (`Default Attributes`) initializes `sleep_interval_sec: 60` (`Flow B`), but does not initialize `flow_a_period_sec`, `device_nickname`, `telemetry_schema_mask`, or `alarm_state`.
3. In ThingsBoard CE, `TbCreateAlarmNode` and `TbClearAlarmNode` create/clear alarm records in the server database, but **do not automatically publish an MQTT message to the device**. Furthermore, because the Humid1 hardware sleeps with Wi-Fi off for 15 minutes at a time (`Flow B`), it cannot receive live RPCs pushed while asleep.

#### Required Patch in `Device Setup Chain` (Node 1 `tbelScript`)
Update Node 1 (`Default Attributes`) in `838f4410-a3f7-11f1-9ad5-91546bc9dd79.json` to seed the complete server-driven contract upon device auto-provisioning:

```javascript
var defaultAttributes = {
    "sleep_interval_sec": 900,
    "sleep_interval_min": 15,
    "flow_a_period_sec": 60,
    "device_theme": "light",
    "sound_enabled": false,
    "auto_update_enabled": true,
    "manual_ota_trigger": false,
    "email_alerts_enabled": true,
    "temp_unit": "F",
    "device_nickname": "",
    "telemetry_schema_mask": 15,
    "alarm_thresholds": {
        "rhLowCritical": 60.0,
        "rhLowWarning": 62.0,
        "rhHighWarning": 72.0,
        "rhHighCritical": 75.0,
        "tempLowCritical": 288.71,
        "tempLowWarning": 290.93,
        "tempHighWarning": 295.37,
        "tempHighCritical": 297.59,
        "batteryLowCritical": 10.0,
        "batteryLowWarning": 20.0,
        "rhHist": 1.0,
        "tempHist": 0.3,
        "battHist": 2.0
    },
    "alarm_state": {
        "active": false,
        "severity": "NONE",
        "type": "",
        "message": "Nominal"
    }
};
return {msg: defaultAttributes, metadata: metadata, msgType: "POST_ATTRIBUTES_REQUEST"};
```

#### Required Patch in `Alarms` Rule Chain (`92da2e30-a9ab-11f1-9663-1fc19e23cbce.json`) for Device Alarm Delivery
In the `Alarms` Rule Chain, wire the output of `Master Evaluator` (or `Create/Clear Alarm` nodes) to a `TbMsgAttributesNode` (`Save Attributes`, scope `SHARED_SCOPE`, `notifyDevice: true`) that persists:

```json
{
  "alarm_state": {
    "active": true,
    "severity": "CRITICAL",
    "type": "RH Alarm",
    "message": "Humidity is critically HIGH (76.2%)."
  }
}
```

**Why this works in both Awake and Sleep modes:**
- When the device is **Awake (Interactive Mode)**, `notifyDevice: true` immediately pushes `alarm_state` over `v1/devices/me/attributes`.
- When the device is in **Deep Sleep**, it fetches `alarm_state` on its very next `Flow B` wake via `v1/devices/me/attributes/request/1`.

---

### 2.2 `INFRA-02`: Fixing `CRITICAL <-> WARNING` Hysteresis Flapping in `Alarms` Rule Chain

#### Problem
In `92da2e30-a9ab-11f1-9663-1fc19e23cbce.json`, Node 5 (`Master Evaluator`) defines `evalMetric()` as:

```javascript
if ((hiCrit && val >= hiCrit) || (loCrit && val <= loCrit)) {
    res.state = "CRITICAL";
    ...
} else if ((hiWarn && val >= hiWarn) || (loWarn && val <= loWarn)) {
    res.state = "WARNING";
```

Hysteresis (`hist`) is only checked in the `else` branch (`WARNING -> CLEAR`), **not** when stepping down from `CRITICAL` to `WARNING`. If `rhHighCritical = 75.0` and `rh` fluctuates between `74.95%` and `75.02%`, the alarm state alternates `CRITICAL` $\rightarrow$ `WARNING` $\rightarrow$ `CRITICAL` on every telemetry sample, triggering duplicate emails and push notifications.

#### Required Patch in `Master Evaluator` (`evalMetric` Function)
Apply hysteresis deadband on `CRITICAL -> WARNING` de-escalation by passing the previous severity stored in server scope (`prevSeverity`):

```javascript
function evalMetric(val, loCrit, loWarn, hiWarn, hiCrit, hist, name, unit, isTemp, tUnit, prevSeverity) {
    var res = { state: "NONE", severity: "WARNING", msg: "" };
    if (val === undefined || val === null || isNaN(val)) return res;

    var stayHiCrit = (prevSeverity === "CRITICAL" && hiCrit && val >= (hiCrit - hist));
    var stayLoCrit = (prevSeverity === "CRITICAL" && loCrit && val <= (loCrit + hist));

    if ((hiCrit && val >= hiCrit) || (loCrit && val <= loCrit) || stayHiCrit || stayLoCrit) {
        res.state = "CRITICAL";
        res.severity = "CRITICAL";
        var dir = ((hiCrit && val >= hiCrit) || stayHiCrit) ? "HIGH" : "LOW";
        res.msg = name + " is critically " + dir + " (" + formatVal(val, isTemp, tUnit) + unit + ").";
    } else if ((hiWarn && val >= hiWarn) || (loWarn && val <= loWarn)) {
        res.state = "WARNING";
        res.severity = "WARNING";
        var dirW = (hiWarn && val >= hiWarn) ? "HIGH" : "LOW";
        res.msg = name + " is " + dirW + " (" + formatVal(val, isTemp, tUnit) + unit + ").";
    } else {
        var hiClear = !hiWarn || (val < (hiWarn - hist));
        var loClear = !loWarn || (val > (loWarn + hist));
        if (hiClear && loClear) {
            res.state = "CLEAR";
        } else {
            res.state = "HOLD";
        }
    }
    return res;
}
```

---

### 2.3 `INFRA-03`: Wiring `Cleared` Alarms to `POST FCM Alerts`

#### Problem
In `POST FCM Alerts` (`aa7f9880-b168-11f1-8055-19de3a92853c.json`), Node 0 (`Create Alert Payload`) contains explicit handling for cleared alarms:

```javascript
if (metadata.action === "ClearAlarm") {
    title = "✅ RESOLVED: " + alarmType;
    body = "The " + alarmType.toLowerCase() + " on " + deviceName + " has returned to normal parameters.";
}
```

However, in `92da2e30-a9ab-11f1-9663-1fc19e23cbce.json` (`Alarms` Rule Chain), the `Clear Alarm` nodes (`Node 2`, `Node 8`, `Node 12`) have **zero outgoing connections** to `Node 14` (`SEND FCM Alarm`).

#### Required Patch
In `92da2e30-a9ab-11f1-9663-1fc19e23cbce.json`, add `{fromIndex: 2, toIndex: 14, type: "Cleared"}`, `{fromIndex: 8, toIndex: 14, type: "Cleared"}`, and `{fromIndex: 12, toIndex: 14, type: "Cleared"}` so mobile push notifications receive `"✅ RESOLVED"` events.

---

### 2.4 `INFRA-04`: Fixing `Boot RPC Trigger` (`32785e40-a4d3-11f1-9ad5-91546bc9dd79.json`)

#### Problem
In `Boot RPC Trigger`, Node 1 (`prep`) constructs a response to device-initiated `getCurrentTime` RPCs (`v1/devices/me/rpc/request/{id}`), but routes its output to `TbSendRPCRequestNode` (which initiates a *new* server-to-device RPC on `v1/devices/me/rpc/request/+` instead of replying on `v1/devices/me/rpc/response/{id}`).

#### Required Patch
Replace Node 0 (`org.thingsboard.rule.engine.rpc.TbSendRPCRequestNode`) with `org.thingsboard.rule.engine.rpc.TbSendRPCReplyNode` (`serviceIdMetaDataAttribute: "serviceId"`, `sessionIdMetaDataAttribute: "sessionId"`, `requestIdMetaDataAttribute: "requestId"`). Note that on the firmware side, SNTP (`net_sntp_ensure_valid_time()`) remains mandatory prior to MQTTS connection so that `esp_crt_bundle_attach` can validate the server's TLS certificate expiration timestamp during the TLS handshake.

---

## 3. Web Dashboard Kelvin Bug Fixes & UI Extensions (`INFRA-05`)

While `_web_dash_src/src/services/alarmThresholds.ts` correctly defines `toKelvinTemp(val)` (which detects values $> 200$ as Kelvin), **three files in `_web_dash_src/src/` bypass `toKelvinTemp()` and hardcode Fahrenheit conversion math**, causing severe temperature corruption when the firmware publishes canonical Kelvin (`~294.26 K`):

### 3.1 Bug 1: `src/components/ControlPanel.tsx` (Line 326)
- **Current Defective Code:**
  ```typescript
  const activeTempK = device.telemetry?.temp !== undefined
    ? (device.telemetry.temp - 32) * (5/9) + 273.15
    : undefined;
  ```
  When `device.telemetry.temp` is `294.26` K ($70.0^\circ\text{F}$), this computes `(294.26 - 32) * 5/9 + 273.15 = 418.85 K` ($294.26^\circ\text{F}$)!
- **Required Fix:**
  ```typescript
  const activeTempK = device.telemetry?.temp !== undefined
    ? toKelvinTemp(device.telemetry.temp)
    : undefined;
  ```

### 3.2 Bug 2: `src/services/thingsboard.ts` (`getHistory` Line 935)
- **Current Defective Code:**
  ```typescript
  const lastTemp = tempArr.length ? parseFloat(tempArr[tempArr.length - 1].value) : 70.0;
  ...
  tempC: Number(((lastTemp - 32) * (5 / 9)).toFixed(1)),
  ```
- **Required Fix:**
  ```typescript
  const lastTempK = tempArr.length ? toKelvinTemp(parseFloat(tempArr[tempArr.length - 1].value)) : 294.26;
  ...
  tempC: Number((lastTempK - 273.15).toFixed(1)),
  ```

### 3.3 Bug 3: `src/services/thingsboard.ts` (`evaluateAllAlarms` Line 1397)
- **Current Defective Code:**
  ```typescript
  const temp = tele.temp;
  if (temp !== undefined) {
    if (temp >= th.tempLowWarning && temp <= th.tempHighWarning) { ... }
  ```
- **Required Fix:**
  ```typescript
  const temp = tele.temp !== undefined ? toKelvinTemp(tele.temp) : undefined;
  ```

### 3.4 Recommended Dashboard UI Control Additions (`ControlPanel.tsx`)
To expose the new dynamic firmware settings specified in `APP_01` and `APP_02`, add two fields to `ControlPanel.tsx`:
1. **Sensor Sample Interval (`flow_a_period_sec`):** Dropdown/slider (`30s`, `60s`, `120s`, `300s`) alongside the existing Cloud Sync Interval (`sleep_interval_sec` / `Flow B`).
2. **Device Nickname (`device_nickname`):** Text input (`maxLength={24}`) written to `SHARED_SCOPE` (and mirrored to ThingsBoard entity `label`), allowing the user to name their humidor on both the Web Dashboard and the device's e-Paper header bar.

---

## 4. H1OS Middleware Mandates Summary (`INFRA-06`)

All required H1OS v2.4.4-dev changes are formally specified in [`docs/architecture/APP_07_os_api_mandates.md`](../architecture/APP_07_os_api_mandates.md):

1. **`MANDATE-01` (Font Generator & Missing TTF Assets):** `firmware/tools/generate_1bpp_fonts.py` has been hardened in this repository so that if `Montserrat-Bold.ttf`, `Montserrat-Medium.ttf`, `DejaVuSans.ttf`, and `FontAwesome5-Solid+Brands+Regular.woff` are not yet placed in `firmware/tools/fonts/`, it generates a clean LVGL built-in font bridge (`h1os_fonts_1bpp.c`) so `idf.py build` never fails.
2. **`MANDATE-02` (`h1os_set_flow_periods()`):** Implemented in `firmware/main/app_state.c` via `<h1os/orch_state.h>` until promoted to `h1os.h` in H1OS v2.5.0.
3. **`MANDATE-03` (Kelvin Fields in `h1os_telemetry_t`):** Provided via inline helpers `app_celsius_to_kelvin()` in `app_types.h` until added to `h1os_telemetry_t`.
4. **`MANDATE-04` (Wake Reason Context in `on_wake`):** Inspected cleanly via `orch_state_get()->last_wake_reason` inside `main.c`.
