# Xiaozhi Linux Port

This project ports the core `xiaozhi-linux` flow as a background service:

- `home_gui` owns activation, WebSocket control, UI state, and GPIO wake.
- `xiaozhi_sound_app` is built from upstream `sound_app` and handles ALSA capture/playback plus Opus encode/decode.
- The two processes use upstream-compatible UDP ports:
  - `5676`: audio Opus upload from `xiaozhi_sound_app` to `home_gui`
  - `5677`: audio Opus download from `home_gui` to `xiaozhi_sound_app`
  - `5678`/`5679`: UI compatibility channel

## Config

Runtime settings live under `xiaozhi` in `config.yaml` (template in `config.yaml.example`).

- `enabled`: starts the Xiaozhi backend when modules start.
- `soundAppPath`: executable path for `xiaozhi_sound_app`; relative paths are resolved next to `home_gui`.
- `configPath`: UUID persistence path. The default matches upstream: `/etc/xiaozhi.cfg`.
- `wakeGpioLine`: sysfs GPIO number used for wake. Set to `-1` to disable GPIO polling.
- `wakeGpioActiveLow`: set to `true` when the button pulls the GPIO low.
- `wakeGpioDebounceMs`: GPIO polling/debounce interval.

Example:

```yaml
xiaozhi:
  enabled: true
  soundAppPath: xiaozhi_sound_app
  wakeGpioLine: 42
  wakeGpioActiveLow: true
```

## Build

Use the normal RK3506 command:

```sh
./scripts/build.sh rk3506
```

Successful builds produce:

- `build-rk3506/home_gui`
- `build-rk3506/xiaozhi_sound_app`

Deploy both binaries together so `home_gui` can start the audio backend.
