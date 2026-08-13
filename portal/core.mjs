export const MAX_BATTERIES = 4;

export function isTianpowerDevice(device) {
  return typeof device?.name === "string" && /^TP_/i.test(device.name.trim());
}

export function normalizeDevices(devices = []) {
  const seen = new Set();
  return devices
    .filter(isTianpowerDevice)
    .filter((device) => {
      const mac = String(device.mac || "").toUpperCase();
      if (!mac || seen.has(mac)) return false;
      seen.add(mac);
      return true;
    })
    .map((device) => ({
      name: device.name.trim(),
      mac: String(device.mac).toUpperCase(),
      rssi: Number.isFinite(Number(device.rssi)) ? Number(device.rssi) : null,
    }))
    .sort((a, b) => (b.rssi ?? -999) - (a.rssi ?? -999));
}

export function maskMac(mac) {
  const parts = String(mac || "").toUpperCase().split(":");
  if (parts.length !== 6) return "Adresă protejată";
  return `${parts[0]}:${parts[1]}:**:**:${parts[4]}:${parts[5]}`;
}

export function signalLabel(rssi) {
  if (rssi === null || rssi === undefined) return "Necunoscut";
  if (rssi >= -60) return "Excelent";
  if (rssi >= -72) return "Bun";
  if (rssi >= -85) return "Slab";
  return "Foarte slab";
}

export function validateConfiguration(config) {
  const errors = [];
  const ssid = String(config?.wifi?.ssid || "").trim();
  const password = String(config?.wifi?.password || "");
  const batteries = Array.isArray(config?.batteries) ? config.batteries : [];

  if (!ssid) errors.push("Selectează rețeaua Wi-Fi locală.");
  if (password.length > 0 && password.length < 8) {
    errors.push("Parola Wi-Fi trebuie să aibă minimum 8 caractere.");
  }
  if (batteries.length < 1) errors.push("Selectează cel puțin o baterie.");
  if (batteries.length > MAX_BATTERIES) {
    errors.push(`Poți selecta maximum ${MAX_BATTERIES} baterii.`);
  }

  const macs = new Set();
  batteries.forEach((battery, index) => {
    const mac = String(battery?.mac || "").toUpperCase();
    if (!mac || macs.has(mac)) errors.push(`Bateria ${index + 1} nu este unică.`);
    macs.add(mac);
    if (!String(battery?.friendly_name || "").trim()) {
      errors.push(`Completează numele bateriei ${index + 1}.`);
    }
  });

  return errors;
}

export function buildConfiguration({ ssid, password, batteries }) {
  return {
    wifi: { ssid: String(ssid).trim(), password: String(password) },
    batteries: batteries.slice(0, MAX_BATTERIES).map((battery, index) => ({
      slot: index + 1,
      mac: String(battery.mac).toUpperCase(),
      advertised_name: battery.name,
      friendly_name: String(battery.friendly_name || `Bateria ${index + 1}`).trim(),
    })),
  };
}
