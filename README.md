# FreOSC - Software Synthesizer

A powerful web-based polyphonic synthesizer with multiple oscillators, advanced filtering, effects processing, and real-time modulation.

## Features

### 🎹 **Sound Generation**
- **3 Main Oscillators**: Sine, Square, Sawtooth, Triangle waveforms
- **Noise Generator**: White, Pink, Brown, Blue, Violet, Grey, Vinyl Crackle, Digital, Wind, Ocean
- **Octave Control**: -2 to +2 octaves per oscillator  
- **Fine Detuning**: Precise pitch control in cents (-50 to +50¢)
- **Stereo Panning**: Individual L/R positioning for each oscillator
- **Level Control**: Independent volume for each sound source

### 🔊 **Advanced Filtering**
- **9 Filter Types**: Low Pass, High Pass, Band Pass, Low Shelf, High Shelf, Peaking, Notch, All Pass, **Formant**
- **Formant Vowels**: A, E, I, O, U, AE, AW, ER for realistic vocal synthesis
- **Cutoff Range**: 100Hz to 8kHz with exponential response
- **Resonance**: 0.1 to 30 for everything from subtle to self-oscillating
- **Filter Gain**: ±20dB for shelf and peaking filters

### 🎚️ **Effects & Processing**
- **ADSR Envelope**: Attack (0-2s), Decay (0-2s), Sustain (0-1), Release (0-3s)
- **FM Synthesis**: Configurable source/target routing with ratio control (0.1-8x)
- **Dynamics Processing**: 
  - Compressor with threshold, ratio, attack, and release controls
  - Limiter with threshold and release for preventing clipping
- **Convolution Reverb**: Room size and wet level control with artificial impulse responses
- **Digital Delay**: Time (0-1000ms), Feedback (0-95%), Wet/Dry mix (0-100%)

### 🌊 **LFO & Modulation**
- **5 LFO Waveforms**: Sine, Triangle, Sawtooth, Square, Random (Sample & Hold)
- **4 Modulation Targets**: 
  - Pitch (vibrato effects)
  - Filter Cutoff (wobble bass effects)
  - Volume (tremolo effects) 
  - Pan (auto-pan effects)
- **Rate Control**: 0.01 to 20 Hz for slow sweeps to audio-rate modulation
- **Amount Control**: 0-100% modulation depth

### 🎛️ **Comprehensive Preset System**

#### **Classic Sounds:**
- **Classic Lead**: Bright sawtooth lead with filter resonance
- **Warm Pad**: Detuned triangles with pink noise and long reverb
- **Vintage Bass**: Fat sawtooth and square bass with brown noise
- **Bell Pad**: Harmonic sine waves with long decay

#### **Modern Textures:**
- **FM Electric**: FM synthesis with bandpass filtering
- **Analog Strings**: Detuned sawtoths with high shelf filtering
- **Retro Pluck**: Square waves with peaking filter and short decay
- **Dreamy Choir**: Soft triangles with formant filtering and heavy reverb

#### **Experimental & Ambient:**
- **Punchy Saw**: Aggressive sawtooth with highpass and heavy compression
- **Ambient Texture**: Slow attack pads with notch filtering and FM
- **Vinyl Lo-Fi**: Vintage sound with crackle noise and low shelf
- **Digital Glitch**: Harsh digital textures with digital noise
- **Ocean Pad**: Atmospheric pad with ocean wave sounds
- **Wind Ambient**: Evolving textures with wind noise and slow FM

#### **Special Vocal & Modulated:**
- **Dalai Lama Chant**: Formant-filtered "O" vowel with pitch LFO vibrato
- **Wobble Bass**: Heavy bass with sine LFO modulating filter cutoff
- **Tremolo Strings**: String ensemble with square LFO volume modulation

### 🎮 **Intuitive Controls**

#### **Mouse Interface**
- Click virtual keyboard keys to play notes
- Real-time parameter adjustment with sliders and dropdowns

#### **Computer Keyboard Mapping**
```
Lower Octave:  Z X C V B N M  (+ S D G H J for sharps/flats)
Main Octave:   Q W E R T Y U  (+ 2 3 5 6 7 for sharps/flats)  
Upper Octave:  I O P          (+ 9 0 for sharps/flats)
```

## Getting Started

### **Quick Setup**
1. **Open `index.html`** in any modern web browser
2. **Click anywhere** to enable audio (Web Audio API requirement)
3. **Select a preset** from the dropdown or create your own sound
4. **Play notes** using mouse clicks or computer keyboard
5. **Experiment** with different controls and effects

### **Sound Design Tips**
- Start with a preset and modify it to learn the controls
- Use **Formant filtering** with vowel selection for vocal-like sounds
- Combine **multiple oscillators** at different octaves for rich textures
- Add **LFO modulation** to bring static sounds to life
- Use **delay and reverb** together for spacious ambient sounds
- **FM synthesis** works great with sine waves for bell-like tones

## Technical Specifications

### **Audio Engine**
- **Web Audio API**: Real-time synthesis and processing
- **Polyphonic**: Play multiple notes simultaneously with full voice allocation
- **Sample Rate**: Adaptive to system (typically 44.1kHz or 48kHz)
- **Bit Depth**: 32-bit floating point processing
- **Latency**: Optimized for real-time performance

### **Architecture**
- **Signal Chain**: Oscillators → Mixer → Filter → Compressor → Limiter → Reverb → Delay → Output
- **No Dependencies**: Pure HTML5/CSS3/JavaScript implementation
- **Game-Style Input**: 120fps polling system for ultra-responsive keyboard control
- **Professional DSP**: Formant filtering, convolution processing, dynamics control

### **Performance Features**
- **Efficient Voice Management**: Automatic cleanup of finished notes
- **Smart Resource Usage**: Oscillators only created when needed (level > 0)
- **Optimized Rendering**: Minimal DOM updates during real-time performance
- **Memory Management**: Proper cleanup of audio nodes and timeouts

## Browser Compatibility

**Fully Supported:**
- Chrome/Chromium 66+
- Firefox 60+
- Safari 14+
- Edge 79+

**Requirements:**
- Web Audio API support
- ES6 JavaScript features
- Modern CSS Grid support

## Advanced Features

### **Synthesis Techniques**
- **Subtractive Synthesis**: Traditional oscillator → filter → amplifier chain
- **FM Synthesis**: Frequency modulation between oscillators
- **Additive Elements**: Multiple sine wave harmonics in bell sounds  
- **Granular Textures**: Various noise types for texture and character
- **Formant Synthesis**: Vocal tract modeling using parallel bandpass filters

### **Audio Processing**
- **Multi-stage Dynamics**: Compression followed by limiting for professional sound
- **Convolution Reverb**: Realistic room acoustics using generated impulse responses
- **Digital Delay**: Clean delay line with feedback control and filtering
- **Advanced Filtering**: Beyond basic filters to formant and multi-mode processing

### **Real-time Control**
- **MIDI-like Responsiveness**: Sub-10ms input latency through optimized polling
- **Parameter Automation**: Smooth parameter changes with Web Audio scheduling
- **Voice Stealing Prevention**: Proper note management prevents audio glitches
- **Performance Optimization**: Efficient CPU usage for complex patches

---

**🎵 Explore the sonic possibilities of FreOSC!**

*Built with passion for electronic music and synthesis. No installation required – just open and play!*