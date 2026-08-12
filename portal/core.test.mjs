import assert from "node:assert/strict";
import test from "node:test";

import {
  MAX_BATTERIES,
  buildConfiguration,
  maskMac,
  normalizeDevices,
  validateConfiguration,
} from "./core.mjs";

test("keeps only unique TP devices and sorts strongest first", () => {
  const result = normalizeDevices([
    { name: "Other", mac: "00:00:00:00:00:01", rssi: -30 },
    { name: "TP_WEAK", mac: "AA:BB:CC:DD:EE:01", rssi: -80 },
    { name: "tp_strong", mac: "AA:BB:CC:DD:EE:02", rssi: -50 },
    { name: "TP_DUP", mac: "AA:BB:CC:DD:EE:02", rssi: -40 },
  ]);
  assert.deepEqual(result.map((item) => item.name), ["tp_strong", "TP_WEAK"]);
});

test("masks the private part of a BLE address", () => {
  assert.equal(maskMac("50:cf:14:a0:ea:c6"), "50:CF:**:**:EA:C6");
});

test("rejects more than six batteries", () => {
  const batteries = Array.from({ length: MAX_BATTERIES + 1 }, (_, index) => ({
    mac: `AA:BB:CC:DD:EE:0${index}`,
    friendly_name: `Battery ${index}`,
  }));
  assert.match(validateConfiguration({ wifi: { ssid: "LAN" }, batteries }).join(" "), /maximum 6/i);
});

test("builds stable one-based slots", () => {
  const result = buildConfiguration({
    ssid: " Local LAN ",
    password: "password123",
    batteries: [{ name: "TP_ONE", mac: "aa:bb:cc:dd:ee:01", friendly_name: "Rack 1" }],
  });
  assert.equal(result.wifi.ssid, "Local LAN");
  assert.equal(result.batteries[0].slot, 1);
  assert.equal(result.batteries[0].mac, "AA:BB:CC:DD:EE:01");
});
