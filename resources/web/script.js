// Industrial Iron — WebView UI script
// iPlug2 WebView bridge + IronKnob canvas control
//
// Parameter indices (must match EParams in IndustrialIron.h):
//   0  kGain
//   1  kTone
//   2  kTightness
//   3  kWidth
//   4  kCabOn
//   5  kParamOutput
//   6  kPreset
//   7  kPan
//
// Message tags (must match EMsgTags in IndustrialIron.h):
//   0  kMsgTagCorrelation

'use strict';

// ── Constants ────────────────────────────────────────────────

const K_GAIN        = 0;
const K_TONE        = 1;
const K_TIGHTNESS   = 2;
const K_WIDTH       = 3;
const K_CAB_ON      = 4;
const K_OUTPUT      = 5;
const K_PRESET      = 6;
const K_PAN         = 7; // pending C++ implementation

const MSG_TAG_CORRELATION = 0; // arbitrary msg tag for correlation data

// ── Global state ─────────────────────────────────────────────

const gKnobs = {};   // paramIdx → IronKnob
let   gCabOn = true; // mirrors kCabOn default (true per Spec §3)
let   gCorrelationSmoothed = 0.5; // EMA-smoothed correlation for jitter reduction

// ============================================================
// IronKnob — Canvas-based knob control
// ============================================================

class IronKnob {
  // Arc geometry: 135° start, 270° clockwise sweep
  static START_DEG = 135;
  static SWEEP_DEG = 270;
  static toRad = deg => deg * Math.PI / 180;

  constructor(canvas) {
    this.canvas      = canvas;
    this.ctx         = canvas.getContext('2d');
    this.paramIdx    = parseInt(canvas.dataset.param, 10);
    this.value       = parseFloat(canvas.dataset.default ?? '0.5');
    this.dragging    = false;
    this.dragStartY  = 0;
    this.dragStartVal = 0;

    canvas.addEventListener('mousedown',  this._onMouseDown.bind(this));
    canvas.addEventListener('dblclick',   this._onDblClick.bind(this));
    canvas.addEventListener('wheel',      this._onWheel.bind(this), { passive: false });

    this.draw();
  }

  // Set value (0.0–1.0) and redraw. Does NOT send to C++.
  setValue(v) {
    this.value = Math.max(0.0, Math.min(1.0, v));
    this.draw();
  }

  draw() {
    const { ctx, canvas, value } = this;
    const W  = canvas.width;   // 80
    const H  = canvas.height;  // 90
    const cx = W / 2;          // 40
    const cy = 40;             // knob center (leaves 50px below for value text)
    const R  = 30;             // arc radius

    ctx.clearRect(0, 0, W, H);

    const startAngle = IronKnob.toRad(IronKnob.START_DEG);
    const endAngle   = IronKnob.toRad(IronKnob.START_DEG + IronKnob.SWEEP_DEG);
    const valAngle   = IronKnob.toRad(IronKnob.START_DEG + value * IronKnob.SWEEP_DEG);

    // ── Background arc (full range) ──────────────────────
    ctx.beginPath();
    ctx.arc(cx, cy, R, startAngle, endAngle);
    ctx.strokeStyle = '#1C1C1C';
    ctx.lineWidth   = 3.5;
    ctx.lineCap     = 'round';
    ctx.stroke();

    // ── Value arc (0 → current value) ───────────────────
    if (value > 0.001) {
      ctx.beginPath();
      ctx.arc(cx, cy, R, startAngle, valAngle);
      ctx.strokeStyle = '#CC2200';
      ctx.lineWidth   = 3.5;
      ctx.lineCap     = 'round';
      ctx.stroke();
    }

    // ── Inner circle ─────────────────────────────────────
    ctx.beginPath();
    ctx.arc(cx, cy, 20, 0, Math.PI * 2);
    ctx.fillStyle = '#141414';
    ctx.fill();

    // Inner circle ring (subtle depth)
    ctx.beginPath();
    ctx.arc(cx, cy, 20, 0, Math.PI * 2);
    ctx.strokeStyle = '#252525';
    ctx.lineWidth   = 1;
    ctx.stroke();

    // ── Indicator line ────────────────────────────────────
    const cosA = Math.cos(valAngle);
    const sinA = Math.sin(valAngle);
    ctx.beginPath();
    ctx.moveTo(cx + 13 * cosA, cy + 13 * sinA);
    ctx.lineTo(cx + 26 * cosA, cy + 26 * sinA);
    ctx.strokeStyle = '#CC2200';
    ctx.lineWidth   = 2;
    ctx.lineCap     = 'round';
    ctx.stroke();

    // ── Center dot ───────────────────────────────────────
    ctx.beginPath();
    ctx.arc(cx, cy, 2.5, 0, Math.PI * 2);
    ctx.fillStyle = '#CC2200';
    ctx.fill();

    // ── Value text (3-decimal, below arc) ────────────────
    ctx.font         = '9px "Courier New", Courier, monospace';
    ctx.fillStyle    = '#505050';
    ctx.textAlign    = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(value.toFixed(3), cx, cy + R + 11);
  }

  // ── Interaction handlers ─────────────────────────────────

  _onMouseDown(e) {
    e.preventDefault();
    this.dragging    = true;
    this.dragStartY  = e.clientY;
    this.dragStartVal = this.value;

    BPCFUI(this.paramIdx);

    const onMove = (ev) => {
      if (!this.dragging) return;
      // 200px vertical travel = full range
      const delta  = (this.dragStartY - ev.clientY) / 200;
      const newVal = Math.max(0.0, Math.min(1.0, this.dragStartVal + delta));
      this.setValue(newVal);
      SPVFUI(this.paramIdx, this.value);
    };

    const onUp = () => {
      this.dragging = false;
      EPCFUI(this.paramIdx);
      document.removeEventListener('mousemove', onMove);
      document.removeEventListener('mouseup',   onUp);
    };

    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup',   onUp);
  }

  _onDblClick(e) {
    e.preventDefault();
    // Reset to factory default on double-click
    const def = parseFloat(this.canvas.dataset.default ?? '0.5');
    this.setValue(def);
    SPVFUI(this.paramIdx, this.value);
  }

  _onWheel(e) {
    e.preventDefault();
    // Mouse wheel: fine-tune (0.5% per tick)
    const delta  = -e.deltaY * 0.0025;
    const newVal = Math.max(0.0, Math.min(1.0, this.value + delta));
    this.setValue(newVal);
    SPVFUI(this.paramIdx, this.value);
  }
}

// ============================================================
// Correlation meter
// ============================================================

function drawCorrelationMeter(value) {
  // value: 0.0 (anti-phase / uncorrelated) → 1.0 (fully correlated)
  const canvas = document.getElementById('correlation-canvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const W = canvas.width;  // 440
  const H = canvas.height; // 18

  ctx.clearRect(0, 0, W, H);

  // Track background
  ctx.fillStyle = '#1A1A1A';
  _fillRoundRect(ctx, 0, 2, W, H - 4, 2);
  ctx.fill();

  // Zone tint: left half red (warning zone), right half neutral
  const zoneGrad = ctx.createLinearGradient(0, 0, W, 0);
  zoneGrad.addColorStop(0.0,  'rgba(180, 28, 0, 0.18)');
  zoneGrad.addColorStop(0.45, 'rgba(120, 28, 0, 0.06)');
  zoneGrad.addColorStop(0.55, 'rgba(40, 80, 20, 0.06)');
  zoneGrad.addColorStop(1.0,  'rgba(30, 80, 10, 0.14)');
  ctx.fillStyle = zoneGrad;
  _fillRoundRect(ctx, 0, 2, W, H - 4, 2);
  ctx.fill();

  // Center reference tick (∅ — null correlation boundary)
  ctx.beginPath();
  ctx.moveTo(W / 2, 2);
  ctx.lineTo(W / 2, H - 2);
  ctx.strokeStyle = '#2C2C2C';
  ctx.lineWidth   = 1;
  ctx.stroke();

  // Needle
  const clampedVal = Math.max(0.0, Math.min(1.0, value));
  const needleX    = clampedVal * W;
  const isWarning  = clampedVal < 0.5;

  // Bloom glow pass (drawn first, wider + blurred)
  ctx.save();
  ctx.shadowColor = isWarning ? 'rgba(204, 34, 0, 0.6)' : 'rgba(119, 119, 119, 0.3)';
  ctx.shadowBlur  = 8;
  ctx.beginPath();
  ctx.moveTo(needleX, 1);
  ctx.lineTo(needleX, H - 1);
  ctx.strokeStyle = isWarning ? '#CC2200' : '#777777';
  ctx.lineWidth   = 2;
  ctx.stroke();
  // Outer glow halo
  ctx.shadowBlur  = 16;
  ctx.shadowColor = isWarning ? 'rgba(204, 34, 0, 0.25)' : 'rgba(119, 119, 119, 0.1)';
  ctx.stroke();
  ctx.restore();

  // Crisp core line on top
  ctx.beginPath();
  ctx.moveTo(needleX, 1);
  ctx.lineTo(needleX, H - 1);
  ctx.strokeStyle = isWarning ? '#CC2200' : '#777777';
  ctx.lineWidth   = 2;
  ctx.stroke();

  // Warning text visibility
  const warningEl = document.getElementById('warning-text');
  if (warningEl) {
    warningEl.style.display = isWarning ? 'block' : 'none';
  }
}

// ── roundRect polyfill ────────────────────────────────────────
function _fillRoundRect(ctx, x, y, w, h, r) {
  if (typeof ctx.roundRect === 'function') {
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, r);
  } else {
    ctx.beginPath();
    ctx.rect(x, y, w, h);
  }
}

// ============================================================
// CAB toggle
// ============================================================

function updateCabButton(on) {
  gCabOn = on;
  const btn = document.getElementById('cab-btn');
  if (!btn) return;
  btn.classList.toggle('on', on);
}

function toggleCab() {
  updateCabButton(!gCabOn);
  SPVFUI(K_CAB_ON, gCabOn ? 1.0 : 0.0);
}

// ============================================================
// Preset buttons
// ============================================================

function updatePresetButtons(idx) {
  document.querySelectorAll('.preset-btn').forEach((btn, i) => {
    btn.classList.toggle('active', i === idx);
  });
}

function setupPresetButtons() {
  document.querySelectorAll('.preset-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const idx        = parseInt(btn.dataset.preset, 10);
      const normalized = idx / 2.0; // 0→0.0, 1→0.5, 2→1.0
      updatePresetButtons(idx);
      SPVFUI(K_PRESET, normalized);
    });
  });
}

// ============================================================
// Utility
// ============================================================

function _base64ToFloat32(b64) {
  // Decode a 4-byte little-endian IEEE 754 float sent as base64
  try {
    const bin   = atob(b64);
    const bytes = new Uint8Array(4);
    for (let i = 0; i < 4; i++) bytes[i] = bin.charCodeAt(i);
    return new Float32Array(bytes.buffer)[0];
  } catch (_) {
    return 0.0;
  }
}

// ============================================================
// iPlug2 bridge — FROM DELEGATE  (C++ → JS)
// These function names are called directly by the WebView
// bridge layer; do not rename them.
// ============================================================

function SPVFD(paramIdx, val) {
  // Parameter value pushed from C++ (host automation, preset load, etc.)
  OnParamChange(paramIdx, val);
}

function SCVFD(ctrlTag, val) {
  // Control value from delegate — not used by this plugin
  OnControlChange(ctrlTag, val);
}

function SCMFD(ctrlTag, msgTag, msg) {
  // Control message from delegate — not used
}

function SAMFD(msgTag, dataSize, msg) {
  // Arbitrary message from delegate (correlation data, etc.)
  OnMessage(msgTag, dataSize, msg);
}

function SMMFD(statusByte, dataByte1, dataByte2) {
  // MIDI message from delegate — not used
}

function SSMFD(offset, size, msg) {
  // Sysex from delegate — not used
}

// ============================================================
// iPlug2 bridge — TO DELEGATE  (JS → C++)
// ============================================================

function SPVFUI(paramIdx, value) {
  const message = {
    msg:      'SPVFUI',
    paramIdx: parseInt(paramIdx, 10),
    value:    value
  };
  if (typeof IPlugSendMsg === 'function') IPlugSendMsg(message);
}

function BPCFUI(paramIdx) {
  const message = {
    msg:      'BPCFUI',
    paramIdx: parseInt(paramIdx, 10)
  };
  if (typeof IPlugSendMsg === 'function') IPlugSendMsg(message);
}

function EPCFUI(paramIdx) {
  const message = {
    msg:      'EPCFUI',
    paramIdx: parseInt(paramIdx, 10)
  };
  if (typeof IPlugSendMsg === 'function') IPlugSendMsg(message);
}

function SAMFUI(msgTag, ctrlTag = -1, data = 0) {
  const message = {
    msg:     'SAMFUI',
    msgTag:  msgTag,
    ctrlTag: ctrlTag,
    data:    data
  };
  if (typeof IPlugSendMsg === 'function') IPlugSendMsg(message);
}

// ============================================================
// Application-level handlers
// ============================================================

function OnParamChange(paramIdx, val) {
  // Route parameter update to the correct control
  if (gKnobs[paramIdx] !== undefined) {
    gKnobs[paramIdx].setValue(val);
    return;
  }
  switch (paramIdx) {
    case K_CAB_ON:
      updateCabButton(val >= 0.5);
      break;
    case K_PRESET:
      updatePresetButtons(Math.round(val * 2));
      break;
  }
}

function OnControlChange(ctrlTag, val) {
  // Not used by this plugin
}

function OnMessage(msgTag, dataSize, msg) {
  if (msgTag === MSG_TAG_CORRELATION) {
    const raw = _base64ToFloat32(msg);
    gCorrelationSmoothed = gCorrelationSmoothed * 0.85 + raw * 0.15;
    drawCorrelationMeter(gCorrelationSmoothed);
  }
  // msgTag === -1: iPlug2 sends param definitions on page load.
  // We hardcode defaults in HTML, so this is intentionally ignored.
}

// ============================================================
// Init
// ============================================================

document.addEventListener('DOMContentLoaded', () => {
  // Build knob instances for every canvas in the knobs row
  document.querySelectorAll('.knob-canvas').forEach(canvas => {
    const knob = new IronKnob(canvas);
    gKnobs[knob.paramIdx] = knob;
  });

  // Initial meter draw — center needle (0.5) for a clear first impression
  drawCorrelationMeter(0.5);

  // Preset button click handlers
  setupPresetButtons();

  // CAB button click handler
  document.getElementById('cab-btn').addEventListener('click', toggleCab);
});
