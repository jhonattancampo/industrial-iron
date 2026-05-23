# SPEC — Industrial Iron v1.0
> Guitar tone plugin — German Industrial Metal character  
> Framework: iPlug2OOS | Targets: VST3 / AU / CLAP | Platform: macOS (primary), Windows (secondary)

---

## 0. CONTEXTO DEL PROYECTO

Este documento es el spec maestro para OpenCode + SDD.  
Cada fase debe ser completada y aprobada antes de avanzar a la siguiente.  
El agente NO debe escribir código fuera del alcance de la fase activa.

**Repositorio:** `industrial-iron-plugin`  
**Lenguaje:** C++17  
**Framework:** iPlug2OOS (zlib license, sin splash screen, compatible closed-source)  
**Distribución:** Gumroad + itch.io — precio target $25 USD  

---

## 1. DESCRIPCIÓN DEL PRODUCTO

Plugin de guitarra que replica la cadena de ganancia, contundencia y double-panning característicos del metal industrial alemán de los años 90–2000s.

**Nombre público:** Industrial Iron  
**Tagline:** "One knob from the wall of sound."  
**Sin referencias a bandas, artistas o nombres protegidos en ningún archivo del proyecto.**

### Qué hace

- Saturación/distorsión extrema con clipping asimétrico y oversampling
- EQ sculpt tipo amp (pre-distorsión): corte de sub-bass, boost low-mid, corte mid, boost presencia
- Compresión serial post-distorsión (squash tipo "pared")
- Double panning engine interno: genera spread L/R desde señal mono
- Cab simulation (modo MVP: filtro IIR modelado, sin convolución)
- 3 presets hardcoded: Rhythm Heavy / Lead Cut / DI Clean

---

## 2. ARQUITECTURA DE LA CADENA DSP

```
[Input] → [Input Gain] → [Pre-EQ] → [Asymmetric Hard Clipper x4 OS] → [Post-EQ] → [Compressor] → [Cab Sim] → [Iron Wall Panner] → [Output Gain] → [L/R Output]
```

### 2.1 Módulos (orden de procesamiento)

| # | Módulo | Descripción |
|---|--------|-------------|
| 1 | InputGain | Gain knob -20 a +40 dB |
| 2 | PreEQ | Tone stack fijo tipo amp (ver parámetros) |
| 3 | AsymClipper | Hard clipper asimétrico con x4 oversample |
| 4 | PostCompressor | Compresión serial, ratio 8:1 fijo |
| 5 | CabSim | Filtro IIR que modela respuesta 4x12 |
| 6 | IronWallPanner | Double panning con delay + detune en canal R |
| 7 | OutputGain | Trim de salida -20 a +6 dB |

---

## 3. PARÁMETROS EXPUESTOS (UI + Automation)

| ID | Nombre | Rango | Default | Descripción |
|----|--------|-------|---------|-------------|
| 0 | kGain | 0.0 – 1.0 | 0.7 | Input gain (mapea -20 a +40 dB) |
| 1 | kTone | 0.0 – 1.0 | 0.5 | Pre-EQ tone (bass vs presence balance) |
| 2 | kTightness | 0.0 – 1.0 | 0.6 | Threshold del compresor post-distorsión |
| 3 | kWidth | 0.0 – 1.0 | 0.8 | Iron Wall spread (0=mono, 1=full L/R) |
| 4 | kCabOn | 0 / 1 | 1 | Cab sim toggle |
| 5 | kOutput | 0.0 – 1.0 | 0.5 | Output trim |
| 6 | kPreset | 0 / 1 / 2 | 0 | Preset selector (ver sección 5) |

---

## 4. ESPECIFICACIÓN DSP DETALLADA

### 4.1 PreEQ (Tone Stack)

Filtros biquad en serie. Los valores son fijos (no paramétricos), solo `kTone` afecta el balance relativo bass/presence.

| Banda | Tipo | Frecuencia | Q | Gain |
|-------|------|-----------|---|------|
| Sub cut | High-pass | 80 Hz | 0.7 | — |
| Low-mid boost | Peak | 280 Hz | 1.2 | +5 dB (modulado por kTone: rango +2 a +8 dB) |
| Mid scoop | Peak | 1000 Hz | 1.5 | -4 dB (fijo) |
| Presence | Peak | 3500 Hz | 1.0 | +3 dB (modulado por kTone inverso: rango +1 a +5 dB) |

### 4.2 AsymClipper

- Oversampling: **x4 mínimo obligatorio** (usar IPlugUtilities o implementar filtro polyphase simple)
- Umbral positivo: `0.70`
- Umbral negativo: `0.85` (asimetría intencional — más "rectificado" en semiciclo negativo)
- Función de clipping: hard clip (no soft/tanh) — intencionalmente agresivo
- Post-clipper: gain de compensación para normalizar nivel percibido

```cpp
// Pseudocódigo de referencia
float clip(float x) {
    if (x > 0.70f)  return 0.70f;
    if (x < -0.85f) return -0.85f;
    return x;
}
```

### 4.3 PostCompressor

- Ratio: 8:1 (fijo, no expuesto)
- Attack: 5 ms (fijo)
- Release: 80 ms (fijo)
- Threshold: controlado por `kTightness` — mapea 0.0–1.0 a -40dBFS a -6dBFS
- Makeup gain: automático (unity gain tracking)
- Implementación: feed-forward RMS detector

### 4.4 CabSim (MVP — IIR modelado)

Respuesta aproximada de gabinete 4x12 usando filtros biquad en cascada:

| Etapa | Tipo | Frecuencia | Q | Gain |
|-------|------|-----------|---|------|
| Low-end body | Low-shelf | 120 Hz | — | +3 dB |
| Box resonance | Peak | 400 Hz | 2.0 | +2 dB |
| High cut (mic rolloff) | Low-pass | 5500 Hz | 0.7 | — |
| Air cut | Low-pass | 8000 Hz | 1.0 | — |

Toggle `kCabOn`: cuando OFF, bypasea este módulo completamente.

### 4.5 IronWallPanner

Procesa señal mono → salida stereo L/R.

- **Canal L:** señal sin modificar
- **Canal R:** 
  - Delay: 7 ms (buffer circular fijo)
  - Detune: +2 cents (pitch shifter de muy baja latencia — puede implementarse con interpolación lineal de buffer circular)
- **Width control:** crossfade entre mono (width=0) y full spread (width=1)
  - `L_out = signal`
  - `R_out = lerp(signal, R_processed, kWidth)`
- **Correlación:** calcular y exponer como valor interno para el meter de UI

---

## 5. PRESETS HARDCODED

| # | Nombre | kGain | kTone | kTightness | kWidth | kCabOn |
|---|--------|-------|-------|-----------|--------|--------|
| 0 | Rhythm Heavy | 0.80 | 0.45 | 0.70 | 0.90 | 1 |
| 1 | Lead Cut | 0.65 | 0.65 | 0.50 | 0.60 | 1 |
| 2 | DI Clean | 0.30 | 0.50 | 0.20 | 0.00 | 0 |

---

## 6. SISTEMA I18N

**Patrón obligatorio desde el inicio — no agregar strings hardcoded en ningún archivo.**

### 6.1 Estructura

```
src/
  i18n/
    StringKeys.h       → enum class StringKey { ... }
    Strings_EN.h       → constexpr array inglés
    Strings_ES.h       → constexpr array español (voseo en warnings)
    LocalizedStrings.h → singleton + macro LS(key)
```

### 6.2 Keys requeridas para v1

```cpp
enum class StringKey {
    // UI Labels
    kPluginName,
    kGainLabel,
    kToneLabel,
    kTightnessLabel,
    kWidthLabel,
    kOutputLabel,
    kCabLabel,
    kPresetLabel,
    
    // Preset names
    kPresetRhythmHeavy,
    kPresetLeadCut,
    kPresetDIClean,
    
    // Warnings / Meters
    kWarningPhaseCancel,     // "Phase cancellation detected — reduce Width"
    kWarningClipHot,         // "Input too hot — reduce Gain"
    kMeterCorrelation,
    
    kCOUNT
};
```

---

## 7. UI SPEC

### 7.1 Layout (800 x 300 px)

```
┌─────────────────────────────────────────────────────────┐
│  INDUSTRIAL IRON                    [Preset ▼]   v1.0   │
├──────────┬──────────┬──────────┬──────────┬─────────────┤
│  GAIN    │  TONE    │TIGHTNESS │  WIDTH   │   OUTPUT    │
│  [knob]  │  [knob]  │  [knob]  │  [knob]  │   [knob]   │
├──────────┴──────────┴──────────┴──────────┴─────────────┤
│  [CAB ON/OFF]          [Correlation meter: L ←——→ R]    │
│  [Warning text area — solo visible si hay warning]       │
└─────────────────────────────────────────────────────────┘
```

### 7.2 Visual identity

- Fondo: `#0A0A0A` (negro casi puro)
- Accent: `#CC2200` (rojo industrial)
- Knobs: metálico oscuro con indicador rojo
- Tipografía: monospace o industrial (sin serifa)
- Sin logos, sin referencias a bandas

---

## 8. ESTRUCTURA DE ARCHIVOS DEL PROYECTO

```
industrial-iron/
├── SPEC.md                    ← este archivo
├── Config.h                   ← nombre, versión, IDs (único-plugin IDs)
├── IndustrialIron.h/.cpp      ← clase principal (hereda IPlug)
├── src/
│   ├── dsp/
│   │   ├── PreEQ.h/.cpp
│   │   ├── AsymClipper.h/.cpp
│   │   ├── PostCompressor.h/.cpp
│   │   ├── CabSim.h/.cpp
│   │   └── IronWallPanner.h/.cpp
│   ├── i18n/
│   │   ├── StringKeys.h
│   │   ├── Strings_EN.h
│   │   ├── Strings_ES.h
│   │   └── LocalizedStrings.h
│   └── ui/
│       └── IronUI.h/.cpp
├── resources/
│   └── (sin IRs en MVP)
└── scripts/
    ├── build_mac.sh
    └── build_win.bat
```

---

## 9. FASES DE IMPLEMENTACIÓN (SDD — 9 fases)

El agente debe seguir este orden estrictamente. **No avanzar sin aprobación explícita.**

### FASE 1 — Scaffolding
**Objetivo:** proyecto compila y abre en el host sin DSP.  
**Entregables:**
- Config.h con IDs únicos generados
- IndustrialIron.h/.cpp con parámetros registrados (sin procesamiento)
- Sistema i18n completo (todas las keys de sección 6)
- UI vacía con layout estructurado (knobs en posición, sin gráficos finales)
- Compila en VST3 macOS sin warnings críticos

**Criterio de aceptación:** el plugin abre en Ableton, muestra los controles, no procesa audio (pass-through).

---

### FASE 2a — AsymClipper + Oversampling
**Objetivo:** distorsión funcional con oversample x4.  
**Entregables:**
- AsymClipper.h/.cpp implementado
- Test unitario simple: input sinusoidal → verificar que no supera umbrales + ausencia de aliasing evidente
- Integrado en cadena (Input → Clipper → Output)

**Criterio de aceptación:** señal distorsionada audible, sin aliasing tipo "digital crunch" a frecuencias agudas anómalas.

---

### FASE 2b — PreEQ + PostCompressor
**Objetivo:** cadena de ganancia completa.  
**Entregables:**
- PreEQ.h/.cpp con 4 filtros biquad (ver sección 4.1)
- PostCompressor.h/.cpp con RMS feed-forward
- kTone y kTightness funcionales

**Criterio de aceptación:** con kTone en 0 → más "gordo"; con kTone en 1 → más "afilado". Compresor squashea el transiente post-clipper.

---

### FASE 3 — IronWallPanner
**Objetivo:** double panning generado internamente.  
**Entregables:**
- IronWallPanner.h/.cpp con buffer de delay + detune
- kWidth funcional
- Cálculo de correlación L/R expuesto como valor interno
- Warning `kWarningPhaseCancel` activado si correlación < 0.5

**Criterio de aceptación:** en mono (kWidth=0) la señal L == R. En kWidth=1 hay spread audible. Sumado a mono no hay cancelación severa.

---

### FASE 4 — CabSim
**Objetivo:** cab sim IIR operativo.  
**Entregables:**
- CabSim.h/.cpp con 4 filtros biquad en cascada (ver sección 4.4)
- Toggle kCabOn hace bypass limpio (sin click)
- Respuesta en frecuencia medible: rolloff visible por encima de 5.5kHz

**Criterio de aceptación:** con CabOn=1 la guitarra suena "encuadrada" (menos high-end crudo). Bypass sin artefactos.

---

### FASE 5 — UI Final
**Objetivo:** UI visual completa según spec sección 7.  
**Entregables:**
- Knobs con skin industrial (color scheme #0A0A0A / #CC2200)
- Meter de correlación L/R visible
- Preset selector funcional (carga presets de sección 5)
- Área de warning visible solo cuando hay warning activo

**Criterio de aceptación:** el plugin se ve "de producto", no de prototipo.

---

### FASE 6 — QA + Build Scripts
**Objetivo:** producto entregable.  
**Entregables:**
- build_mac.sh que produce .vst3 y .component (AU) en `/dist/`
- build_win.bat que produce .vst3 en `/dist/`
- Checklist de QA completado (ver sección 10)

---

## 10. CHECKLIST DE QA (manual — no automatizable)

- [ ] Abre en Ableton Live sin crash
- [ ] Abre en un segundo host (Reaper o Logic)
- [ ] No clickea al mover knobs con audio corriendo
- [ ] CPU < 5% a buffer de 256 samples, 44.1kHz
- [ ] Silencio cuando no hay señal de entrada (no genera ruido)
- [ ] Sumado a mono con kWidth=1 → sin cancelación severa
- [ ] Los 3 presets suenan coherentes
- [ ] Warning de phase cancel se activa con kWidth > 0.9

---

## 11. REGLAS PARA EL AGENTE

1. **No mencionar bandas, artistas o nombres protegidos** en ningún comentario, string, o nombre de variable
2. **Respetar el orden de fases** — no implementar módulos de fases posteriores anticipadamente
3. **El oversample en AsymClipper es obligatorio** — no se acepta implementación sin él
4. **Todos los strings visibles al usuario pasan por el sistema i18n** — cero hardcoded strings en UI
5. **Antes de cada fase**, el agente debe mostrar un plan de implementación y esperar aprobación
6. **Si un módulo requiere una decisión de diseño no especificada** → preguntar antes de implementar
7. **Los coeficientes de filtros biquad** deben calcularse en tiempo de inicialización / sample rate change, no hardcodeados como números mágicos

---

## 12. PROMPT DE ARRANQUE PARA OPENCODE

Copiar esto en OpenCode para iniciar la sesión de Fase 1:

```
Leé el archivo SPEC.md en su totalidad antes de hacer cualquier cosa.

Sos un desarrollador C++17 especializado en desarrollo de plugins de audio con iPlug2OOS.

Tu tarea es implementar la FASE 1 — Scaffolding del plugin "Industrial Iron" según el SPEC.

Antes de escribir una sola línea de código, presentá un plan detallado de lo que vas a crear, 
en qué orden, y qué decisiones técnicas tomás donde el SPEC no es explícito.

Esperá mi aprobación del plan antes de comenzar la implementación.

Criterio de éxito de esta fase: el plugin compila como VST3 en macOS, 
abre en Ableton, muestra los 5 knobs + toggle + preset selector, 
y hace pass-through de audio sin procesamiento.
```