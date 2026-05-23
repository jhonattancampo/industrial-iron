# Skill: Audio Plugin Testing

## Checklist obligatorio por fase
- [ ] Compila sin warnings críticos
- [ ] Abre en Ableton sin crash
- [ ] No clickea al mover knobs con audio corriendo
- [ ] CPU < 5% a 256 samples buffer, 44.1kHz
- [ ] Silencio cuando no hay señal de entrada

## Tests específicos por módulo

### AsymClipper
- Input sinusoidal 440Hz a 0dBFS → verificar clipping visible en waveform
- Espectro: no debe haber componentes fuertes por encima de 18kHz (aliasing)

### IronWallPanner
- kWidth=0 → correlación L/R = 1.0 (mono perfecto)
- kWidth=1 → correlación L/R > 0.5 (sin cancelación severa)
- Suma a mono → sin cancelación evidente

### CabSim
- Respuesta medible: rolloff visible por encima de 5.5kHz
- Toggle ON/OFF sin click