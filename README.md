# Moonrunner - 80s Synthesizer

A comprehensive 80s-style synthesizer plugin featuring multiple synthesis engines inspired by the most iconic synthesizers of the 1980s.

## Features

### Synthesis Engines

1. **FM Synthesis (Yamaha DX7)**
   - 6-operator FM synthesis
   - Multiple algorithms
   - Per-operator ADSR envelopes
   - LFO modulation
   - Multiple waveforms per operator

2. **Analog Synthesis (Prophet-5, Jupiter-8, Juno-60/106)**
   - Dual oscillators with multiple waveforms
   - 24dB/octave lowpass filter
   - Filter and amplitude envelopes
   - LFO with multiple destinations
   - Juno-style chorus effect
   - Sub oscillator

3. **Sampler (Fairlight CMI)**
   - Sample playback with pitch shifting
   - Loop support
   - Filter processing
   - ADSR envelope

### Design Philosophy

Moonrunner combines the best elements of classic 80s synthesizers:
- **Yamaha DX7**: Revolutionary FM synthesis
- **Sequential Circuits Prophet-5**: Rich analog polyphony
- **Roland Jupiter-8 & Juno-60/106**: Lush analog sounds with chorus
- **Fairlight CMI**: Groundbreaking sampling technology
- **Korg M1**: Digital workstation capabilities

## Building

This is a JUCE project. Open `Moonrunner.jucer` in Projucer or use your preferred build system.

### Requirements
- JUCE framework (referenced from `../NebulaEQ/JUCE/modules`)
- C++17 compatible compiler
- macOS or Windows build system

## Usage

1. Load the plugin in your DAW
2. Select synthesis mode: FM, Analog, or Sampler
3. Play MIDI notes to trigger the synthesizer
4. Adjust master volume and tune as needed

## Future Enhancements

- Expanded parameter controls for each synthesis engine
- Preset management system
- Additional effects (reverb, delay)
- More FM algorithms
- Expanded sampler features
- MIDI learn functionality

## License

Copyright 2025 CK Audio Design



