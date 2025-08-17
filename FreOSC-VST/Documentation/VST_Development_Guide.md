# FreOSC VST Plugin Development Guide

## Project Overview
Converting the FreOSC web synthesizer into a cross-platform VST3 plugin using the JUCE framework.

## Prerequisites

### Required Software
1. **JUCE Framework**
   - Download from: https://juce.com/get-juce
   - Version: 7.0+ recommended
   - License: GPL v3 (free for open source) or Commercial license

2. **Development Environment**
   - **Windows**: Visual Studio 2019/2022 (Community Edition is fine)
   - **macOS**: Xcode 12+
   - **Linux**: GCC 9+ or Clang 10+

3. **Build Tools**
   - CMake 3.22+ (if using CMake build)
   - Git (for version control and dependencies)

### Optional Tools
- **Projucer**: JUCE's project management tool (included with JUCE)
- **Plugin validation tools** for testing in different hosts

## Installation Steps

### 1. Install JUCE
```bash
# Clone JUCE repository
git clone https://github.com/juce-framework/JUCE.git
cd JUCE

# Build Projucer (optional but recommended)
cd extras/Projucer/Builds/VisualStudio2022  # Windows
# or
cd extras/Projucer/Builds/MacOSX            # macOS
# or  
cd extras/Projucer/Builds/LinuxMakefile     # Linux

# Open project and build
```

### 2. Create Plugin Project Structure
```
FreOSC-VST/
├── Source/
│   ├── DSP/                    # Audio processing classes
│   │   ├── FreOscOscillator.h/cpp
│   │   ├── FreOscFilter.h/cpp
│   │   ├── FreOscEnvelope.h/cpp
│   │   ├── FreOscLFO.h/cpp
│   │   ├── FreOscDelay.h/cpp
│   │   └── FreOscReverb.h/cpp
│   ├── GUI/                    # User interface
│   │   ├── FreOscEditor.h/cpp
│   │   ├── FreOscLookAndFeel.h/cpp
│   │   └── Components/
│   ├── Parameters/             # Plugin parameters
│   │   └── FreOscParameters.h/cpp
│   ├── Presets/               # Preset management
│   │   └── FreOscPresets.h/cpp
│   ├── PluginProcessor.h/cpp   # Main plugin class
│   └── PluginEditor.h/cpp      # Main editor class
├── Resources/                  # Assets and resources
│   ├── Presets/               # Factory presets
│   └── Graphics/              # GUI graphics
├── Builds/                     # Generated build files
└── FreOSC-VST.jucer          # Projucer project file
```

## Current Web Synthesizer Analysis

### Core Components to Port

#### 1. Oscillators (3x)
- **Waveforms**: Sine, Square, Sawtooth, Triangle
- **Controls**: Octave (-2 to +2), Level (0-1), Detune (-50 to +50 cents), Pan (-1 to +1)
- **Implementation**: Use JUCE's juce::dsp::Oscillator

#### 2. Noise Generator
- **Types**: White, Pink, Brown, Blue, Violet, Grey, Vinyl Crackle, Digital, Wind, Ocean
- **Controls**: Type selection, Level (0-1), Pan (-1 to +1)
- **Implementation**: Custom noise generators using juce::Random

#### 3. Envelope (ADSR)
- **Parameters**: Attack (0-2s), Decay (0-2s), Sustain (0-1), Release (0-3s)
- **Implementation**: juce::ADSR class

#### 4. Filter System
- **Types**: Lowpass, Highpass, Bandpass, Lowshelf, Highshelf, Peaking, Notch, Allpass, Formant
- **Controls**: Cutoff (100-8000 Hz), Resonance (0.1-30), Gain (-20 to +20 dB)
- **Formant**: Vowel selection (A, E, I, O, U, AE, AW, ER)
- **Implementation**: juce::dsp::IIR filters + custom formant filters

#### 5. Effects Chain
- **FM Synthesis**: Source/Target routing, Ratio (0.1-8), Amount (0-1000)
- **Compressor**: Threshold (-60 to 0 dB), Ratio (1-20), Attack/Release
- **Limiter**: Threshold (-12 to 0 dB), Release time
- **Reverb**: Room Size (0-1), Wet Level (0-1)
- **Delay**: Time (0-1000ms), Feedback (0-0.95), Wet Level (0-1)

#### 6. LFO System
- **Waveforms**: Sine, Triangle, Sawtooth, Square, Random
- **Targets**: Pitch, Filter Cutoff, Volume, Pan
- **Controls**: Rate (0.01-20 Hz), Amount (0-1)

#### 7. Presets (16 total)
- All current presets need to be converted to VST parameter states

## Parameter Mapping Strategy

### VST Parameter Organization
```cpp
enum class FreOscParameterIDs
{
    // Oscillator 1 (5 parameters)
    osc1Waveform = 0,
    osc1Octave,
    osc1Level, 
    osc1Detune,
    osc1Pan,
    
    // Oscillator 2 (5 parameters)
    osc2Waveform,
    // ... etc
    
    // Total: ~65 parameters
};
```

## Development Phases

### Phase 1: Basic Plugin Shell ✅ 
- [x] Create project structure
- [ ] Set up Projucer project
- [ ] Configure build targets (VST3, AU, Standalone)
- [ ] Create basic parameter layout

### Phase 2: Core DSP Engine
- [ ] Port oscillator system
- [ ] Implement envelope generator
- [ ] Create basic filtering
- [ ] Add polyphonic voice management

### Phase 3: Effects Chain  
- [ ] Port FM synthesis
- [ ] Implement dynamics processing
- [ ] Add reverb and delay effects
- [ ] Create LFO system

### Phase 4: GUI Development
- [ ] Design main interface layout
- [ ] Create custom components
- [ ] Implement preset browser
- [ ] Add virtual keyboard

### Phase 5: Advanced Features
- [ ] MIDI input support
- [ ] Tempo synchronization
- [ ] Additional modulation routing
- [ ] MPE support

## Testing Strategy

### Plugin Validation
- Test in multiple DAWs: Logic Pro, Ableton Live, FL Studio, Reaper, Cubase
- Validate parameter automation
- Check MIDI input functionality
- Stress test polyphony and CPU usage

### Cross-Platform Testing
- Build and test on Windows, macOS, and Linux
- Verify audio compatibility across sample rates
- Test GUI scaling on different screen resolutions

## Distribution Plan

### Build Automation
- Set up GitHub Actions for automated builds
- Configure code signing for macOS and Windows
- Create installer packages for each platform

### Licensing Considerations
- JUCE GPL v3: Plugin must be open source
- JUCE Commercial License: Allows proprietary distribution
- VST3 SDK: Free with Steinberg license agreement

## Next Steps

1. **Install JUCE and Visual Studio** (if on Windows)
2. **Create Projucer project** with basic plugin template
3. **Set up version control** with Git
4. **Begin DSP porting** starting with oscillators
5. **Implement parameter system** for real-time control

---

**Target Completion**: 3-4 months for full-featured VST3 plugin
**Complexity Level**: Intermediate to Advanced (C++, DSP, Audio Programming)