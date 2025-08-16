class FreOSC {
    constructor() {
        this.audioContext = null;
        this.masterGain = null;
        this.activeOscillators = new Map();
        this.filter = null;
        
        this.settings = {
            // Oscillator 1
            osc1Waveform: 'sine',
            osc1Octave: 0,
            osc1Level: 0.8,
            osc1Detune: 0,
            osc1Pan: 0,
            // Oscillator 2  
            osc2Waveform: 'sawtooth',
            osc2Octave: 0,
            osc2Level: 0.4,
            osc2Detune: -7,
            osc2Pan: -0.3,
            // Oscillator 3
            osc3Waveform: 'square',
            osc3Octave: -1,
            osc3Level: 0,
            osc3Detune: 12,
            osc3Pan: 0.3,
            // Noise
            noiseType: 'white',
            noiseLevel: 0,
            noisePan: 0,
            // Master
            volume: 0.3,
            // Envelope
            attack: 0.1,
            decay: 0.3,
            sustain: 0.6,
            release: 0.5,
            // Filter
            filterType: 'lowpass',
            cutoff: 2000,
            resonance: 1,
            filterGain: 0,
            formantVowel: 'a',
            // FM
            fmAmount: 0,
            fmSource: 'none',
            fmTarget: 'osc1',
            fmRatio: 1,
            // Dynamics
            compThreshold: -12,
            compRatio: 4,
            compAttack: 0.003,
            compRelease: 0.25,
            limThreshold: -3,
            limRelease: 0.01,
            // Reverb
            roomSize: 0.5,
            reverbWet: 0.2
        };
        
        this.presets = {
            'classic-lead': {
                osc1Waveform: 'sawtooth', osc1Octave: 0, osc1Level: 0.8, osc1Detune: 0, osc1Pan: 0,
                osc2Waveform: 'sawtooth', osc2Octave: 0, osc2Level: 0.6, osc2Detune: -7, osc2Pan: 0.2,
                osc3Waveform: 'sine', osc3Octave: 1, osc3Level: 0.3, osc3Detune: 0, osc3Pan: -0.2,
                noiseType: 'white', noiseLevel: 0, noisePan: 0,
                volume: 0.4, attack: 0.05, decay: 0.2, sustain: 0.7, release: 0.3,
                filterType: 'lowpass', cutoff: 3500, resonance: 8, filterGain: 0, formantVowel: 'a', fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -12, compRatio: 4, compAttack: 0.003, compRelease: 0.25,
                limThreshold: -3, limRelease: 0.01, roomSize: 0.3, reverbWet: 0.15
            },
            'warm-pad': {
                osc1Waveform: 'triangle', osc1Octave: 0, osc1Level: 0.7, osc1Detune: 0, osc1Pan: -0.3,
                osc2Waveform: 'triangle', osc2Octave: 0, osc2Level: 0.7, osc2Detune: 5, osc2Pan: 0.3,
                osc3Waveform: 'sine', osc3Octave: -1, osc3Level: 0.4, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'pink', noiseLevel: 0.02, noisePan: 0,
                volume: 0.3, attack: 0.8, decay: 0.5, sustain: 0.8, release: 1.2,
                filterType: 'lowpass', cutoff: 1500, resonance: 2, filterGain: 0, formantVowel: 'a', fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -18, compRatio: 3, compAttack: 0.01, compRelease: 0.3,
                limThreshold: -6, limRelease: 0.02, roomSize: 0.7, reverbWet: 0.35
            },
            'vintage-bass': {
                osc1Waveform: 'sawtooth', osc1Octave: -1, osc1Level: 1.0, osc1Detune: 0, osc1Pan: 0,
                osc2Waveform: 'square', osc2Octave: -1, osc2Level: 0.6, osc2Detune: -12, osc2Pan: 0,
                osc3Waveform: 'sine', osc3Octave: -2, osc3Level: 0.3, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'brown', noiseLevel: 0.05, noisePan: 0,
                volume: 0.5, attack: 0.01, decay: 0.4, sustain: 0.3, release: 0.2,
                filterType: 'lowpass', cutoff: 800, resonance: 12, filterGain: 0, formantVowel: 'a', fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -8, compRatio: 6, compAttack: 0.001, compRelease: 0.1,
                limThreshold: -2, limRelease: 0.005, roomSize: 0.1, reverbWet: 0.05
            },
            'bell-pad': {
                osc1Waveform: 'sine', osc1Octave: 0, osc1Level: 0.8, osc1Detune: 0, osc1Pan: 0,
                osc2Waveform: 'sine', osc2Octave: 1, osc2Level: 0.6, osc2Detune: 0, osc2Pan: -0.4,
                osc3Waveform: 'sine', osc3Octave: 2, osc3Level: 0.4, osc3Detune: 0, osc3Pan: 0.4,
                noiseType: 'white', noiseLevel: 0, noisePan: 0,
                volume: 0.35, attack: 0.3, decay: 0.8, sustain: 0.6, release: 2.0,
                filterType: 'lowpass', cutoff: 4000, resonance: 3, filterGain: 0, formantVowel: 'a', fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -15, compRatio: 3, compAttack: 0.005, compRelease: 0.4,
                limThreshold: -4, limRelease: 0.015, roomSize: 0.8, reverbWet: 0.4
            },
            'fm-electric': {
                osc1Waveform: 'sine', osc1Octave: 0, osc1Level: 0.9, osc1Detune: 0, osc1Pan: 0,
                osc2Waveform: 'sine', osc2Octave: 1, osc2Level: 0.5, osc2Detune: 0, osc2Pan: 0.3,
                osc3Waveform: 'sine', osc3Octave: 0, osc3Level: 0.3, osc3Detune: 7, osc3Pan: -0.3,
                noiseType: 'white', noiseLevel: 0, noisePan: 0,
                volume: 0.4, attack: 0.02, decay: 0.6, sustain: 0.4, release: 0.8,
                filterType: 'bandpass', cutoff: 3000, resonance: 4, filterGain: 0, formantVowel: 'a', fmAmount: 200, fmSource: 'osc2', fmTarget: 'osc1', fmRatio: 2.0,
                compThreshold: -10, compRatio: 5, compAttack: 0.002, compRelease: 0.15,
                limThreshold: -3, limRelease: 0.008, roomSize: 0.4, reverbWet: 0.25
            },
            'analog-strings': {
                osc1Waveform: 'sawtooth', osc1Octave: 0, osc1Level: 0.7, osc1Detune: -5, osc1Pan: -0.2,
                osc2Waveform: 'sawtooth', osc2Octave: 0, osc2Level: 0.7, osc2Detune: 5, osc2Pan: 0.2,
                osc3Waveform: 'square', osc3Octave: -1, osc3Level: 0.4, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'white', noiseLevel: 0, noisePan: 0,
                volume: 0.3, attack: 0.6, decay: 0.4, sustain: 0.9, release: 1.5,
                filterType: 'highshelf', cutoff: 2200, resonance: 6, filterGain: 3, fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -16, compRatio: 4, compAttack: 0.01, compRelease: 0.35,
                limThreshold: -5, limRelease: 0.02, roomSize: 0.6, reverbWet: 0.3
            },
            'retro-pluck': {
                osc1Waveform: 'square', osc1Octave: 0, osc1Level: 0.8, osc1Detune: 0, osc1Pan: 0,
                osc2Waveform: 'square', osc2Octave: 1, osc2Level: 0.4, osc2Detune: -12, osc2Pan: 0,
                osc3Waveform: 'triangle', osc3Octave: 0, osc3Level: 0.0, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'pink', noiseLevel: 0.08, noisePan: 0,
                volume: 0.45, attack: 0.01, decay: 0.3, sustain: 0.1, release: 0.4,
                filterType: 'peaking', cutoff: 4500, resonance: 15, filterGain: 8, fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -8, compRatio: 8, compAttack: 0.001, compRelease: 0.08,
                limThreshold: -2, limRelease: 0.003, roomSize: 0.2, reverbWet: 0.1
            },
            'dreamy-choir': {
                osc1Waveform: 'triangle', osc1Octave: 0, osc1Level: 0.6, osc1Detune: -3, osc1Pan: -0.4,
                osc2Waveform: 'triangle', osc2Octave: 0, osc2Level: 0.6, osc2Detune: 3, osc2Pan: 0.4,
                osc3Waveform: 'sine', osc3Octave: 1, osc3Level: 0.4, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'white', noiseLevel: 0, noisePan: 0,
                volume: 0.25, attack: 1.2, decay: 0.6, sustain: 0.8, release: 2.5,
                filterType: 'lowpass', cutoff: 1800, resonance: 1, filterGain: 0, formantVowel: 'a', fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -20, compRatio: 2, compAttack: 0.02, compRelease: 0.5,
                limThreshold: -8, limRelease: 0.03, roomSize: 0.9, reverbWet: 0.5
            },
            'punchy-saw': {
                osc1Waveform: 'sawtooth', osc1Octave: 0, osc1Level: 1.0, osc1Detune: 0, osc1Pan: 0,
                osc2Waveform: 'sawtooth', osc2Octave: 0, osc2Level: 0.8, osc2Detune: -7, osc2Pan: 0,
                osc3Waveform: 'square', osc3Octave: -1, osc3Level: 0.2, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'white', noiseLevel: 0, noisePan: 0,
                volume: 0.4, attack: 0.001, decay: 0.15, sustain: 0.6, release: 0.25,
                filterType: 'highpass', cutoff: 5000, resonance: 10, filterGain: 0, formantVowel: 'a', fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -6, compRatio: 10, compAttack: 0.0005, compRelease: 0.05,
                limThreshold: -1, limRelease: 0.002, roomSize: 0.1, reverbWet: 0.08
            },
            'ambient-texture': {
                osc1Waveform: 'triangle', osc1Octave: 0, osc1Level: 0.5, osc1Detune: -8, osc1Pan: -0.6,
                osc2Waveform: 'sine', osc2Octave: 1, osc2Level: 0.4, osc2Detune: 8, osc2Pan: 0.6,
                osc3Waveform: 'triangle', osc3Octave: -1, osc3Level: 0.6, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'pink', noiseLevel: 0.15, noisePan: 0,
                volume: 0.2, attack: 2.0, decay: 1.5, sustain: 0.9, release: 3.0,
                filterType: 'notch', cutoff: 1200, resonance: 3, filterGain: 0, formantVowel: 'a', fmAmount: 50, fmSource: 'osc3', fmTarget: 'all', fmRatio: 0.5,
                compThreshold: -22, compRatio: 2, compAttack: 0.05, compRelease: 0.8,
                limThreshold: -10, limRelease: 0.05, roomSize: 1.0, reverbWet: 0.6
            },
            'vinyl-lo-fi': {
                osc1Waveform: 'triangle', osc1Octave: 0, osc1Level: 0.7, osc1Detune: 0, osc1Pan: 0,
                osc2Waveform: 'sine', osc2Octave: 0, osc2Level: 0.5, osc2Detune: -3, osc2Pan: -0.2,
                osc3Waveform: 'sine', osc3Octave: 1, osc3Level: 0.3, osc3Detune: 3, osc3Pan: 0.2,
                noiseType: 'crackle', noiseLevel: 0.25, noisePan: 0,
                volume: 0.4, attack: 0.3, decay: 0.4, sustain: 0.7, release: 0.8,
                filterType: 'lowshelf', cutoff: 1800, resonance: 4, filterGain: -3, fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -15, compRatio: 6, compAttack: 0.01, compRelease: 0.2,
                limThreshold: -4, limRelease: 0.01, roomSize: 0.4, reverbWet: 0.2
            },
            'digital-glitch': {
                osc1Waveform: 'square', osc1Octave: 0, osc1Level: 0.6, osc1Detune: 0, osc1Pan: -0.3,
                osc2Waveform: 'sawtooth', osc2Octave: 0, osc2Level: 0.4, osc2Detune: -12, osc2Pan: 0.3,
                osc3Waveform: 'triangle', osc3Octave: 1, osc3Level: 0.2, osc3Detune: 7, osc3Pan: 0,
                noiseType: 'digital', noiseLevel: 0.4, noisePan: 0,
                volume: 0.45, attack: 0.001, decay: 0.2, sustain: 0.4, release: 0.3,
                filterType: 'bandpass', cutoff: 3500, resonance: 12, filterGain: 0, formantVowel: 'a', fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -8, compRatio: 8, compAttack: 0.0005, compRelease: 0.05,
                limThreshold: -2, limRelease: 0.002, roomSize: 0.1, reverbWet: 0.05
            },
            'ocean-pad': {
                osc1Waveform: 'sine', osc1Octave: 0, osc1Level: 0.5, osc1Detune: 0, osc1Pan: -0.4,
                osc2Waveform: 'triangle', osc2Octave: 0, osc2Level: 0.4, osc2Detune: 5, osc2Pan: 0.4,
                osc3Waveform: 'sine', osc3Octave: -1, osc3Level: 0.3, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'ocean', noiseLevel: 0.6, noisePan: 0,
                volume: 0.25, attack: 1.5, decay: 1.0, sustain: 0.8, release: 2.5,
                filterType: 'lowpass', cutoff: 1500, resonance: 2, filterGain: 0, formantVowel: 'a', fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -20, compRatio: 3, compAttack: 0.02, compRelease: 0.4,
                limThreshold: -8, limRelease: 0.03, roomSize: 0.8, reverbWet: 0.4
            },
            'wind-ambient': {
                osc1Waveform: 'triangle', osc1Octave: 0, osc1Level: 0.3, osc1Detune: -5, osc1Pan: -0.6,
                osc2Waveform: 'sine', osc2Octave: 1, osc2Level: 0.2, osc2Detune: 5, osc2Pan: 0.6,
                osc3Waveform: 'triangle', osc3Octave: -1, osc3Level: 0.4, osc3Detune: 0, osc3Pan: 0,
                noiseType: 'wind', noiseLevel: 0.8, noisePan: 0,
                volume: 0.2, attack: 2.5, decay: 2.0, sustain: 0.9, release: 4.0,
                filterType: 'allpass', cutoff: 800, resonance: 1, filterGain: 0, formantVowel: 'a', fmAmount: 30, fmSource: 'osc3', fmTarget: 'all', fmRatio: 0.3,
                compThreshold: -25, compRatio: 2, compAttack: 0.05, compRelease: 1.0,
                limThreshold: -12, limRelease: 0.05, roomSize: 1.0, reverbWet: 0.7
            },
            'dalai-lama-chant': {
                osc1Waveform: 'sawtooth', osc1Octave: 0, osc1Level: 0.8, osc1Detune: 0, osc1Pan: 0,
                osc2Waveform: 'sawtooth', osc2Octave: -1, osc2Level: 0.6, osc2Detune: -3, osc2Pan: 0,
                osc3Waveform: 'triangle', osc3Octave: 1, osc3Level: 0.3, osc3Detune: 2, osc3Pan: 0,
                noiseType: 'pink', noiseLevel: 0.05, noisePan: 0,
                volume: 0.35, attack: 0.8, decay: 0.4, sustain: 0.9, release: 1.5,
                filterType: 'formant', cutoff: 2000, resonance: 1, filterGain: 0, formantVowel: 'o',
                fmAmount: 0, fmSource: 'none', fmTarget: 'osc1', fmRatio: 1,
                compThreshold: -18, compRatio: 4, compAttack: 0.01, compRelease: 0.3,
                limThreshold: -6, limRelease: 0.02, roomSize: 0.8, reverbWet: 0.4
            }
        };
        
        this.noteTimeouts = new Map(); // Track note cleanup timeouts
        
        // Game-style input system
        this.inputState = {};
        this.lastInputState = {};
        this.keyToNoteMap = {
            // Lower octave
            'KeyZ': 'C3', 'KeyS': 'C#3', 'KeyX': 'D3', 'KeyD': 'D#3', 'KeyC': 'E3',
            'KeyV': 'F3', 'KeyG': 'F#3', 'KeyB': 'G3', 'KeyH': 'G#3', 'KeyN': 'A3',
            'KeyJ': 'A#3', 'KeyM': 'B3',
            // Main octave  
            'KeyQ': 'C4', 'Digit2': 'C#4', 'KeyW': 'D4', 'Digit3': 'D#4', 'KeyE': 'E4',
            'KeyR': 'F4', 'Digit5': 'F#4', 'KeyT': 'G4', 'Digit6': 'G#4', 'KeyY': 'A4',
            'Digit7': 'A#4', 'KeyU': 'B4',
            // Upper octave
            'KeyI': 'C5', 'Digit9': 'C#5', 'KeyO': 'D5', 'Digit0': 'D#5', 'KeyP': 'E5'
        };
        
        this.init();
    }
    
    async init() {
        try {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            this.masterGain = this.audioContext.createGain();
            
            // Create main filter
            this.filter = this.audioContext.createBiquadFilter();
            this.filter.type = this.settings.filterType;
            this.filter.frequency.setValueAtTime(this.settings.cutoff, this.audioContext.currentTime);
            this.filter.Q.setValueAtTime(this.settings.resonance, this.audioContext.currentTime);
            this.filter.gain.setValueAtTime(this.settings.filterGain, this.audioContext.currentTime);
            
            // Create formant filters (3 bandpass filters for vowel formants)
            this.formantFilters = [];
            this.formantGains = [];
            for (let i = 0; i < 3; i++) {
                const formantFilter = this.audioContext.createBiquadFilter();
                formantFilter.type = 'bandpass';
                const formantGain = this.audioContext.createGain();
                this.formantFilters.push(formantFilter);
                this.formantGains.push(formantGain);
            }
            this.formantMixer = this.audioContext.createGain();
            this.formantBypass = this.audioContext.createGain();
            
            // Set up formant routing: input -> formant filters -> formant mixer
            // Also direct bypass for non-formant filtering
            this.setupFormantRouting();
            
            // Add a compressor to prevent clipping - more aggressive settings
            this.compressor = this.audioContext.createDynamicsCompressor();
            this.compressor.threshold.setValueAtTime(-18, this.audioContext.currentTime); // Lower threshold
            this.compressor.knee.setValueAtTime(30, this.audioContext.currentTime);
            this.compressor.ratio.setValueAtTime(8, this.audioContext.currentTime); // Higher ratio
            this.compressor.attack.setValueAtTime(0.001, this.audioContext.currentTime);
            this.compressor.release.setValueAtTime(0.1, this.audioContext.currentTime);
            
            // Add a limiter as final safety - more aggressive
            this.limiter = this.audioContext.createDynamicsCompressor();
            this.limiter.threshold.setValueAtTime(-6, this.audioContext.currentTime); // Higher threshold
            this.limiter.knee.setValueAtTime(0, this.audioContext.currentTime);
            this.limiter.ratio.setValueAtTime(50, this.audioContext.currentTime);
            this.limiter.attack.setValueAtTime(0.0001, this.audioContext.currentTime);
            this.limiter.release.setValueAtTime(0.005, this.audioContext.currentTime); // Faster release
            
            // Set up reverb
            await this.setupReverb();
            
            // Create filter selector node
            this.filterSelector = this.audioContext.createGain();
            
            // Connect: masterGain -> compressor -> limiter -> filterSelector
            this.masterGain.connect(this.compressor);
            this.compressor.connect(this.limiter);
            this.limiter.connect(this.filterSelector);
            
            // Set up filter routing
            this.updateFilterRouting();
            
            // Connect to reverb and destination
            this.reverbOutput.connect(this.audioContext.destination);
            this.masterGain.gain.setValueAtTime(this.settings.volume, this.audioContext.currentTime);
            
            this.setupControls();
            this.setupKeyboard();
            this.startInputSystem();
            this.updateVowelControlVisibility();
            
            console.log('FreOSC initialized successfully!');
        } catch (error) {
            console.error('Failed to initialize audio context:', error);
        }
    }
    
    setupControls() {
        // Oscillator 1 controls
        const osc1WaveformSelect = document.getElementById('osc1-waveform');
        const osc1OctaveSelect = document.getElementById('osc1-octave');
        const osc1LevelSlider = document.getElementById('osc1-level');
        const osc1DetuneSlider = document.getElementById('osc1-detune');
        const osc1PanSlider = document.getElementById('osc1-pan');
        
        // Oscillator 2 controls
        const osc2WaveformSelect = document.getElementById('osc2-waveform');
        const osc2OctaveSelect = document.getElementById('osc2-octave');
        const osc2LevelSlider = document.getElementById('osc2-level');
        const osc2DetuneSlider = document.getElementById('osc2-detune');
        const osc2PanSlider = document.getElementById('osc2-pan');
        
        // Oscillator 3 controls
        const osc3WaveformSelect = document.getElementById('osc3-waveform');
        const osc3OctaveSelect = document.getElementById('osc3-octave');
        const osc3LevelSlider = document.getElementById('osc3-level');
        const osc3DetuneSlider = document.getElementById('osc3-detune');
        const osc3PanSlider = document.getElementById('osc3-pan');
        
        // Noise controls
        const noiseTypeSelect = document.getElementById('noise-type');
        const noiseLevelSlider = document.getElementById('noise-level');
        const noisePanSlider = document.getElementById('noise-pan');
        
        // Master controls
        const volumeSlider = document.getElementById('volume');
        const attackSlider = document.getElementById('attack');
        const decaySlider = document.getElementById('decay');
        const sustainSlider = document.getElementById('sustain');
        const releaseSlider = document.getElementById('release');
        
        // Filter controls
        const filterTypeSelect = document.getElementById('filter-type');
        const cutoffSlider = document.getElementById('cutoff');
        const resonanceSlider = document.getElementById('resonance');
        const filterGainSlider = document.getElementById('filter-gain');
        const formantVowelSelect = document.getElementById('formant-vowel');
        
        // FM controls
        const fmAmountSlider = document.getElementById('fm-amount');
        const fmSourceSelect = document.getElementById('fm-source');
        const fmTargetSelect = document.getElementById('fm-target');
        const fmRatioSlider = document.getElementById('fm-ratio');
        
        // Dynamics controls
        const compThresholdSlider = document.getElementById('comp-threshold');
        const compRatioSlider = document.getElementById('comp-ratio');
        const compAttackSlider = document.getElementById('comp-attack');
        const compReleaseSlider = document.getElementById('comp-release');
        const limThresholdSlider = document.getElementById('lim-threshold');
        const limReleaseSlider = document.getElementById('lim-release');
        
        const roomSizeSlider = document.getElementById('room-size');
        const reverbWetSlider = document.getElementById('reverb-wet');
        
        // Preset selector
        const presetSelector = document.getElementById('preset-selector');
        
        // Oscillator 1 event listeners
        osc1WaveformSelect.addEventListener('change', (e) => {
            this.settings.osc1Waveform = e.target.value;
        });
        
        osc1OctaveSelect.addEventListener('change', (e) => {
            this.settings.osc1Octave = parseInt(e.target.value);
        });
        
        osc1LevelSlider.addEventListener('input', (e) => {
            this.settings.osc1Level = parseFloat(e.target.value);
            document.getElementById('osc1-level-display').textContent = this.settings.osc1Level.toFixed(2);
        });
        
        osc1DetuneSlider.addEventListener('input', (e) => {
            this.settings.osc1Detune = parseFloat(e.target.value);
            document.getElementById('osc1-detune-display').textContent = this.settings.osc1Detune.toFixed(0) + '¢';
        });
        
        osc1PanSlider.addEventListener('input', (e) => {
            this.settings.osc1Pan = parseFloat(e.target.value);
            document.getElementById('osc1-pan-display').textContent = this.formatPanDisplay(this.settings.osc1Pan);
        });
        
        // Oscillator 2 event listeners
        osc2WaveformSelect.addEventListener('change', (e) => {
            this.settings.osc2Waveform = e.target.value;
        });
        
        osc2OctaveSelect.addEventListener('change', (e) => {
            this.settings.osc2Octave = parseInt(e.target.value);
        });
        
        osc2LevelSlider.addEventListener('input', (e) => {
            this.settings.osc2Level = parseFloat(e.target.value);
            document.getElementById('osc2-level-display').textContent = this.settings.osc2Level.toFixed(2);
        });
        
        osc2DetuneSlider.addEventListener('input', (e) => {
            this.settings.osc2Detune = parseFloat(e.target.value);
            document.getElementById('osc2-detune-display').textContent = this.settings.osc2Detune.toFixed(0) + '¢';
        });
        
        osc2PanSlider.addEventListener('input', (e) => {
            this.settings.osc2Pan = parseFloat(e.target.value);
            document.getElementById('osc2-pan-display').textContent = this.formatPanDisplay(this.settings.osc2Pan);
        });
        
        // Oscillator 3 event listeners
        osc3WaveformSelect.addEventListener('change', (e) => {
            this.settings.osc3Waveform = e.target.value;
        });
        
        osc3OctaveSelect.addEventListener('change', (e) => {
            this.settings.osc3Octave = parseInt(e.target.value);
        });
        
        osc3LevelSlider.addEventListener('input', (e) => {
            this.settings.osc3Level = parseFloat(e.target.value);
            document.getElementById('osc3-level-display').textContent = this.settings.osc3Level.toFixed(2);
        });
        
        osc3DetuneSlider.addEventListener('input', (e) => {
            this.settings.osc3Detune = parseFloat(e.target.value);
            document.getElementById('osc3-detune-display').textContent = this.settings.osc3Detune.toFixed(0) + '¢';
        });
        
        osc3PanSlider.addEventListener('input', (e) => {
            this.settings.osc3Pan = parseFloat(e.target.value);
            document.getElementById('osc3-pan-display').textContent = this.formatPanDisplay(this.settings.osc3Pan);
        });
        
        // Noise event listeners
        noiseTypeSelect.addEventListener('change', (e) => {
            this.settings.noiseType = e.target.value;
        });
        
        noiseLevelSlider.addEventListener('input', (e) => {
            this.settings.noiseLevel = parseFloat(e.target.value);
            document.getElementById('noise-level-display').textContent = this.settings.noiseLevel.toFixed(2);
        });
        
        noisePanSlider.addEventListener('input', (e) => {
            this.settings.noisePan = parseFloat(e.target.value);
            document.getElementById('noise-pan-display').textContent = this.formatPanDisplay(this.settings.noisePan);
        });
        
        volumeSlider.addEventListener('input', (e) => {
            this.settings.volume = parseFloat(e.target.value);
            this.updateMasterVolume();
            document.getElementById('volume-display').textContent = this.settings.volume.toFixed(2);
        });
        
        attackSlider.addEventListener('input', (e) => {
            this.settings.attack = parseFloat(e.target.value);
            document.getElementById('attack-display').textContent = this.settings.attack.toFixed(2) + 's';
        });
        
        decaySlider.addEventListener('input', (e) => {
            this.settings.decay = parseFloat(e.target.value);
            document.getElementById('decay-display').textContent = this.settings.decay.toFixed(2) + 's';
        });
        
        sustainSlider.addEventListener('input', (e) => {
            this.settings.sustain = parseFloat(e.target.value);
            document.getElementById('sustain-display').textContent = this.settings.sustain.toFixed(2);
        });
        
        releaseSlider.addEventListener('input', (e) => {
            this.settings.release = parseFloat(e.target.value);
            document.getElementById('release-display').textContent = this.settings.release.toFixed(2) + 's';
        });
        
        // Filter event listeners
        filterTypeSelect.addEventListener('change', (e) => {
            this.settings.filterType = e.target.value;
            this.updateFilterType();
            this.updateVowelControlVisibility();
        });
        
        cutoffSlider.addEventListener('input', (e) => {
            this.settings.cutoff = parseFloat(e.target.value);
            this.filter.frequency.exponentialRampToValueAtTime(this.settings.cutoff, this.audioContext.currentTime + 0.01);
            document.getElementById('cutoff-display').textContent = this.settings.cutoff.toFixed(0) + 'Hz';
        });
        
        resonanceSlider.addEventListener('input', (e) => {
            this.settings.resonance = parseFloat(e.target.value);
            this.filter.Q.exponentialRampToValueAtTime(this.settings.resonance, this.audioContext.currentTime + 0.01);
            document.getElementById('resonance-display').textContent = this.settings.resonance.toFixed(1);
        });
        
        filterGainSlider.addEventListener('input', (e) => {
            this.settings.filterGain = parseFloat(e.target.value);
            this.filter.gain.setValueAtTime(this.settings.filterGain, this.audioContext.currentTime);
            document.getElementById('filter-gain-display').textContent = this.settings.filterGain.toFixed(1) + 'dB';
        });
        
        formantVowelSelect.addEventListener('change', (e) => {
            this.settings.formantVowel = e.target.value;
            if (this.settings.filterType === 'formant') {
                this.updateFormantFilter();
            }
        });
        
        // FM event listeners
        fmAmountSlider.addEventListener('input', (e) => {
            this.settings.fmAmount = parseFloat(e.target.value);
            document.getElementById('fm-amount-display').textContent = this.settings.fmAmount.toFixed(0);
        });
        
        fmSourceSelect.addEventListener('change', (e) => {
            this.settings.fmSource = e.target.value;
        });
        
        fmTargetSelect.addEventListener('change', (e) => {
            this.settings.fmTarget = e.target.value;
        });
        
        fmRatioSlider.addEventListener('input', (e) => {
            this.settings.fmRatio = parseFloat(e.target.value);
            document.getElementById('fm-ratio-display').textContent = this.settings.fmRatio.toFixed(1);
        });
        
        // Dynamics event listeners
        compThresholdSlider.addEventListener('input', (e) => {
            this.settings.compThreshold = parseFloat(e.target.value);
            this.compressor.threshold.setValueAtTime(this.settings.compThreshold, this.audioContext.currentTime);
            document.getElementById('comp-threshold-display').textContent = this.settings.compThreshold.toFixed(0) + 'dB';
        });
        
        compRatioSlider.addEventListener('input', (e) => {
            this.settings.compRatio = parseFloat(e.target.value);
            this.compressor.ratio.setValueAtTime(this.settings.compRatio, this.audioContext.currentTime);
            document.getElementById('comp-ratio-display').textContent = this.settings.compRatio.toFixed(1) + ':1';
        });
        
        compAttackSlider.addEventListener('input', (e) => {
            this.settings.compAttack = parseFloat(e.target.value);
            this.compressor.attack.setValueAtTime(this.settings.compAttack, this.audioContext.currentTime);
            document.getElementById('comp-attack-display').textContent = (this.settings.compAttack * 1000).toFixed(0) + 'ms';
        });
        
        compReleaseSlider.addEventListener('input', (e) => {
            this.settings.compRelease = parseFloat(e.target.value);
            this.compressor.release.setValueAtTime(this.settings.compRelease, this.audioContext.currentTime);
            document.getElementById('comp-release-display').textContent = (this.settings.compRelease * 1000).toFixed(0) + 'ms';
        });
        
        limThresholdSlider.addEventListener('input', (e) => {
            this.settings.limThreshold = parseFloat(e.target.value);
            this.limiter.threshold.setValueAtTime(this.settings.limThreshold, this.audioContext.currentTime);
            document.getElementById('lim-threshold-display').textContent = this.settings.limThreshold.toFixed(1) + 'dB';
        });
        
        limReleaseSlider.addEventListener('input', (e) => {
            this.settings.limRelease = parseFloat(e.target.value);
            this.limiter.release.setValueAtTime(this.settings.limRelease, this.audioContext.currentTime);
            document.getElementById('lim-release-display').textContent = (this.settings.limRelease * 1000).toFixed(0) + 'ms';
        });
        
        roomSizeSlider.addEventListener('input', (e) => {
            this.settings.roomSize = parseFloat(e.target.value);
            this.updateReverbImpulse();
            document.getElementById('room-size-display').textContent = this.settings.roomSize.toFixed(2);
        });
        
        reverbWetSlider.addEventListener('input', (e) => {
            this.settings.reverbWet = parseFloat(e.target.value);
            this.updateReverbMix();
            document.getElementById('reverb-wet-display').textContent = this.settings.reverbWet.toFixed(2);
        });
        
        // Preset selector event listener
        presetSelector.addEventListener('change', (e) => {
            const presetName = e.target.value;
            if (presetName !== 'custom') {
                this.loadPreset(presetName);
            }
        });
    }
    
    setupKeyboard() {
        const keys = document.querySelectorAll('.key');
        
        keys.forEach(key => {
            key.addEventListener('mousedown', (e) => {
                e.preventDefault();
                const freq = parseFloat(key.dataset.freq);
                const note = key.dataset.note;
                this.playNote(freq, note);
                key.classList.add('active');
            });
            
            key.addEventListener('mouseup', (e) => {
                e.preventDefault();
                const note = key.dataset.note;
                this.stopNote(note);
                key.classList.remove('active');
            });
            
            key.addEventListener('mouseleave', (e) => {
                const note = key.dataset.note;
                this.stopNote(note);
                key.classList.remove('active');
            });
        });
        
        // Game-style input handlers - use e.code for consistent physical keys
        document.addEventListener('keydown', (e) => {
            if (e.repeat) return; // Ignore key repeats
            
            const keyCode = e.code;
            if (this.keyToNoteMap[keyCode]) {
                this.inputState[keyCode] = true;
                
                if (this.audioContext.state === 'suspended') {
                    this.audioContext.resume();
                }
            }
        });
        
        document.addEventListener('keyup', (e) => {
            const keyCode = e.code;
            if (this.keyToNoteMap[keyCode]) {
                this.inputState[keyCode] = false;
            }
        });
        
        // Clear all input on focus loss
        document.addEventListener('visibilitychange', () => {
            if (document.hidden) {
                this.inputState = {};
            }
        });
        
        window.addEventListener('blur', () => {
            this.inputState = {};
        });
    }
    
    async setupReverb() {
        // Create reverb using convolution with artificial impulse response
        this.reverbConvolver = this.audioContext.createConvolver();
        
        // Create dry/wet mixer
        this.reverbInput = this.audioContext.createGain();
        this.reverbWet = this.audioContext.createGain();
        this.reverbDry = this.audioContext.createGain();
        this.reverbOutput = this.audioContext.createGain();
        
        // Connect dry/wet paths
        this.reverbInput.connect(this.reverbDry);
        this.reverbInput.connect(this.reverbConvolver);
        this.reverbConvolver.connect(this.reverbWet);
        this.reverbDry.connect(this.reverbOutput);
        this.reverbWet.connect(this.reverbOutput);
        
        // Generate initial impulse response
        this.updateReverbImpulse();
        this.updateReverbMix();
    }
    
    generateImpulseResponse(duration, decay, reverse = false) {
        const sampleRate = this.audioContext.sampleRate;
        const length = sampleRate * duration;
        const impulse = this.audioContext.createBuffer(2, length, sampleRate);
        
        for (let channel = 0; channel < 2; channel++) {
            const channelData = impulse.getChannelData(channel);
            for (let i = 0; i < length; i++) {
                const n = reverse ? length - i : i;
                channelData[i] = (Math.random() * 2 - 1) * Math.pow(1 - n / length, decay);
            }
        }
        return impulse;
    }
    
    updateReverbImpulse() {
        const roomSize = this.settings.roomSize;
        const duration = 0.5 + roomSize * 3; // 0.5 to 3.5 seconds
        const decay = 2 + roomSize * 3; // Exponential decay factor
        
        const impulse = this.generateImpulseResponse(duration, decay);
        this.reverbConvolver.buffer = impulse;
    }
    
    updateReverbMix() {
        const wet = this.settings.reverbWet;
        const dry = 1 - wet;
        
        this.reverbWet.gain.setValueAtTime(wet, this.audioContext.currentTime);
        this.reverbDry.gain.setValueAtTime(dry, this.audioContext.currentTime);
    }
    
    formatPanDisplay(panValue) {
        if (Math.abs(panValue) < 0.05) return 'Center';
        if (panValue < 0) return `Left ${Math.abs(panValue * 100).toFixed(0)}%`;
        return `Right ${(panValue * 100).toFixed(0)}%`;
    }
    
    loadPreset(presetName) {
        const preset = this.presets[presetName];
        if (!preset) return;
        
        // Stop all notes to avoid issues during preset change
        this.stopAllNotes();
        
        // Load preset settings
        Object.assign(this.settings, preset);
        
        // Update all UI controls
        this.updateAllControls();
        
        // Update audio parameters
        this.updateMasterVolume();
        this.updateFilterType();
        this.updateVowelControlVisibility();
        this.compressor.threshold.setValueAtTime(this.settings.compThreshold, this.audioContext.currentTime);
        this.compressor.ratio.setValueAtTime(this.settings.compRatio, this.audioContext.currentTime);
        this.compressor.attack.setValueAtTime(this.settings.compAttack, this.audioContext.currentTime);
        this.compressor.release.setValueAtTime(this.settings.compRelease, this.audioContext.currentTime);
        this.limiter.threshold.setValueAtTime(this.settings.limThreshold, this.audioContext.currentTime);
        this.limiter.release.setValueAtTime(this.settings.limRelease, this.audioContext.currentTime);
        this.updateReverbImpulse();
        this.updateReverbMix();
    }
    
    updateAllControls() {
        // Oscillator controls
        document.getElementById('osc1-waveform').value = this.settings.osc1Waveform;
        document.getElementById('osc1-octave').value = this.settings.osc1Octave;
        document.getElementById('osc1-level').value = this.settings.osc1Level;
        document.getElementById('osc1-level-display').textContent = this.settings.osc1Level.toFixed(2);
        document.getElementById('osc1-detune').value = this.settings.osc1Detune;
        document.getElementById('osc1-detune-display').textContent = this.settings.osc1Detune.toFixed(0) + '¢';
        document.getElementById('osc1-pan').value = this.settings.osc1Pan;
        document.getElementById('osc1-pan-display').textContent = this.formatPanDisplay(this.settings.osc1Pan);
        
        document.getElementById('osc2-waveform').value = this.settings.osc2Waveform;
        document.getElementById('osc2-octave').value = this.settings.osc2Octave;
        document.getElementById('osc2-level').value = this.settings.osc2Level;
        document.getElementById('osc2-level-display').textContent = this.settings.osc2Level.toFixed(2);
        document.getElementById('osc2-detune').value = this.settings.osc2Detune;
        document.getElementById('osc2-detune-display').textContent = this.settings.osc2Detune.toFixed(0) + '¢';
        document.getElementById('osc2-pan').value = this.settings.osc2Pan;
        document.getElementById('osc2-pan-display').textContent = this.formatPanDisplay(this.settings.osc2Pan);
        
        document.getElementById('osc3-waveform').value = this.settings.osc3Waveform;
        document.getElementById('osc3-octave').value = this.settings.osc3Octave;
        document.getElementById('osc3-level').value = this.settings.osc3Level;
        document.getElementById('osc3-level-display').textContent = this.settings.osc3Level.toFixed(2);
        document.getElementById('osc3-detune').value = this.settings.osc3Detune;
        document.getElementById('osc3-detune-display').textContent = this.settings.osc3Detune.toFixed(0) + '¢';
        document.getElementById('osc3-pan').value = this.settings.osc3Pan;
        document.getElementById('osc3-pan-display').textContent = this.formatPanDisplay(this.settings.osc3Pan);
        
        // Noise controls
        document.getElementById('noise-type').value = this.settings.noiseType;
        document.getElementById('noise-level').value = this.settings.noiseLevel;
        document.getElementById('noise-level-display').textContent = this.settings.noiseLevel.toFixed(2);
        document.getElementById('noise-pan').value = this.settings.noisePan;
        document.getElementById('noise-pan-display').textContent = this.formatPanDisplay(this.settings.noisePan);
        
        // Master controls
        document.getElementById('volume').value = this.settings.volume;
        document.getElementById('volume-display').textContent = this.settings.volume.toFixed(2);
        
        // Envelope controls
        document.getElementById('attack').value = this.settings.attack;
        document.getElementById('attack-display').textContent = this.settings.attack.toFixed(2) + 's';
        document.getElementById('decay').value = this.settings.decay;
        document.getElementById('decay-display').textContent = this.settings.decay.toFixed(2) + 's';
        document.getElementById('sustain').value = this.settings.sustain;
        document.getElementById('sustain-display').textContent = this.settings.sustain.toFixed(2);
        document.getElementById('release').value = this.settings.release;
        document.getElementById('release-display').textContent = this.settings.release.toFixed(2) + 's';
        
        // Filter controls
        document.getElementById('filter-type').value = this.settings.filterType;
        document.getElementById('cutoff').value = this.settings.cutoff;
        document.getElementById('cutoff-display').textContent = this.settings.cutoff.toFixed(0) + 'Hz';
        document.getElementById('resonance').value = this.settings.resonance;
        document.getElementById('resonance-display').textContent = this.settings.resonance.toFixed(1);
        document.getElementById('filter-gain').value = this.settings.filterGain;
        document.getElementById('filter-gain-display').textContent = this.settings.filterGain.toFixed(1) + 'dB';
        document.getElementById('formant-vowel').value = this.settings.formantVowel;
        
        // FM controls
        document.getElementById('fm-amount').value = this.settings.fmAmount;
        document.getElementById('fm-amount-display').textContent = this.settings.fmAmount.toFixed(0);
        document.getElementById('fm-source').value = this.settings.fmSource;
        document.getElementById('fm-target').value = this.settings.fmTarget;
        document.getElementById('fm-ratio').value = this.settings.fmRatio;
        document.getElementById('fm-ratio-display').textContent = this.settings.fmRatio.toFixed(1);
        
        // Dynamics controls
        document.getElementById('comp-threshold').value = this.settings.compThreshold;
        document.getElementById('comp-threshold-display').textContent = this.settings.compThreshold.toFixed(0) + 'dB';
        document.getElementById('comp-ratio').value = this.settings.compRatio;
        document.getElementById('comp-ratio-display').textContent = this.settings.compRatio.toFixed(1) + ':1';
        document.getElementById('comp-attack').value = this.settings.compAttack;
        document.getElementById('comp-attack-display').textContent = (this.settings.compAttack * 1000).toFixed(0) + 'ms';
        document.getElementById('comp-release').value = this.settings.compRelease;
        document.getElementById('comp-release-display').textContent = (this.settings.compRelease * 1000).toFixed(0) + 'ms';
        document.getElementById('lim-threshold').value = this.settings.limThreshold;
        document.getElementById('lim-threshold-display').textContent = this.settings.limThreshold.toFixed(1) + 'dB';
        document.getElementById('lim-release').value = this.settings.limRelease;
        document.getElementById('lim-release-display').textContent = (this.settings.limRelease * 1000).toFixed(0) + 'ms';
        
        // Reverb controls
        document.getElementById('room-size').value = this.settings.roomSize;
        document.getElementById('room-size-display').textContent = this.settings.roomSize.toFixed(2);
        document.getElementById('reverb-wet').value = this.settings.reverbWet;
        document.getElementById('reverb-wet-display').textContent = this.settings.reverbWet.toFixed(2);
    }
    
    startInputSystem() {
        // Game-style input polling at 120fps for ultra-smooth response
        const processInput = () => {
            // Check each mapped key for state changes
            Object.keys(this.keyToNoteMap).forEach(keyCode => {
                const note = this.keyToNoteMap[keyCode];
                const isPressed = this.inputState[keyCode] === true;
                const wasPressed = this.lastInputState[keyCode] === true;
                
                // Key just pressed (rising edge)
                if (isPressed && !wasPressed) {
                    const keyElement = document.querySelector(`[data-note="${note}"]`);
                    if (keyElement && !this.activeOscillators.has(note)) {
                        const freq = parseFloat(keyElement.dataset.freq);
                        this.playNote(freq, note);
                        keyElement.classList.add('active');
                    }
                }
                
                // Key just released (falling edge)
                else if (!isPressed && wasPressed) {
                    this.stopNote(note);
                    const keyElement = document.querySelector(`[data-note="${note}"]`);
                    if (keyElement) {
                        keyElement.classList.remove('active');
                    }
                }
            });
            
            // Store current state for next frame
            this.lastInputState = { ...this.inputState };
            
            // Continue at ~120fps for ultra-responsive input
            setTimeout(processInput, 8);
        };
        
        // Start the input system
        processInput();
    }
    
    
    updateMasterVolume() {
        // Set master volume based on slider only - let compressor/limiter handle clipping
        this.masterGain.gain.setValueAtTime(this.settings.volume, this.audioContext.currentTime);
    }
    
    setupFormantRouting() {
        // Connect formant filters in parallel
        for (let i = 0; i < 3; i++) {
            this.formantFilters[i].connect(this.formantGains[i]);
            this.formantGains[i].connect(this.formantMixer);
        }
        
        // Set initial formant frequencies
        this.updateFormantFilter();
    }
    
    updateFilterRouting() {
        // Disconnect existing connections
        try {
            this.filterSelector.disconnect();
            this.filter.disconnect();
            this.formantMixer.disconnect();
        } catch (e) {
            // Ignore disconnection errors
        }
        
        if (this.settings.filterType === 'formant') {
            // Route through formant filters
            for (let i = 0; i < 3; i++) {
                this.filterSelector.connect(this.formantFilters[i]);
            }
            this.formantMixer.connect(this.reverbInput);
        } else {
            // Route through regular filter
            this.filterSelector.connect(this.filter);
            this.filter.connect(this.reverbInput);
        }
    }
    
    updateFilterType() {
        // Update the filter type and refresh all parameters
        if (this.settings.filterType === 'formant') {
            this.updateFormantFilter();
        } else {
            this.filter.type = this.settings.filterType;
            this.filter.frequency.setValueAtTime(this.settings.cutoff, this.audioContext.currentTime);
            this.filter.Q.setValueAtTime(this.settings.resonance, this.audioContext.currentTime);
            this.filter.gain.setValueAtTime(this.settings.filterGain, this.audioContext.currentTime);
        }
        
        // Update routing
        this.updateFilterRouting();
    }
    
    updateVowelControlVisibility() {
        const vowelControl = document.getElementById('vowel-control');
        if (this.settings.filterType === 'formant') {
            vowelControl.style.display = 'block';
        } else {
            vowelControl.style.display = 'none';
        }
    }
    
    updateFormantFilter() {
        // Formant frequencies for different vowels (F1, F2, F3 in Hz)
        const formantData = {
            'a':  [730, 1090, 2440],  // "ah" sound
            'e':  [270, 2290, 3010],  // "eh" sound  
            'i':  [390, 1990, 2550],  // "ee" sound
            'o':  [570, 840, 2410],   // "oh" sound
            'u':  [440, 1020, 2240],  // "oo" sound
            'ae': [660, 1720, 2410],  // "ay" sound
            'aw': [610, 900, 2150],   // "aw" sound
            'er': [490, 1350, 1690]   // "ur" sound
        };
        
        const formants = formantData[this.settings.formantVowel] || formantData['a'];
        const bandwidths = [90, 120, 150]; // Bandwidth for each formant
        
        // Set up each formant filter
        for (let i = 0; i < 3; i++) {
            this.formantFilters[i].frequency.setValueAtTime(formants[i], this.audioContext.currentTime);
            this.formantFilters[i].Q.setValueAtTime(formants[i] / bandwidths[i], this.audioContext.currentTime);
            
            // Set relative gains (F1 loudest, F2 medium, F3 quieter)
            const gains = [1.0, 0.7, 0.3];
            this.formantGains[i].gain.setValueAtTime(gains[i], this.audioContext.currentTime);
        }
    }
    
    createNoiseBuffer(type, duration = 2.0) {
        const sampleRate = this.audioContext.sampleRate;
        const frameCount = sampleRate * duration;
        const buffer = this.audioContext.createBuffer(1, frameCount, sampleRate);
        const output = buffer.getChannelData(0);
        
        if (type === 'white') {
            // White noise - equal power across all frequencies
            for (let i = 0; i < frameCount; i++) {
                output[i] = Math.random() * 2 - 1;
            }
        } else if (type === 'pink') {
            // Pink noise - 1/f frequency response
            let b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
            for (let i = 0; i < frameCount; i++) {
                const white = Math.random() * 2 - 1;
                b0 = 0.99886 * b0 + white * 0.0555179;
                b1 = 0.99332 * b1 + white * 0.0750759;
                b2 = 0.96900 * b2 + white * 0.1538520;
                b3 = 0.86650 * b3 + white * 0.3104856;
                b4 = 0.55000 * b4 + white * 0.5329522;
                b5 = -0.7616 * b5 - white * 0.0168980;
                output[i] = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362) * 0.11;
                b6 = white * 0.115926;
            }
        } else if (type === 'brown') {
            // Brown noise - 1/f^2 frequency response (Brownian noise)
            let lastOut = 0.0;
            for (let i = 0; i < frameCount; i++) {
                const white = Math.random() * 2 - 1;
                output[i] = (lastOut + (0.02 * white)) / 1.02;
                lastOut = output[i];
                output[i] *= 3.5; // Compensate for the volume reduction
            }
        } else if (type === 'blue') {
            // Blue noise - f frequency response (opposite of pink)
            let lastOut = 0.0;
            for (let i = 0; i < frameCount; i++) {
                const white = Math.random() * 2 - 1;
                output[i] = white - lastOut;
                lastOut = white;
                output[i] *= 0.5; // Compensate for increased amplitude
            }
        } else if (type === 'violet') {
            // Violet noise - f^2 frequency response (opposite of brown)
            let lastOut = 0.0;
            let lastOut2 = 0.0;
            for (let i = 0; i < frameCount; i++) {
                const white = Math.random() * 2 - 1;
                output[i] = white - 2 * lastOut + lastOut2;
                lastOut2 = lastOut;
                lastOut = white;
                output[i] *= 0.25; // Compensate for increased amplitude
            }
        } else if (type === 'grey') {
            // Grey noise - psychoacoustically flat noise
            let b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0;
            for (let i = 0; i < frameCount; i++) {
                const white = Math.random() * 2 - 1;
                b0 = 0.99765 * b0 + white * 0.0990460;
                b1 = 0.96300 * b1 + white * 0.2965164;
                b2 = 0.57000 * b2 + white * 1.0526913;
                b3 = 0.14001 * b3 + white * 0.1848;
                output[i] = (b0 + b1 + b2 + b3 + white * 0.0362) * 0.15;
            }
        } else if (type === 'crackle') {
            // Vinyl crackle - sparse random pops and clicks
            for (let i = 0; i < frameCount; i++) {
                if (Math.random() < 0.002) { // Sparse pops
                    output[i] = (Math.random() * 2 - 1) * Math.random(); // Random amplitude pop
                } else if (Math.random() < 0.01) { // Background hiss
                    output[i] = (Math.random() * 2 - 1) * 0.1;
                } else {
                    output[i] = 0;
                }
            }
        } else if (type === 'digital') {
            // Digital noise - quantized/aliased noise
            for (let i = 0; i < frameCount; i++) {
                let sample = Math.random() * 2 - 1;
                // Quantize to simulate low bit depth
                sample = Math.floor(sample * 32) / 32;
                // Add some aliasing
                if (i > 0) {
                    sample += output[i-1] * 0.3;
                }
                output[i] = Math.max(-1, Math.min(1, sample));
            }
        } else if (type === 'wind') {
            // Wind noise - low frequency rumble with variations
            let b0 = 0, b1 = 0, b2 = 0;
            let modPhase = 0;
            for (let i = 0; i < frameCount; i++) {
                const white = Math.random() * 2 - 1;
                // Heavy low-pass filtering
                b0 = 0.999 * b0 + white * 0.001;
                b1 = 0.995 * b1 + b0 * 0.005;
                b2 = 0.99 * b2 + b1 * 0.01;
                
                // Add slow modulation
                modPhase += 0.0001;
                const mod = Math.sin(modPhase) * 0.3;
                
                output[i] = b2 * (1 + mod) * 8; // Amplify the quiet result
            }
        } else if (type === 'ocean') {
            // Ocean waves - filtered noise with wave-like modulation
            let b0 = 0, b1 = 0, b2 = 0;
            let wavePhase = 0;
            let waveAmp = 0;
            for (let i = 0; i < frameCount; i++) {
                const white = Math.random() * 2 - 1;
                
                // Band-pass filtering for wave sound
                b0 = 0.995 * b0 + white * 0.005;
                b1 = 0.98 * b1 + (b0 - b2) * 0.02;
                b2 = 0.99 * b2 + b1 * 0.01;
                
                // Wave modulation - slow and irregular
                wavePhase += 0.00005 + Math.random() * 0.00002;
                waveAmp = Math.sin(wavePhase) + Math.sin(wavePhase * 2.3) * 0.5;
                waveAmp = Math.max(0, waveAmp); // Only positive waves
                
                output[i] = b1 * (0.3 + waveAmp * 0.7) * 3;
            }
        }
        
        return buffer;
    }
    
    createNoiseSource(type) {
        const bufferSource = this.audioContext.createBufferSource();
        bufferSource.buffer = this.createNoiseBuffer(type);
        bufferSource.loop = true;
        return bufferSource;
    }
    
    playNote(frequency, note) {
        try {
            // Ensure audio context is ready
            if (this.audioContext.state === 'suspended') {
                this.audioContext.resume();
            }
            
            // Stop existing note if any
            if (this.activeOscillators.has(note)) {
                this.stopNote(note);
            }
            
            // Create mixer for this note
            const noteGain = this.audioContext.createGain();
            const oscillators = [];
            const gainNodes = [];
            const fmOscillator = this.settings.fmAmount > 0 && this.settings.fmSource !== 'none' ? this.audioContext.createOscillator() : null;
            const fmGain = fmOscillator ? this.audioContext.createGain() : null;
            
            // Create FM oscillator if needed
            if (fmOscillator && fmGain) {
                const fmFreq = frequency * this.settings.fmRatio;
                fmOscillator.frequency.setValueAtTime(fmFreq, this.audioContext.currentTime);
                fmOscillator.type = this.getFMWaveform();
                fmGain.gain.setValueAtTime(this.settings.fmAmount, this.audioContext.currentTime);
                fmOscillator.connect(fmGain);
                fmOscillator.start(this.audioContext.currentTime);
            }
            
            // Create up to 3 oscillators
            const oscConfigs = [
                { id: 'osc1', waveform: this.settings.osc1Waveform, octave: this.settings.osc1Octave, level: this.settings.osc1Level, detune: this.settings.osc1Detune, pan: this.settings.osc1Pan },
                { id: 'osc2', waveform: this.settings.osc2Waveform, octave: this.settings.osc2Octave, level: this.settings.osc2Level, detune: this.settings.osc2Detune, pan: this.settings.osc2Pan },
                { id: 'osc3', waveform: this.settings.osc3Waveform, octave: this.settings.osc3Octave, level: this.settings.osc3Level, detune: this.settings.osc3Detune, pan: this.settings.osc3Pan }
            ];
            
            oscConfigs.forEach((config, i) => {
                // Only create oscillator if level > 0
                if (config.level > 0) {
                    const oscillator = this.audioContext.createOscillator();
                    const gainNode = this.audioContext.createGain();
                    const panNode = this.audioContext.createStereoPanner();
                    
                    oscillator.type = config.waveform;
                    
                    // Calculate octave shift (each octave is *2 or /2)
                    const octaveMultiplier = Math.pow(2, config.octave);
                    
                    // Calculate detuned frequency (cents to frequency ratio)
                    const detuneRatio = Math.pow(2, config.detune / 1200);
                    
                    // Apply both octave and detune
                    const finalFreq = frequency * octaveMultiplier * detuneRatio;
                    
                    oscillator.frequency.setValueAtTime(finalFreq, this.audioContext.currentTime);
                    
                    // Apply FM if this oscillator is the target
                    const shouldReceiveFM = (this.settings.fmTarget === 'all' || this.settings.fmTarget === config.id) && fmGain;
                    if (shouldReceiveFM) {
                        fmGain.connect(oscillator.frequency);
                    }
                    
                    // Set oscillator level
                    gainNode.gain.setValueAtTime(config.level, this.audioContext.currentTime);
                    
                    // Set panning (-1 = full left, 0 = center, 1 = full right)
                    panNode.pan.setValueAtTime(config.pan, this.audioContext.currentTime);
                    
                    // Connect: oscillator -> gain -> pan -> noteGain
                    oscillator.connect(gainNode);
                    gainNode.connect(panNode);
                    panNode.connect(noteGain);
                    
                    oscillators.push(oscillator);
                    gainNodes.push(gainNode);
                }
            });
            
            // Add noise oscillator if level > 0
            let noiseSource = null;
            let noiseGain = null;
            let noisePan = null;
            
            if (this.settings.noiseLevel > 0) {
                noiseSource = this.createNoiseSource(this.settings.noiseType);
                noiseGain = this.audioContext.createGain();
                noisePan = this.audioContext.createStereoPanner();
                
                // Set noise level
                noiseGain.gain.setValueAtTime(this.settings.noiseLevel, this.audioContext.currentTime);
                
                // Set panning
                noisePan.pan.setValueAtTime(this.settings.noisePan, this.audioContext.currentTime);
                
                // Connect: noiseSource -> gain -> pan -> noteGain
                noiseSource.connect(noiseGain);
                noiseGain.connect(noisePan);
                noisePan.connect(noteGain);
                
                oscillators.push(noiseSource);
                gainNodes.push(noiseGain);
            }
            
            // Connect note mixer to master gain
            noteGain.connect(this.masterGain);
            
            // Apply envelope to note gain
            const currentTime = this.audioContext.currentTime;
            const attackTime = Math.max(currentTime + 0.001, currentTime + this.settings.attack);
            const decayTime = attackTime + Math.max(0.001, this.settings.decay);
            
            noteGain.gain.setValueAtTime(0, currentTime);
            noteGain.gain.linearRampToValueAtTime(1, attackTime);
            noteGain.gain.linearRampToValueAtTime(this.settings.sustain, decayTime);
            
            // Start all oscillators
            oscillators.forEach(osc => osc.start(currentTime));
            
            this.activeOscillators.set(note, { oscillators, gainNodes, noteGain, fmOscillator, fmGain, noiseSource, noiseGain, noisePan });
            
            // Clear any existing timeout for this note
            if (this.noteTimeouts.has(note)) {
                clearTimeout(this.noteTimeouts.get(note));
                this.noteTimeouts.delete(note);
            }
            
            // Set a safety timeout to auto-stop notes after 30 seconds
            const timeoutId = setTimeout(() => {
                if (this.activeOscillators.has(note)) {
                    console.warn(`Force stopping stuck note: ${note}`);
                    this.stopNote(note);
                    const keyElement = document.querySelector(`[data-note="${note}"]`);
                    if (keyElement) {
                        keyElement.classList.remove('active');
                    }
                }
                this.noteTimeouts.delete(note);
            }, 30000);
            this.noteTimeouts.set(note, timeoutId);
            
            
        } catch (error) {
            console.error('Error playing note:', error);
        }
    }
    
    getFMWaveform() {
        // Use the source oscillator's waveform for FM
        switch (this.settings.fmSource) {
            case 'osc1': return this.settings.osc1Waveform;
            case 'osc2': return this.settings.osc2Waveform;
            case 'osc3': return this.settings.osc3Waveform;
            default: return 'sine';
        }
    }
    
    stopNote(note) {
        const noteData = this.activeOscillators.get(note);
        if (!noteData) return;
        
        const { oscillators, gainNodes, noteGain, fmOscillator, fmGain } = noteData;
        const currentTime = this.audioContext.currentTime;
        const releaseTime = currentTime + this.settings.release;
        
        // Apply release envelope to note gain
        noteGain.gain.cancelScheduledValues(currentTime);
        noteGain.gain.setValueAtTime(noteGain.gain.value, currentTime);
        noteGain.gain.linearRampToValueAtTime(0, releaseTime);
        
        // Stop all oscillators
        oscillators.forEach(osc => osc.stop(releaseTime));
        
        // Stop FM oscillator if it exists
        if (fmOscillator) {
            fmOscillator.stop(releaseTime);
        }
        
        setTimeout(() => {
            this.activeOscillators.delete(note);
            // Clear the safety timeout
            if (this.noteTimeouts.has(note)) {
                clearTimeout(this.noteTimeouts.get(note));
                this.noteTimeouts.delete(note);
            }
        }, this.settings.release * 1000 + 100);
    }
    
    stopAllNotes() {
        this.activeOscillators.forEach((noteData, note) => {
            this.stopNote(note);
        });
    }
}

document.addEventListener('DOMContentLoaded', () => {
    const synth = new FreOSC();
    
    document.addEventListener('click', () => {
        if (synth.audioContext && synth.audioContext.state === 'suspended') {
            synth.audioContext.resume();
        }
    }, { once: true });
    
    window.synth = synth;
});