# Industrial Iron — Agent Instructions

## Proyecto
Plugin de guitarra VST3/AU/CLAP con iPlug2OOS.
Identidad: German Industrial Metal tone. Sin referencias a bandas reales.

## Fuente de verdad
Lee `Spec.md` completo antes de cualquier tarea. Es el contrato irrompible.

## Build
```bash
cmake --preset macos-ninja
ninja -C build/macos-ninja
```
VST3 y AU se despliegan automáticamente.


## Convenciones obligatorias
- Todos los strings de UI pasan por LS() — nunca hardcoded
- Coeficientes biquad se calculan en OnReset(), nunca hardcodeados
- Oversample x4 obligatorio en AsymClipper
- No mencionar bandas/artistas en comentarios, variables, ni strings

## Skills disponibles
Antes de implementar DSP → lee `skills/iplug2oos-dsp/SKILL.md`
Antes de tocar CMake/build → lee `skills/cpp-audio-plugin/SKILL.md`
Antes de QA → lee `skills/audio-testing/SKILL.md`

## Fases activas
Ver sección 9 del Spec.md. No implementar código de fases futuras.
Siempre presentar plan y esperar aprobación antes de implementar.

## Phase completion rule
No phase is complete until a QA subagent runs the checklist
from skills/audio-testing/SKILL.md and reports results.
The orchestrator must launch this subagent explicitly at the
end of every phase, before any git commits.