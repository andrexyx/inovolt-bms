class InoVoltBatteryCard extends HTMLElement {
  static getConfigElement() {
    return document.createElement("inovolt-battery-card-editor");
  }

  static getStubConfig() {
    return {
      title: "Bateria 1",
      cells: Array.from({ length: 16 }, (_, index) => ({ name: `C${index + 1}`, entity: "", balancing_entity: "" })),
    };
  }

  setConfig(config) {
    if (!config) throw new Error("Configurația cardului lipsește");
    this.config = {
      title: "InoVolt Battery",
      segments: 7,
      columns: 4,
      cells: [],
      min_color: "#28a9ff",
      normal_color: "#20c768",
      max_color: "#ff4d58",
      balance_on_color: "#2ee875",
      balance_off_color: "#e0444e",
      empty_color: "#526159",
      equal_tolerance: 0.001,
      soc_colors: [
        { from: 0, color: "#e0444e" },
        { from: 20, color: "#f2a72e" },
        { from: 50, color: "#20c768" },
      ],
      ...config,
    };
    if (!this.shadowRoot) this.attachShadow({ mode: "open" });
    this.render();
  }

  set hass(hass) { this._hass = hass; this.render(); }
  getCardSize() { return 6; }
  value(entity) {
    const value = Number(this._hass?.states?.[entity]?.state);
    return Number.isFinite(value) ? value : null;
  }
  state(entity) { return String(this._hass?.states?.[entity]?.state || "").toLowerCase(); }
  escape(value) { const div = document.createElement("div"); div.textContent = String(value ?? ""); return div.innerHTML; }
  colorForSoc(soc) {
    return [...this.config.soc_colors].sort((a, b) => Number(a.from) - Number(b.from))
      .reduce((color, stop) => soc >= Number(stop.from) ? stop.color : color, this.config.empty_color);
  }
  openMoreInfo(entity) {
    this.dispatchEvent(new CustomEvent("hass-more-info", { bubbles: true, composed: true, detail: { entityId: entity } }));
  }

  render() {
    if (!this.config || !this.shadowRoot) return;
    const soc = this.value(this.config.soc_entity);
    const safeSoc = Math.max(0, Math.min(100, soc ?? 0));
    const headerSegments = Math.max(4, Math.min(20, Number(this.config.header_segments || 10)));
    const filledHeader = Math.round(safeSoc / 100 * headerSegments);
    const socColor = this.colorForSoc(safeSoc);
    const cells = (Array.isArray(this.config.cells) ? this.config.cells : [])
      .map((cell, index) => typeof cell === "string" ? { entity: cell, name: `C${index + 1}` } : { name: `C${index + 1}`, ...cell })
      .filter((cell) => cell.entity);
    const voltages = cells.map((cell) => this.value(cell.entity));
    const valid = voltages.filter((value) => value !== null);
    const minimum = valid.length ? Math.min(...valid) : null;
    const maximum = valid.length ? Math.max(...valid) : null;
    const delta = minimum === null ? null : maximum - minimum;
    const minimumIndex = valid.length ? voltages.findIndex((value) => value === minimum) : -1;
    const maximumIndex = valid.length ? voltages.findIndex((value) => value === maximum) : -1;
    const segments = Math.max(3, Math.min(12, Number(this.config.segments)));
    const rangeMin = Number(this.config.cell_voltage_min ?? 2.5);
    const rangeMax = Number(this.config.cell_voltage_max ?? 3.65);
    const cellMarkup = cells.map((cell, index) => {
      const voltage = voltages[index];
      const isMin = index === minimumIndex;
      const isMax = index === maximumIndex;
      const color = isMax ? this.config.max_color : isMin ? this.config.min_color : this.config.normal_color;
      const ratio = voltage === null ? 0 : Math.max(0, Math.min(1, (voltage - rangeMin) / (rangeMax - rangeMin)));
      const filled = Math.round(ratio * segments);
      const balancing = cell.balancing_entity ? ["on", "true", "1"].includes(this.state(cell.balancing_entity)) : false;
      return `<button class="cell" data-entity="${this.escape(cell.entity)}" style="--cell:${color}">
        <div class="cell-head"><span class="cell-value"><strong>${this.escape(cell.name)}</strong><span class="voltage">${voltage === null ? "—" : voltage.toFixed(4)} <small>V</small></span></span><span class="led ${balancing ? "on" : "off"}" title="Balansare ${balancing ? "activă" : "oprită"}"></span></div>
        <div class="segments">${Array.from({length: segments}, (_, i) => `<i class="${i < filled ? "filled" : ""}"></i>`).join("")}</div>
      </button>`;
    }).join("");
    const metric = (label, entity, digits = 1, unit = "") => {
      if (!entity) return ""; const value = this.value(entity);
      return `<button class="metric" data-entity="${this.escape(entity)}"><span>${this.escape(label)}</span><b>${value === null ? "—" : value.toFixed(digits)} ${this.escape(unit)}</b></button>`;
    };
    this.shadowRoot.innerHTML = `<style>
      :host{display:block}*{box-sizing:border-box}button{font:inherit;color:inherit}ha-card{overflow:hidden;padding:14px;background:var(--ha-card-background,var(--card-background-color));color:var(--primary-text-color);border-radius:var(--ha-card-border-radius,12px)}.top{display:grid;grid-template-columns:minmax(190px,1fr) auto;gap:14px;align-items:center}.title{margin:0 0 6px;font-size:18px;font-weight:500}.battery{position:relative;display:grid;grid-template-columns:repeat(${headerSegments},1fr);gap:3px;padding:5px;border:1px solid var(--divider-color);border-radius:8px;background:var(--secondary-background-color)}.battery:after{content:"";position:absolute;right:-6px;top:32%;width:5px;height:36%;border-radius:0 3px 3px 0;background:var(--divider-color)}.battery i,.segments i{display:block;border-radius:2px;background:${this.config.empty_color};box-shadow:inset 0 0 0 1px #ffffff10}.battery i{height:28px}.battery i.filled{background:${socColor};box-shadow:0 0 5px ${socColor}55}.soc{text-align:right;font-size:26px;font-weight:600;color:${socColor}}.soc small{display:block;color:var(--secondary-text-color);font-size:10px;font-weight:400}.metrics{display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin:10px 0}.metric,.cell{border:1px solid var(--divider-color);background:var(--secondary-background-color);border-radius:8px;cursor:pointer}.metric{padding:6px 8px;text-align:left}.metric span{display:block;color:var(--secondary-text-color);font-size:10px}.metric b{font-size:13px;font-weight:500}.grid{display:grid;grid-template-columns:repeat(${Math.max(1, Math.min(8, Number(this.config.columns)))},minmax(0,1fr));gap:5px}.empty-state{grid-column:1/-1;padding:16px;text-align:center;color:var(--secondary-text-color)}.cell{min-width:0;padding:6px 7px;text-align:left}.cell-head{display:flex;align-items:center;justify-content:space-between;gap:5px}.cell-value{display:flex;align-items:baseline;gap:6px;white-space:nowrap}.cell-head strong{font-size:10px;font-weight:500;color:var(--secondary-text-color)}.led{flex:0 0 auto;width:8px;height:8px;border-radius:50%}.led.on{background:${this.config.balance_on_color};box-shadow:0 0 6px ${this.config.balance_on_color}}.led.off{background:${this.config.balance_off_color}}.segments{display:grid;grid-template-columns:repeat(${segments},1fr);gap:2px;margin-top:5px}.segments i{height:6px}.segments i.filled{background:var(--cell);box-shadow:0 0 3px color-mix(in srgb,var(--cell),transparent 55%)}.voltage{font-size:11px;font-weight:500;font-variant-numeric:tabular-nums}.voltage small{font-size:8px;color:var(--secondary-text-color)}.legend{display:flex;flex-wrap:wrap;gap:10px;margin-top:8px;color:var(--secondary-text-color);font-size:9px}.legend i{width:7px;height:7px;border-radius:50%;display:inline-block;margin-right:3px}@media(max-width:600px){ha-card{padding:10px}.top{grid-template-columns:1fr}.soc{text-align:left}.metrics{grid-template-columns:repeat(2,1fr)}.grid{grid-template-columns:repeat(2,minmax(0,1fr))}}
      .battery{border:2px solid var(--secondary-text-color,#8b9690);box-shadow:inset 0 0 0 1px #ffffff12}
      .battery:after{right:-7px;top:30%;width:6px;height:40%;background:var(--secondary-text-color,#8b9690)}
      .battery i,.segments i{border:1px solid #ffffff28;box-shadow:inset 0 0 0 1px #00000020}
      .battery i.filled,.segments i.filled{border-color:transparent}
    </style><ha-card><div class="top"><div><h2 class="title">${this.escape(this.config.title)}</h2><div class="battery">${Array.from({length:headerSegments},(_,i)=>`<i class="${i<filledHeader?"filled":""}"></i>`).join("")}</div></div><div class="soc">${soc === null ? "—" : Math.round(soc)+"%"}<small>Stare de încărcare</small></div></div>
    <div class="metrics">${metric("Tensiune",this.config.voltage_entity,2,"V")}${metric("Curent",this.config.current_entity,2,"A")}${metric("Putere",this.config.power_entity,0,"W")}${metric("Diferență celule",this.config.delta_entity,4,"V") || `<div class="metric"><span>Diferență celule</span><b>${delta===null?"—":delta.toFixed(4)} V</b></div>`}</div>
    <div class="grid">${cellMarkup || '<div class="empty-state">Selectează entitățile celulelor din editorul vizual al cardului.</div>'}</div><div class="legend"><span><i style="background:${this.config.min_color}"></i>Minim</span><span><i style="background:${this.config.normal_color}"></i>Normal</span><span><i style="background:${this.config.max_color}"></i>Maxim</span><span><i style="background:${this.config.balance_on_color}"></i>Balansare activă</span></div></ha-card>`;
    this.shadowRoot.querySelectorAll("[data-entity]").forEach((item) => item.addEventListener("click", () => this.openMoreInfo(item.dataset.entity)));
  }
}

class InoVoltBatteryCardEditor extends HTMLElement {
  set hass(hass) {
    this._hass = hass;
    this.render();
  }

  setConfig(config) {
    this._config = { title: "Bateria 1", cells: [], ...config };
    if (!this.shadowRoot) this.attachShadow({ mode: "open" });
    this.render();
  }

  normalizeCells() {
    const configured = Array.isArray(this._config?.cells) ? this._config.cells : [];
    return Array.from({ length: 16 }, (_, index) => {
      const cell = configured[index];
      if (typeof cell === "string") return { name: `C${index + 1}`, entity: cell, balancing_entity: "" };
      return { name: `C${index + 1}`, entity: "", balancing_entity: "", ...(cell || {}) };
    });
  }

  changed(config) {
    this._config = config;
    this.dispatchEvent(new CustomEvent("config-changed", {
      bubbles: true,
      composed: true,
      detail: { config },
    }));
  }

  updateField(key, value) {
    this.changed({ ...this._config, [key]: value });
  }

  updateCell(index, key, value) {
    const cells = this.normalizeCells();
    cells[index] = { ...cells[index], [key]: value };
    this.changed({ ...this._config, cells });
  }

  addEntityPicker(container, label, value, domains, onChange) {
    const picker = document.createElement("ha-entity-picker");
    picker.hass = this._hass;
    picker.label = label;
    picker.value = value || "";
    picker.allowCustomEntity = true;
    picker.includeDomains = domains;
    picker.addEventListener("value-changed", (event) => onChange(event.detail.value || ""));
    container.appendChild(picker);
  }

  render() {
    if (!this._config || !this._hass || !this.shadowRoot) return;
    const cells = this.normalizeCells();
    this.shadowRoot.innerHTML = `<style>
      :host{display:block}.editor{display:grid;gap:16px}.section{display:grid;gap:10px;padding:14px;border:1px solid var(--divider-color);border-radius:12px}.section h3{margin:0;font-size:16px}.hint{margin:0;color:var(--secondary-text-color);font-size:13px}.main-grid,.cell-row{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.cell-row{align-items:end}.cell-name{font-weight:600;padding-bottom:12px}@media(max-width:700px){.main-grid,.cell-row{grid-template-columns:1fr}.cell-name{padding-bottom:0}}</style>
      <div class="editor">
        <div class="section"><h3>Card</h3><div class="main-grid" id="card-fields"></div></div>
        <div class="section"><h3>Entități principale</h3><div class="main-grid" id="main-entities"></div></div>
        <div class="section"><h3>Celule și balansare</h3><p class="hint">Alege senzorul de tensiune și, opțional, entitatea de balansare pentru fiecare celulă.</p><div id="cell-entities"></div></div>
      </div>`;

    const cardFields = this.shadowRoot.getElementById("card-fields");
    const title = document.createElement("ha-textfield");
    title.label = "Titlu";
    title.value = this._config.title || "";
    title.addEventListener("change", (event) => this.updateField("title", event.target.value));
    cardFields.appendChild(title);
    [
      ["columns", "Coloane", 1, 8, 4],
      ["segments", "Segmente celulă", 3, 12, 7],
      ["header_segments", "Segmente baterie", 4, 20, 10],
    ].forEach(([key, label, min, max, fallback]) => {
      const field = document.createElement("ha-textfield");
      field.type = "number";
      field.label = label;
      field.min = String(min);
      field.max = String(max);
      field.value = String(this._config[key] ?? fallback);
      field.addEventListener("change", (event) => this.updateField(key, Number(event.target.value)));
      cardFields.appendChild(field);
    });

    const mainEntities = this.shadowRoot.getElementById("main-entities");
    [
      ["soc_entity", "Stare de încărcare (SOC)"],
      ["voltage_entity", "Tensiune totală"],
      ["current_entity", "Curent"],
      ["power_entity", "Putere"],
      ["delta_entity", "Diferență între celule"],
    ].forEach(([key, label]) => this.addEntityPicker(mainEntities, label, this._config[key], ["sensor"], (value) => this.updateField(key, value)));

    const cellEntities = this.shadowRoot.getElementById("cell-entities");
    cells.forEach((cell, index) => {
      const row = document.createElement("div");
      row.className = "cell-row";
      const name = document.createElement("div");
      name.className = "cell-name";
      name.textContent = cell.name || `C${index + 1}`;
      row.appendChild(name);
      this.addEntityPicker(row, `Tensiune ${cell.name || `C${index + 1}`}`, cell.entity, ["sensor"], (value) => this.updateCell(index, "entity", value));
      this.addEntityPicker(row, `Balansare ${cell.name || `C${index + 1}`}`, cell.balancing_entity, ["binary_sensor", "switch"], (value) => this.updateCell(index, "balancing_entity", value));
      cellEntities.appendChild(row);
    });
  }
}

if (!customElements.get("inovolt-battery-card-editor")) customElements.define("inovolt-battery-card-editor", InoVoltBatteryCardEditor);
if (!customElements.get("inovolt-battery-card")) customElements.define("inovolt-battery-card", InoVoltBatteryCard);
window.customCards = window.customCards || [];
if (!window.customCards.some((card) => card.type === "inovolt-battery-card")) window.customCards.push({ type:"inovolt-battery-card", name:"InoVolt Battery Card", description:"SOC, electrical values, cell voltages and balancing status", preview:false });
