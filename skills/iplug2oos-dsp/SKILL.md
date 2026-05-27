# Skill: iPlug2OOS DSP

## Cuándo se usa
Implementación de módulos DSP en C++ con iPlug2OOS:
filtros biquad, oversampling, clipping, compresión, panning stereo.

## Patrones obligatorios

### Filtros biquad
Los coeficientes se calculan SIEMPRE en OnReset() o cuando cambia el sample rate.
Nunca en ProcessBlock(). Nunca como números mágicos.

```cpp
void OnReset() override {
    double sr = GetSampleRate();
    // calcular coeficientes acá
    mFilter.SetCoeffs(sr, freq, q, gainDb);
}
```

### Oversample x4
Usar IPlugUtilities o buffer circular manual con filtro anti-alias.
El oversample va ANTES del clipper y DESPUÉS se downsamplea.
Sin oversample = rechazo automático de la implementación.

### ProcessBlock
- Nunca allocar memoria en ProcessBlock
- Nunca hacer operaciones costosas en el hot path
- Parámetros suavizados con GetParam()->Value() + smoothing manual si hay clicks

### Bypass limpio
Toggle de módulos con crossfade de 64 samples mínimo para evitar clicks.

## Errores comunes a evitar
- Hardcodear sample rate (siempre GetSampleRate())
- Olvidar resetear estado de filtros en OnReset()
- No compensar latencia al agregar oversample (actualizar PLUG_LATENCY)


## Known iPlug2OOS quirks

### HIIR headers missing <cassert>
Any DSP module that includes `Extras/Oversampler.h` must add:
  #include <cassert>
before that include, or compilation fails as a separate translation unit.
Apply this to every new file in src/dsp/ that uses oversampling.

### Preset loading / forcing UI redraw
To push param values FROM delegate TO UI (preset load, state restore):
  - Set each param: GetParam(idx)->SetNormalized(value)
  - Then call: SendCurrentParamValuesFromDelegate()
  
DO NOT use SendParameterValueFromUI for this — it goes UI→Delegate 
(for automation recording only) and does NOT update knob visuals.

For preset selector UI: use IVRadioButtonControl, not IVButtonControl.
Toggle-off: SetValue(-1.0) → no button highlighted (index = -1).

### UI Control overrides (iPlug2OOS specific)
- Override `Draw(IGraphics& g)` NOT `OnDraw()` — OnDraw doesn't exist
- Override `OnGUIIdle()` NOT `OnIdle()` — for polling in custom controls
- `IText` constructed directly: `IText(size, color)` — no `.WithColor()` chaining

### USE_IDLE_CALLS no está definido en este proyecto
OnGUIIdle() NUNCA se llama — es código muerto gateado por #ifdef USE_IDLE_CALLS.
Para polling per-frame en custom controls, usar:
  SetAnimation([](IControl*) {});  // en el constructor
Esto hace que IsDirty() retorne true siempre → redraw cada frame.
El poll del valor va directamente en Draw(), no en OnGUIIdle().