# Industrial Iron — Agent Instructions

## Proyecto
Plugin de guitarra VST3/AU/CLAP con iPlug2OOS.
Identidad: German Industrial Metal tone. Sin referencias a bandas reales.

## Fuente de verdad
Lee `Spec.md` completo antes de cualquier tarea. Es el contrato irrompible.

## Build
```bash
cmake --preset macos-ninja
cmake --build build --config Release
```
El .vst3 se despliega automáticamente en ~/Library/Audio/Plug-Ins/VST3/

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

## Branch creation rule
ALWAYS create new feature branches from `dev`, not from other 
feature branches. dev is the integration branch and always 
contains the latest stable state of all phases.

Before creating any branch:
  git checkout dev
  git pull origin dev
  git checkout -b feat/new-branch-name

Never branch from feat/* branches directly.

### Circular buffer positions — float precision trap
Never let write/read positions grow unbounded as int/float.
IEEE 754 floats represent integers exactly only up to 2²⁴ (16M).
At 44100 Hz that's ~380 seconds — after that, float precision 
degrades and circular buffer drift checks stop working.

Fix: always keep positions in [0, mBufSize) using modulo wrap.
Drift check: use fmod(write - read + bufSize, bufSize) 
instead of raw subtraction.