# STM32F407VG Discovery Midi Synth

Extension of the https://github.com/rogerallen/stm32disc_synth1 project 
adding USB MIDI keyboard input.

## Goals

- use STM32CubeIDE
- USB MIDI Keyboard Control
- using HAL & BSP_AUDIO Stm32 Firmware
- a "reasonable"-sounding synth
- polyphonic wavetable synth 
- resonant lowpass filter
- reverb
- user button to reset & reprogram synth
 
## Usage

- git clone this repo
- add repo to STM32CubeIDE
- compile & run
- plug in midi keyboard (via USB OTG adapter cable) to send note on, note off messages
- listen to sounds
- press user button to reset & reprogram synth

## Note

If you run in Debug mode, the full 10-voice synth will fail because it cannot keep up.  
Change the number of voices to 2 in synth.c.

Coded for clarity, not necessarily performance.

## Reprogram Synth

press user button, the orange LED will activate & this will cause it to output
```
begin edit mode
{
  wave      = 0
  voices    = 10
  attack    = 200
  decay     = 200
  sustain   = 800
  release   = 200
  scale     = 300
  cutoff    = 600
  resonance = 5
  wet       = 750
  delay     = 1500
}
Enter 'variable value' (e.g. 'attack 500').
End edit mode with '.' 
```

wave, voices, cutoff and resonance are unscaled but the rest of the values are scaled by 1000.
(scanf %f was giving me grief)

enter in settings and finish with a . 
```
wave 1
attack 800
cutoff 1000
.
```

## License

Who the heck knows?  I pulled stuff from everywhere to make this.

usbh_MIDI by Xavier Halgand (GPLv2) via https://github.com/MrBlueXav/Dekrispator_v2
biquad by Sean Connelly (MIT) via https://github.com/velipso/sndfilter
reverb written by me but inspired by https://github.com/YetAnotherElectronicsChannel/STM32_DSP_Reverb





