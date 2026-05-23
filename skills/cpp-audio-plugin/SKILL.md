# Skill: C++ Audio Plugin Build

## Entorno
- iPlug2OOS en /Users/awesomejohnny/Development/Plugins/iPlug2OOS/iPlug2
- macOS primary: VST3 + AU
- CMakePresets.json con preset macos-ninja

## Build commands
```bash
cmake --preset macos-ninja
cmake --build build --config Release
```

## Targets de deploy
VST3: ~/Library/Audio/Plug-Ins/VST3/
AU:   ~/Library/Audio/Plug-Ins/Components/

## Estructura de archivos — no modificar sin razón
Config.h → IDs únicos, dimensiones, channel IO
IndustrialIron.h/.cpp → clase principal únicamente
src/dsp/ → un archivo por módulo DSP
src/i18n/ → solo sistema de strings, nunca lógica
src/ui/ → controles visuales (Fase 5)

## PLUG_LATENCY
Actualizar cuando se agregue oversample en Fase 2a.
Fórmula: latency_samples = (oversample_factor - 1) * buffer_size / 2