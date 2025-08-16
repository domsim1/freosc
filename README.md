# FreOSC - Software Synthesizer

A powerful web-based polyphonic synthesizer with multiple oscillators, advanced filtering, and effects processing.

## Features

### 🎹 **Oscillators**
- **3 Main Oscillators**: Sine, Square, Sawtooth, Triangle waveforms
- **Noise Generator**: White, Pink, Brown, Blue, Violet, Grey, Vinyl Crackle, Digital, Wind, Ocean
- **Octave Control**: -2 to +2 octaves per oscillator  
- **Detuning**: Fine pitch control in cents
- **Stereo Panning**: Individual L/R positioning

### 🔊 **Filtering**
- **9 Filter Types**: Low Pass, High Pass, Band Pass, Low Shelf, High Shelf, Peaking, Notch, All Pass, **Formant**
- **Formant Vowels**: A, E, I, O, U, AE, AW, ER for vocal synthesis
- **Cutoff & Resonance**: Full frequency range control
- **Filter Gain**: ±20dB for shelf and peaking filters

### 🎚️ **Effects**
- **ADSR Envelope**: Attack, Decay, Sustain, Release
- **FM Synthesis**: Configurable source/target routing with ratio control
- **Dynamics**: Compressor and Limiter with full parameter control
- **Reverb**: Room size and wet level control with convolution processing

### 🎛️ **Presets**
**Classic Sounds:**
- Classic Lead, Warm Pad, Vintage Bass, Bell Pad

**Modern Textures:**
- FM Electric, Analog Strings, Retro Pluck, Dreamy Choir

**Experimental:**
- Punchy Saw, Ambient Texture, Vinyl Lo-Fi, Digital Glitch
- Ocean Pad, Wind Ambient, **Dalai Lama Chant**

### 🎮 **Controls**

#### **Mouse**
- Click keyboard keys to play notes

#### **Computer Keyboard**
```
Lower Octave: Z X C V B N M (+ S D G H J for sharps/flats)
Main Octave:  Q W E R T Y U (+ 2 3 5 6 7 for sharps/flats)  
Upper Octave: I O P (+ 9 0 for sharps/flats)
```

## Getting Started

1. **Open `index.html`** in any modern web browser
2. **Select a preset** or create your own sound
3. **Play notes** using mouse or keyboard
4. **Experiment** with different filter types and effects

## Technical Details

- **Web Audio API**: Real-time synthesis and processing
- **Polyphonic**: Play multiple notes simultaneously  
- **No Dependencies**: Pure HTML/CSS/JavaScript
- **Game-Style Input**: 120fps polling for responsive keyboard control
- **Professional DSP**: Formant filtering, convolution reverb, dynamics processing

## Browser Compatibility

Works in all modern browsers supporting Web Audio API:
- Chrome/Edge 66+
- Firefox 60+
- Safari 14+

---

**🎵 Happy Synthesizing!**