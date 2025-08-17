# FreOSC VST Development Setup Guide

This guide will help you set up the complete development environment for building the FreOSC VST plugin.

## Step 1: Install Prerequisites

### Windows Development Setup

1. **Install Visual Studio 2022 Community** (Free)
   - Download: https://visualstudio.microsoft.com/vs/community/
   - Workloads to install:
     - "Desktop development with C++"
     - "Windows 10/11 SDK" (latest version)

2. **Install Git** (if not already installed)
   - Download: https://git-scm.com/download/win
   - Choose "Git from the command line and also from 3rd-party software"

3. **Install CMake** (Optional, but recommended)
   - Download: https://cmake.org/download/
   - Add CMake to system PATH during installation

### macOS Development Setup

1. **Install Xcode** (Free from App Store)
   - Open Xcode and accept license agreements
   - Install command line tools: `xcode-select --install`

2. **Install Homebrew** (Package manager)
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

3. **Install dependencies**:
   ```bash
   brew install cmake git
   ```

### Linux Development Setup (Ubuntu/Debian)

1. **Install development tools**:
   ```bash
   sudo apt update
   sudo apt install build-essential cmake git
   sudo apt install libasound2-dev libjack-jackd2-dev
   sudo apt install libcurl4-openssl-dev libfreetype6-dev
   sudo apt install libx11-dev libxcomposite-dev libxcursor-dev
   sudo apt install libxext-dev libxinerama-dev libxrandr-dev libxrender-dev
   sudo apt install libwebkit2gtk-4.0-dev libglu1-mesa-dev
   ```

## Step 2: Download and Setup JUCE

### Method A: Download JUCE Release (Recommended for beginners)

1. **Download JUCE**:
   - Go to: https://juce.com/get-juce
   - Download the latest version (7.0+)
   - Extract to a permanent location (e.g., `C:\JUCE` or `/usr/local/JUCE`)

2. **Set environment variable**:
   - **Windows**: Add `JUCE_DIR=C:\JUCE` to system environment variables
   - **macOS/Linux**: Add `export JUCE_DIR=/usr/local/JUCE` to `.bashrc` or `.zshrc`

### Method B: Clone JUCE from GitHub (For advanced users)

```bash
# Clone JUCE repository
git clone https://github.com/juce-framework/JUCE.git
cd JUCE

# Checkout stable release (optional)
git checkout 7.0.8

# Set JUCE_DIR to this location
export JUCE_DIR=$(pwd)
```

## Step 3: Build Projucer (Optional but helpful)

Projucer is JUCE's project management tool:

### Windows:
```cmd
cd %JUCE_DIR%\extras\Projucer\Builds\VisualStudio2022
# Open Projucer.sln in Visual Studio and build
```

### macOS:
```bash
cd $JUCE_DIR/extras/Projucer/Builds/MacOSX
xcodebuild -configuration Release
```

### Linux:
```bash
cd $JUCE_DIR/extras/Projucer/Builds/LinuxMakefile
make CONFIG=Release
```

## Step 4: Clone FreOSC VST Project

```bash
# Clone the FreOSC VST project
git clone [your-repository-url] FreOSC-VST
cd FreOSC-VST
```

## Step 5: Build FreOSC VST Plugin

### Method A: Using CMake (Cross-platform)

```bash
# Create build directory
mkdir build && cd build

# Configure project
cmake .. -DJUCE_DIR=$JUCE_DIR

# Build (adjust number based on your CPU cores)
cmake --build . --config Release --parallel 8

# Install to system plugin directories (optional)
cmake --install .
```

### Method B: Using Projucer

1. **Open Projucer** (built in Step 3)

2. **Create new Audio Plugin project**:
   - Project Type: "Audio Plug-in"
   - Plugin Name: "FreOSC"
   - Plugin Code: "Fosc" (must be unique 4-character code)
   - Company: "FreOSC"

3. **Configure Plugin Settings**:
   - Plugin is a Synth: ✅ Enabled
   - Plugin MIDI Input: ✅ Enabled
   - Plugin MIDI Output: ❌ Disabled
   - Plugin Editor: ✅ Enabled
   - Plugin formats: VST3, AU (macOS only), Standalone

4. **Add Source Files**:
   - Add all `.h` and `.cpp` files from the `Source/` directory
   - Organize into groups: DSP, Parameters, Presets, GUI

5. **Set Preprocessor Definitions**:
   ```
   JUCE_WEB_BROWSER=0
   JUCE_USE_CURL=0
   JUCE_VST3_CAN_REPLACE_VST2=0
   ```

6. **Generate and Build**:
   - Click "Save and Open in IDE"
   - Build Release configuration

## Step 6: Test the Plugin

### VST3 Plugin Locations

The built plugin will be copied to standard locations:

- **Windows**: `C:\Program Files\Common Files\VST3\FreOSC.vst3`
- **macOS**: `/Library/Audio/Plug-Ins/VST3/FreOSC.vst3`
- **Linux**: `~/.vst3/FreOSC.vst3`

### Test in DAW

1. **Open your DAW** (Logic Pro, Ableton Live, FL Studio, Reaper, etc.)
2. **Scan for new plugins** (usually in Preferences/Settings)
3. **Load FreOSC** as an instrument plugin
4. **Test basic functionality**:
   - Play MIDI notes
   - Adjust parameters
   - Load presets

### Standalone Application

The build also creates a standalone app for testing without a DAW:

- **Windows**: `FreOSC.exe`  
- **macOS**: `FreOSC.app`
- **Linux**: `FreOSC`

## Step 7: Development Workflow

### Code Organization

```
Source/
├── PluginProcessor.h/cpp        # Main plugin logic
├── PluginEditor.h/cpp          # GUI interface
├── DSP/                        # Audio processing classes
├── Parameters/                 # Parameter management
├── Presets/                   # Preset system
└── GUI/                       # Custom UI components
```

### Making Changes

1. **Edit source files** in your preferred IDE/editor
2. **Rebuild** using CMake or your IDE
3. **Test** in standalone app or DAW
4. **Debug** using IDE debugger attached to DAW process

### Common Issues and Solutions

#### Build Errors

1. **"JUCE not found"**: 
   - Verify JUCE_DIR environment variable
   - Check JUCE installation path

2. **"Missing dependencies"**:
   - Install platform-specific development libraries
   - Check CMake output for specific missing packages

3. **Compiler errors**:
   - Ensure C++17 compiler support
   - Check JUCE version compatibility (7.0+ required)

#### Runtime Issues

1. **Plugin not loading**:
   - Check plugin is in correct directory
   - Verify DAW architecture matches plugin (64-bit vs 32-bit)
   - Check DAW plugin scanner logs

2. **Audio glitches**:
   - Check buffer size settings in DAW
   - Test with different sample rates
   - Enable debug logging to identify issues

3. **Parameter automation not working**:
   - Verify parameter IDs are unique
   - Check parameter ranges and default values
   - Test with different DAWs

## Step 8: Packaging for Distribution

### Code Signing (Required for macOS, recommended for Windows)

1. **macOS**: Obtain Apple Developer certificate
2. **Windows**: Obtain code signing certificate from trusted CA

### Installer Creation

Use tools like:
- **Windows**: Inno Setup, NSIS, or WiX Toolset
- **macOS**: Packages.app or create DMG
- **Linux**: Create .deb/.rpm packages or AppImage

### Automated Builds

Set up GitHub Actions or similar CI/CD for automated building and testing across platforms.

---

## Next Steps

Once you have a working build environment:

1. **Complete the DSP implementation** (oscillators, filters, effects)
2. **Create the GUI** to match the original web interface
3. **Implement the preset system**
4. **Add comprehensive testing**
5. **Optimize for performance**
6. **Package for distribution**

The plugin architecture is now ready for development. Each component can be implemented incrementally and tested independently.