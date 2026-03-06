<p align="center"><img src="Assets/Features/Plugin%20UI%20v1.2.png" width="80%" alt="View of full plugin" /></p>

# Just a Sample

[Your favorite sampler shouldn't be complicated.](https://bobona.github.io/just-a-sample/)

[Promo video](https://www.youtube.com/watch?v=P7dgOe_frXw)

[See releases](https://github.com/BOBONA/Just-a-Sample/releases)

Available for Windows, Mac, and Linux in VST3/AU.

## Overview
Just a Sample is a powerful, _modern_ audio sampler, with a focus on simplicity and ease of use. 
Best of all, it's **free** and **open-source**! I spent a lot of time on this project, and I think it will
be a great addition to your plugin collection. 

Feel free to take a look at the source code and reach out with any questions or suggestions. I took care
to make the code as clean and readable as possible, with a focus on good design practices. That being said, 
I am still new to audio programming.

## Features

[Detailed feature list](FEATURES.md)

#### Core features
- Smoothly zoom in to the level of individual samples to 
set bounds as accurately as you need. Waveform drawing is **optimized** for large samples.
- Integrated [Bungee time stretcher](https://github.com/kupix/bungee) allows for
freely modulating time and pitch independently. Try extreme slow-downs (0.01x) for unique sounds.
- Modern navigation controls allow for easy browsing of waveforms with touchpad or mouse.
- A routable FX chain includes reverb, chorus, distortion, and EQ for easy sound design.
- Waveform Mode activates when small bounds are set, looping the waveform periodically like a wavetable synth. This turns Just a Sample into a unique tone generator.
- Easily modify attack and release curves.
- Includes equal power cross-fade looping with separate attack and release sample portions.
- Supports pitch bend and fine-tuning.

#### Extras
- Disable antialiasing for a gritty LoFi effect.
- Record samples directly into the sampler.
- Store samples directly in plugin state to remove dependencies (can be disabled).
- Auto-tune to A440 (experimental, works best on simple sounds).

## Installation
### Pre-built 

You can download installers from,
- [itch.io](https://binyaminf.itch.io/just-a-sample)
- [GitHub Releases](https://github.com/BOBONA/Just-a-Sample/releases)

### Build from source
Just a Sample is easy to build from source.

```bash
# Clone the repository
git clone --recurse-submodules https://github.com/BOBONA/Just-a-Sample/.git

# Go to the project root
cd Just-a-Sample

# Configure the plugin (replace <platform> with windows, mac, or linux)
cmake --preset=<platform>

# Build the plugin
cmake --build --preset=release-<platform>
```

Note that the first time you configure the project, CMake will download JUCE and other dependencies, which may take a while.

Your built plugin will be located in `out/build/<platform>/<configuration>/JustASample_artefacts/<Configuration>/<format>/` where `<platform>` is windows, mac, or linux, `<configuration>` is either debug or release, and `<format>` is either VST3 or AU (Mac only).

#### Build Options

You can set the following CMake options by passing `-D<option>=<value>` to the CMake configure step.

- `JAS_DARKMODE_DEFAULT`: Set the default theme to dark mode (default: OFF)
- `JAS_VST3_REAPER_INTEGRATION`: Enable Reaper-specific VST3 extensions (Windows only, default: OFF)

#### Requirements

**All Platforms:**
- Git
- CMake 3.22 or higher
- Platform-specific build tools

**Windows:**
- Ninja build system
- Visual Studio 2019 or later

**macOS:**
- Xcode 12 or later

**Linux:**
- GCC 9+
- Ninja build system
- JUCE dependencies (install via the provided script, or do it yourself):
  ```bash
  ./Releases/Linux/install-dependencies.sh
  ```
  
### Dependencies
Just a Sample relies on:
- [JUCE](https://juce.com/) for plugin framework and UI
- [Bungee](https://bungee.parabolaresearch.com/) for time-stretching and pitch-shifting
- [Melatonin Blur](https://melatonin.dev/manuals/melatonin-blur/) for fast shadow-compositing
- [LEAF](https://github.com/spiricom/LEAF) for a fast pitch-detection algorithm
- [readerwriterqueue](https://github.com/cameron314/readerwriterqueue) for lock-free thread communication
- [Gin](https://github.com/FigBug/Gin) for AirWindows distortion and SimpleVerb reverb algorithms
- [MTS-ESP](https://github.com/ODDSound/MTS-ESP/tree/main/Client) for microtuning support
- [reaper-sdk](https://github.com/justinfrankel/reaper-sdk/tree/main/sdk) for Reaper-specific VST3 extensions

JUCE, Bungee, Melatonin Blur, and LEAF are included through [CMake](CMakeLists.txt#L29), readerwriterqueue is included as a 
git [submodule](.gitmodules), and the others are included as source files [in the project](External).

## Credits
This is my first audio plugin, and I am very happy at how it turned out! This was a long-running project
that took place over the course of over a year (with long breaks), and I learned a lot on the way.

Special thanks to JUCE and The Audio Programmer Discord for all the help. 
