# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FreOSC-VST is a **general-purpose cross-platform VST3 synthesizer plugin** built with JUCE 7.0+. This is a complete conversion from the original web-based FreOSC synthesizer, now offering professional DAW integration with full parameter automation and a custom tabbed GUI.

**Important**: FreOSC-VST is designed as a **versatile general synthesizer** capable of creating a wide range of sounds - leads, pads, basses, plucks, ambient textures, etc. While it includes advanced formant filtering capabilities for vocal-style synthesis, this is just **one feature among many**. The synthesizer should maintain excellent performance and sound quality across all synthesis types.

**Current Status**: Fully functional general synthesizer with dual filter system, 2 modulation envelopes, enhanced FM synthesis with intuitive ratio display, 17 factory presets covering diverse synthesis styles, and flexible effects routing.

## Build Process

To build the VST plugin:

```bash
cd FreOSC-VST
msbuild FreOSC-VST.sln /p:Configuration=Release /p:Platform=x64
```

Alternative CMake build:
```bash
cd FreOSC-VST
mkdir build
cd build
cmake ..
cmake --build . --config Release --parallel 4
```

The built VST3 will be located in: `build/FreOSC-VST_artefacts/Release/VST3/FreOSC.vst3`

### Testing Commands
- **CMake Release Build**: `cd FreOSC-VST && mkdir -p build && cd build && cmake .. && cmake --build . --config Release`
- **CMake Debug Build**: `cd FreOSC-VST && mkdir -p build && cd build && cmake .. && cmake --build . --config Debug`
- **MSBuild Release**: `msbuild FreOSC-VST.sln /p:Configuration=Release /p:Platform=x64` (if .sln exists)
- **MSBuild Debug**: `msbuild FreOSC-VST.sln /p:Configuration=Debug /p:Platform=x64` (if .sln exists)

### Requirements
- JUCE Framework 7.0+ (installed at C:\JUCE)
- CMake 3.22+
- Visual Studio 2019+ (Windows) / Xcode (macOS) / GCC (Linux)

## Architecture

### Core Components

#### Audio Processing
- **PluginProcessor.cpp/.h**: Main audio processor managing synthesis chain and parameter state
- **FreOscVoice.cpp/.h**: Individual voice management with polyphonic synthesis
- **FreOscSound.h**: Sound class for JUCE synthesizer framework

#### DSP Modules
- **FreOscOscillator**: Individual oscillator with waveform, octave, level, detune, pan
- **FreOscFilter**: Multi-mode filter with lowpass/highpass/bandpass/notch/formant types
- **FreOscLFO**: Low-frequency oscillator with pitch/filter/volume/pan modulation
- **FreOscReverb**: Algorithmic reverb using 4-delay-line architecture
- **FreOscDelay**: Digital delay with feedback and wet/dry mixing
- **FreOscNoise**: Multiple noise types (white, pink, brown, etc.)
- **FreOscCompressor/Limiter**: Dynamics processing

#### GUI System
- **PluginEditor.cpp/.h**: Tabbed GUI interface with 5 organized tabs
- **FreOscPresets**: Preset management system with factory presets

#### Parameter System
- **FreOscParameters.h**: Centralized parameter definitions and ranges
- **AudioProcessorValueTreeState**: JUCE parameter system with automation

### Audio Engine Structure

**Per-Voice Signal Flow**: 
```
Oscillators (1-3) + Noise → FM Synthesis → Voice Mixing → 
LFO Modulation (Pitch/Volume/Pan) → ADSR Envelope → Voice Output
```

**Global Effects Chain**: 
```
Summed Voices → Compressor → Limiter → Filter → Reverb → Delay → Master Output
```

**Architecture Rationale**: This is a **classic synthesizer architecture** where:
- **Per-Voice Processing**: Each voice generates oscillators, applies FM/LFO modulation, and envelope shaping
- **Global Effects**: Master effects (filter, reverb, delay) process the summed voice output
- **LFO Filter Modulation**: Applied globally to the master filter for classic "wobble" effects
- **Formant Mode**: The global filter switches to formant mode when selected, providing vocal character to any sound source

### Key Features

#### Oscillators
- **3 Independent Oscillators**: Sine, Square, Sawtooth, Triangle waveforms
- **Octave Control**: -2 to +2 octave range per oscillator
- **Fine Detuning**: ±50 cents per oscillator
- **Individual Panning**: Stereo positioning per oscillator
- **FM Synthesis**: Cross-modulation between oscillators

#### Modulation & Polyphony
- **16-Voice Polyphony**: Full polyphonic synthesis with proper voice stealing
- **LFO**: 5 waveforms (sine, triangle, sawtooth, square, random)
- **LFO Targets**: Pitch, Filter Cutoff, Filter2 Cutoff, Volume, Pan
- **Main Envelope**: ADSR with 0.01ms to 5s timing ranges for amplitude control
- **Modulation Envelopes**: 2 independent ADSR envelopes for parameter modulation
  - **ModEnv Targets**: FM Amount, FM Ratio, Filter Cutoff, Filter2 Cutoff
  - **ModEnv Modes**: Gate (standard), One-Shot (complete cycle), Looping (continuous cycle)
  - **Per-Voice Processing**: Each voice has independent modulation envelope instances
  - **Real-time Control**: 0-100% modulation amount with selectable targets and modes

#### Filter System
- **9 Filter Types**: Lowpass, Highpass, Bandpass, Notch, Peaking, Shelving, Allpass, **Formant**
- **Advanced Formant Filtering**: Professional vocal synthesis with 3-stage series processing
  - **8 Vowel Shapes**: A(ah), E(eh), I(ee), O(oh), U(oo), AE(ay), AW(aw), ER(ur)
  - **Formant Frequencies**: Accurate vocal tract modeling (F1: 300-730Hz, F2: 850-2300Hz, F3: 1690-3200Hz)
  - **Peaking Filter Implementation**: Series cascade of 3 peaking filters for realistic vocal formants
  - **Optimized for Vocal Synthesis**: Works best with rich harmonic sources (sawtooth oscillators)
- **Resonance**: 0.1 to 30.0 Q factor range
- **Filter Gain**: ±24dB for peaking/shelving filters

#### FM Synthesis
- **Classic FM Implementation**: Phase modulation using Oscillator 3 as fixed modulator source
- **Intuitive Controls**: 
  - **FM Amount**: 0-100% modulation depth (improved from 0-1000 range)
  - **FM Ratio**: Displays as proper ratios (1:1, 3:2, 2:1, etc.) for musical context
- **Flexible Routing**: Modulate Osc1, Osc2, or both from Osc3
- **Modulation Envelope Control**: Both modulation envelopes can target FM parameters
- **Musical Ratios**: Common harmonic ratios (1:2, 3:2, 2:1, 5:3, etc.) for bell, brass, and complex timbres

#### Effects
- **Dynamics**: Compressor + Limiter with full parameter control
- **Reverb**: **Optimized Algorithmic Processing**
  - **Stable Feedback**: Conservative feedback levels (0.12-0.3) prevent instability and crackling
  - **Clean Processing**: 4-delay-line architecture with input/output clamping
  - **Vocal-Friendly**: Reduced wet levels and moderate room sizes preserve vocal clarity
- **Delay**: Up to 3 seconds with feedback and wet level control

#### GUI Interface
- **5 Organized Tabs**:
  1. **Oscillators**: All 3 oscillators + noise generator + main envelope
  2. **Filter & Envelope**: Dual filter system + routing controls  
  3. **Modulation**: LFO + FM synthesis + 2 modulation envelopes with Target/Mode selection (2x2 grid layout)
  4. **Effects**: Plate Reverb + Tape Delay with flexible routing
  5. **Master**: Volume + preset management
- **Resizable Interface**: 50%-150% scaling with maintained aspect ratio
- **Dark Theme**: Professional appearance with blue accents
- **Real-time Parameter Display**: Live value updates for all controls

#### Preset System
- **17 Factory Presets**: Includes signature **"Dalai Lama Chant"** vocal synthesis preset
  - **Classic Lead**: Bright sawtooth lead with high-pass filtering
  - **Warm Pad**: Lush detuned triangles with sine sub
  - **FM Electric**: Clean DX7-style electric piano
  - **Dalai Lama Chant**: Advanced vocal synthesis using formant filtering
  - **And 13 More**: Complete range from basses to ambient textures
- **Clean Parameter Setup**: All presets use normalized values (0.0-1.0) without mathematical conversions
- **Choice Parameter Compatibility**: Raw index values for AudioParameterChoice parameters
- **Automatic Loading**: Dropdown selector with instant parameter updates

## Development Notes

### Parameter System
- All parameters use JUCE's AudioProcessorValueTreeState
- Normalized values (0.0-1.0) are converted to actual ranges
- Parameter attachments provide automatic GUI ↔ processor synchronization
- Host automation is fully supported
- **Critical**: AudioParameterChoice parameters use raw indices (0.0, 1.0, 2.0) not normalized values (0.0, 0.33, 0.67)

### Voice Management  
- 16-voice polyphony using JUCE Synthesizer framework
- Each voice has independent oscillators, envelope, and modulation
- Proper note stealing and voice allocation
- Amplitude scaling prevents clipping with multiple voices

### Audio Threading
- Real-time audio processing in dedicated thread
- Parameter updates are atomic and thread-safe
- No allocations in audio callback
- Proper sample rate and buffer size handling

### Cross-Platform Compatibility
- JUCE framework handles platform differences
- CMake build system for all platforms
- VST3 format works in all major DAWs

### Advanced Features

#### Formant Synthesis Implementation
The formant filter system provides professional-grade vocal synthesis:

**Mathematical Approach**:
- **Series Processing**: Three peaking filters in cascade (not parallel) - F1 → F2 → F3
- **Peaking Filters**: Uses `makePeakFilter()` instead of bandpass - preserves more signal
- **Formant Frequencies**: Based on acoustic research (F1: 730Hz, F2: 1090Hz, F3: 2440Hz for "A" vowel)
- **Bandwidth Control**: Moderate Q values (5-12) with bandwidths of 50-100Hz
- **Gain Staging**: 15-35dB peaks at formant frequencies for strong vocal character

**Vocal Synthesis Best Practices**:
- **Rich Harmonic Source**: Use multiple sawtooth oscillators for maximum harmonic content
- **Proper Oscillator Levels**: High levels (0.9, 0.6, 0.4) provide strong formant filtering material
- **Sub-harmonic Support**: -1 octave oscillator adds vocal body and fullness
- **Breath Simulation**: Light pink noise (0.08 level) simulates natural breath sounds

#### Reverb Optimization
The reverb system has been optimized for stability and vocal clarity:

**Stability Improvements**:
- **Conservative Feedback**: Reduced from 0.35-0.7 to 0.12-0.3 to prevent runaway feedback
- **Input/Output Clamping**: Hard limiting at ±1.0 prevents digital clipping
- **Reduced Complexity**: 4 delay lines instead of 8 reduces potential for phase issues
- **Feedback Scaling**: Room size maps to 0.2-0.5 feedback range (was 0.5-0.9)

**Vocal-Friendly Processing**:
- **Lower Wet Levels**: Reduced wet signal gain preserves vocal formant clarity
- **Simplified Damping**: Basic high-frequency roll-off instead of complex IIR filtering
- **Moderate Preset Settings**: Dalai Lama preset uses 0.5 room size, 0.2 wet level

## Testing

### Audio Engine Testing
1. **Oscillator Testing**: Verify each waveform produces expected output
2. **Filter Testing**: Test all filter types with various cutoff/resonance settings
3. **Modulation Testing**: Verify LFO affects targeted parameters correctly
4. **Effects Testing**: Confirm reverb, delay, and dynamics processing work
5. **Polyphony Testing**: Play multiple notes simultaneously
6. **Parameter Automation**: Test DAW automation of all parameters

### GUI Testing  
1. **Tab Switching**: Verify all tabs display correctly
2. **Parameter Control**: Test all sliders, combo boxes update audio
3. **Preset Loading**: Verify presets load all parameters correctly
4. **Resizing**: Test interface scaling at different sizes
5. **Real-time Updates**: Confirm parameter displays update during playback

### Build Testing
1. **Release Build**: Test optimized release configuration
2. **VST3 Validation**: Use VST3 plugin validator if available
3. **DAW Integration**: Test in multiple DAW environments
4. **Performance**: Monitor CPU usage and audio dropouts

## Common Issues

### Build Issues
- **JUCE Path**: Ensure JUCE is installed at C:\JUCE (Windows)
- **CMake Version**: Requires CMake 3.22 or newer
- **Compiler**: Use supported compiler versions
- **MSBuild**: Use `msbuild FreOSC-VST.sln /p:Configuration=Release /p:Platform=x64` for fastest builds

### Audio Issues  
- **Filter Stereo**: Uses ProcessorDuplicator for proper stereo processing
- **Amplitude Scaling**: Voices scaled to prevent clipping
- **Parameter Ranges**: All parameters properly clamped to valid ranges

### GUI Issues
- **Tab Layout**: Custom tab components handle layout internally
- **Parameter Attachments**: Ensure all controls have parameter attachments
- **Threading**: GUI updates happen on message thread only

### Recent Fixes Applied

#### Modulation Envelope Modes (Latest)
- **UI Implementation**: Added Mode dropdown controls next to Target dropdowns in Mod Env 1/2
- **Parameter Mapping**: Fixed combo box indexing for proper 0-based parameter alignment
- **Mode Functionality**: Implemented Gate/One-Shot/Looping envelope behaviors in voice processing
- **Layout**: Target and Mode controls positioned side-by-side in modulation envelope sections

#### Formant Filter Issues
- **No Audio Through Filter**: Fixed by changing from parallel bandpass to series peaking filters
- **Too Quiet**: Increased formant gains to 15-35dB and added +9dB mixer gain
- **Not Vocal Enough**: Enhanced with proper formant frequencies and bandwidth control
- **Crackling/Distortion**: Added input/output clamping and conservative gain staging

#### Reverb Issues  
- **Crackling Sound**: Fixed by reducing feedback levels and adding signal clamping
- **Muddy Output**: Simplified processing and reduced wet levels for vocal clarity
- **Instability**: Conservative feedback scaling and reduced complexity (4 delay lines)

#### Preset Issues
- **Mathematical Complexity**: Converted all presets to use clean normalized values (0.0-1.0)
- **Choice Parameter Errors**: Fixed AudioParameterChoice to use raw indices instead of normalized values
- **Inconsistent Loading**: Standardized parameter mapping across all 17 factory presets

## File Structure

```
FreOSC-VST/
├── Source/
│   ├── PluginProcessor.h/cpp        # Main audio processor
│   ├── PluginEditor.h/cpp          # Tabbed GUI interface  
│   ├── DSP/                        # Audio processing modules
│   │   ├── FreOscVoice.h/cpp       # Voice management
│   │   ├── FreOscOscillator.h/cpp  # Oscillator implementation
│   │   ├── FreOscFilter.h/cpp      # Multi-mode filter
│   │   ├── FreOscLFO.h/cpp         # LFO modulation
│   │   ├── FreOscReverb.h/cpp      # Reverb effect
│   │   └── ...                     # Other DSP modules
│   ├── Parameters/
│   │   └── FreOscParameters.h      # Parameter definitions
│   └── Presets/
│       ├── FreOscPresets.h/cpp     # Preset management
├── CMakeLists.txt                  # Build configuration
└── build/                          # Build output directory
    └── FreOSC-VST_artefacts/Release/VST3/FreOSC.vst3  # Final VST3 plugin
```