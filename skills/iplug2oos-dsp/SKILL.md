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