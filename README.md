# Vercidium Audio (Native)

Raytraced audio GDExtension with realistic muffling, reverb, ambience and visualisation for non-Mono Godot 4, using OpenAL Soft as the audio backend.

> [!WARNING]
> This repository contains the source code for the plugin. For releases, see [vaudio-godot-native-openal-3d-release](https://github.com/vercidium-audio/vaudio-godot-native-openal-3d-release).

For Mono Godot (C#), please use [this plugin](https://github.com/vercidium-audio/vaudio-godot-mono-openal-3d/releases).

This repository requires Vercidium Audio v1.8.0 and OpenAL Soft to run. Windows, Linux and macOS are supported.
- Download the Vercidium Audio SDK from [vercidium.com](https://vercidium.com)
- Download OpenAL Soft from [github.com/kcat/openal-soft](https://github.com/kcat/openal-soft/releases/tag/1.25.2)

> Please note that the Vercidium Audio SDK is not free for commercial use. See [vercidium.com/eula](https://vercidium.com/eula)

## Features

- Muffle sounds in real time
- Accurate reverb in any environment
- Innovative event-based raytracing system
- Realistic energy-based model using materials
- Dynamic scene updates - automatically handles moving objects

## References
- [Release repo](https://github.com/vercidium-audio/vaudio-godot-native-openal-3d-release)
- [Vercidium Audio documentation](https://vercidium.com/docs)

## Building

On Windows, run `build.bat`. On Linux/macOS, run `build-unix.sh` (auto-detects the host - Linux builds an x86_64 `.so`, macOS builds an arm64 `.dylib`).

To automatically copy the plugin to your game, add your game's path to `.env`, e.g. `VAUDIO_RELEASE_DIR=your-godot-game\addons\vaudio-godot-native-openal-3d-release`

## Licencing

The Vercidium Audio SDK is free for non-commercial products only. To purchase a licence for commercial use, head over to the [Vercidium Audio website](https://vercidium.com).
