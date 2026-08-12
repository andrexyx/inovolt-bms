import { MAX_BATTERIES, buildConfiguration, maskMac, normalizeDevices, signalLabel, validateConfiguration } from "./core.mjs";

const demoWifi = [
  { ssid: "Casa WiFi", rssi: -48, secure: true },
  { ssid: "Home Assistant IoT", rssi: -66, secure: true },
  { ssid: "Rețea oaspeți", rssi: -79, secure: true },
];
const demoBms = [
  { name: "TP_BSTBD-24I-101", mac: "50:CF:14:A0:10:01", rssi: -52 },
  { name: "TP_BSTBD-24I-102", mac: "50:CF:14:A0:10:02", rssi: -64 },
  { name: "TP_LV-103", mac: "50:CF:14:A0:10:03", rssi: -76 },
];

const state = { devices: [], selected: new Map() };
const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => [...document.querySelectorAll(selector)];

async function api(path, options) {
  const response = await fetch(path, { ...options, headers: { "content-type": "application/json", ...(options?.headers || {}) } });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

function showStep(step) {
  $$("[data-panel]").forEach((panel) => { panel.hidden = panel.dataset.panel !== String(step); panel.classList.toggle("active", !panel.hidden); });
  $$(".step").forEach((item) => item.classList.toggle("active", Number(item.dataset.step) <= step));
  if (step === 3) renderSummary();
}

function renderWifi(networks) {
  $("#wifi-list").innerHTML = networks.map((network) => `<button class="wifi-choice" data-ssid="${escapeHtml(network.ssid)}"><span>◉</span><span><strong>${escapeHtml(network.ssid)}</strong><small>${signalLabel(network.rssi)} · ${network.secure ? "Securizată" : "Deschisă"}</small></span></button>`).join("");
  $$(".wifi-choice").forEach((button) => button.addEventListener("click", () => { $("#wifi-ssid").value = button.dataset.ssid; }));
}

function renderBatteries() {
  $("#selected-count").textContent = state.selected.size;
  $("#battery-list").innerHTML = state.devices.length ? state.devices.map((device) => {
    const selected = state.selected.get(device.mac);
    return `<article class="battery ${selected ? "selected" : ""}" data-mac="${device.mac}"><input type="checkbox" ${selected ? "checked" : ""} aria-label="Selectează ${escapeHtml(device.name)}"><div class="battery-info"><strong>${escapeHtml(device.name)}</strong><small>${maskMac(device.mac)} · Semnal ${signalLabel(device.rssi)}</small></div><input class="battery-name" value="${escapeHtml(selected?.friendly_name || device.name.replace(/^TP_/i, "Bateria "))}" aria-label="Numele bateriei" ${selected ? "" : "disabled"}></article>`;
  }).join("") : '<p class="help">Apasă „Scanează Bluetooth” pentru a găsi bateriile.</p>';

  $$(".battery").forEach((card) => {
    const checkbox = card.querySelector('input[type="checkbox"]');
    const nameInput = card.querySelector(".battery-name");
    checkbox.addEventListener("change", () => {
      const device = state.devices.find((item) => item.mac === card.dataset.mac);
      if (checkbox.checked && state.selected.size >= MAX_BATTERIES) { checkbox.checked = false; alert(`Poți selecta maximum ${MAX_BATTERIES} baterii.`); return; }
      if (checkbox.checked) state.selected.set(device.mac, { ...device, friendly_name: nameInput.value }); else state.selected.delete(device.mac);
      renderBatteries();
    });
    nameInput.addEventListener("input", () => { const item = state.selected.get(card.dataset.mac); if (item) item.friendly_name = nameInput.value; });
  });
}

function currentConfiguration() {
  return buildConfiguration({ ssid: $("#wifi-ssid").value, password: $("#wifi-password").value, batteries: [...state.selected.values()] });
}

function renderSummary() {
  const config = currentConfiguration();
  $("#summary").innerHTML = `<div class="summary-card"><strong>Rețea locală</strong><p>${escapeHtml(config.wifi.ssid || "Neselectată")}</p></div><div class="summary-card"><strong>Baterii (${config.batteries.length})</strong>${config.batteries.map((battery) => `<p>${battery.slot}. ${escapeHtml(battery.friendly_name)} · ${maskMac(battery.mac)}</p>`).join("") || "<p>Nicio baterie selectată</p>"}</div>`;
  $("#errors").textContent = validateConfiguration(config).join(" ");
}

function escapeHtml(value) { const element = document.createElement("span"); element.textContent = String(value); return element.innerHTML; }

$("#scan-wifi").addEventListener("click", async () => { $("#wifi-list").innerHTML = '<p class="help">Caut rețele locale…</p>'; try { renderWifi(await api("/api/wifi/scan")); } catch { renderWifi(demoWifi); } });
$("#scan-bms").addEventListener("click", async () => { $("#battery-list").innerHTML = '<p class="help">Scanez dispozitivele Bluetooth…</p>'; try { state.devices = normalizeDevices(await api("/api/bms/scan")); } catch { state.devices = normalizeDevices(demoBms); } renderBatteries(); });
$$('[data-next]').forEach((button) => button.addEventListener("click", () => showStep(Number(button.dataset.next))));
$$('[data-back]').forEach((button) => button.addEventListener("click", () => showStep(Number(button.dataset.back))));
$("#save").addEventListener("click", async () => {
  const config = currentConfiguration(); const errors = validateConfiguration(config); $("#errors").textContent = errors.join(" "); if (errors.length) return;
  $("#save").disabled = true; $("#save").textContent = "Salvez…";
  try { await api("/api/config", { method: "POST", body: JSON.stringify(config) }); } catch { localStorage.setItem("inovolt-demo-config", JSON.stringify({ ...config, wifi: { ...config.wifi, password: "" } })); }
  $$(".panel,.steps").forEach((item) => item.hidden = true); $("#success").hidden = false;
});

renderWifi(demoWifi); renderBatteries();
