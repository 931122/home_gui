# Repository Guidelines

## Project Structure & Module Organization

This repository contains a Qt 6 application for camera, ONVIF, Home Assistant, and weather integration.

- `src/main.cpp`: application entry and QML type registration.
- `src/core/`: configuration, global state, platform helpers, and module lifecycle.
- `src/modules/video/`: RTSP, ONVIF discovery, and PTZ control logic.
- `src/modules/homeassistant/`: Home Assistant integration hooks.
- `src/ui/`: `AppController` and custom UI items such as `FfmpegVideoItem`.
- `src/ui/qml/`: QML screens and reusable UI surfaces.
- `resources/qml.qrc`: QML resource manifest.
- `config.yaml.example`: runtime configuration template (copy to `config.yaml` for local run).
- `cmake/toolchains/`: cross-compilation toolchains.
- `scripts/`: helper build scripts.

There is currently no dedicated `tests/` directory.

## Build, Test, and Development Commands

- `cmake -S . -B build && cmake --build build`: configure and build locally.
- `bash scripts/build.sh native`: one-step local native build (auto-detects Linux/macOS).
- `bash scripts/build.sh linux`: one-step local Linux build.
- `bash scripts/build.sh cross [toolchain]`: generic Linux cross build with CMake toolchain file.
- `bash scripts/build.sh buildroot [output]`: Buildroot cross build (supports RK3506, RK3568, etc.).
- `bash scripts/build.sh rk3506`: preset shortcut for RK3506 Buildroot build.
- `bash scripts/build.sh rk3568`: preset shortcut for RK3568 Buildroot build.
- `bash scripts/build.sh android`: one-step Android build (requires Qt 6 for Android and NDK).
- `bash scripts/build.sh ios [simulator|device]`: one-step iOS build (requires Xcode and Qt 6 for iOS).
- `./build/home_gui`: run the locally built application.
- `cmake -S . -B build-cross -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-cross.cmake`: explicit cross-build entry.

Use `home_gui.pro` when opening the project in Qt Creator.

## Coding Style & Naming Conventions

- Use C++17 and Qt 6 APIs already present in the project.
- Prefer 4-space indentation in C++ and consistent property alignment in QML.
- Class names use `PascalCase`; functions and variables use `camelCase`.
- Keep QML IDs short and descriptive, for example `videoLoader`, `cameraPopup`.
- Follow the existing module split: configuration in `core`, device logic in `modules`, UI state in `ui`.

No formatter or linter is currently enforced, so match surrounding code closely.

## Testing Guidelines

This project does not yet have automated unit tests. At minimum:

- build successfully before submitting changes;
- verify the main UI loads;
- verify impacted flows such as camera playback, ONVIF detection, PTZ, or weather refresh.

When adding tests later, place them under a new `tests/` directory and name them by feature, for example `test_onvif_discovery.cpp`.

## Commit & Pull Request Guidelines

Recent commits use short imperative messages, for example:

- `Initial project snapshot`
- `Add gitignore and drop build artifacts`

Prefer concise commit subjects under 72 characters. For pull requests, include:

- a brief summary of behavior changes;
- affected platforms, such as `macOS` or `RK3506`;
- config changes in `config.yaml.example`;
- screenshots for visible QML/UI updates.

## Security & Configuration Tips

Do not commit real device credentials or private API keys. Use `config.yaml` for local device settings (ignored by git) and keep private secrets out of `config.yaml.example` before sharing branches or patches.

