# Home GUI Config Reference

本文档说明 `config.yaml`（模板见 `config.yaml.example`）的字段含义、推荐写法、最小示例，以及旧配置的兼容策略。

## 1. Design Rules

当前配置遵循这几个原则：

- 只保留用户真正需要维护的输入项。
- 运行期可自动探测的内容，不写进配置。
- 配置格式尽量稳定，代码内部做兼容迁移。

例如：

- 摄像头的 `profileToken`、`deviceServiceUrl`、`mediaServiceUrl`、`ptzServiceUrl` 已移除。
- 摄像头的 `ptzEnabled` 不再建议配置；PTZ 可用性由 ONVIF 自动探测。
- Wi‑Fi 的 `ctrlPath`、`configPath` 已移除。

这些字段都由程序在运行时自动识别。

## 2. Full Example

```yaml
platform:
  chip: RK3568
  width: 800
  height: 480
  renderMode: eglfs
  reducedEffects: false

cameras:
  - enabled: true
    renderer: video
    url: onvif://192.168.1.100:2020
    backend: libav
    decoder: auto
    onvifProfile: auto
    cameraName: 前门摄像头
    username: admin
    password: YOUR_PASSWORD

homeAssistant:
  enabled: true
  baseUrl: http://192.168.1.20:8123
  token: YOUR_HOME_ASSISTANT_LONG_LIVED_TOKEN
  actions:
    - name: 客厅灯
      domain: light
      service: toggle
      entityId: light.living_room

weather:
  enabled: true
  provider: amap
  apiKey: YOUR_AMAP_KEY
  cityAdcode: "110101"
  refreshMinutes: 30

wifi:
  enabled: true
  interface: wlan0
  ssid: ""
  password: ""

screenPower:
  enabled: true
  blankStart: "23:00"
  blankEnd: "07:00"
  temporaryWakeMinutes: 5
  panelConfig: auto
  idleTimeoutSeconds: 180

xiaozhi:
  enabled: true
  applicationName: xiaozhi_linux_100ask
  applicationVersion: 1.0.0
  authToken: YOUR_AUTH_TOKEN
  autoListenOnConnect: false
  autoStartAudio: true
  boardName: rk3506_home_gui
  boardType: rk3506_linux_board
  configPath: /etc/xiaozhi.cfg
  language: zh-CN
  otaUrl: https://api.tenclass.net/xiaozhi/ota/
  soundAppPath: xiaozhi_sound_app
  userAgent: weidongshan1
  wakeGpioActiveLow: false
  wakeGpioDebounceMs: 80
  wakeGpioLine: -1
  websocketHost: api.tenclass.net
  websocketPath: /xiaozhi/v1/
  websocketPort: 443
```

## 3. Platform

路径：

- `platform.chip`
- `platform.width`
- `platform.height`
- `platform.renderMode`
- `platform.reducedEffects`

说明：

- `chip`
  主要用于平台标识和部分默认行为判断。
- `width` / `height`
  给界面布局和平台状态展示使用。
- `renderMode`
  使用 EGL/GPU 渲染时设为 `eglfs`（RK3568 推荐）；仅在没有可用 GPU/EGL 的设备上使用 `linuxfb`。Liquid Glass 的 ShaderEffect 需要 Qt Quick 图形渲染后端。
- `reducedEffects`
  控制是否降低动画和视觉效果。

示例：

```yaml
platform:
  chip: RK3568
  width: 800
  height: 480
  renderMode: eglfs
  reducedEffects: false
```

## 4. Cameras

路径：

- `cameras[]`
  每个元素代表一路摄像头。

推荐字段：

- `enabled`
- `renderer`
- `url`
- `backend`
- `decoder`
- `onvifProfile`
- `cameraName`
- `username`
- `password`

说明：

- `renderer`
  当前固定使用 `video`。
- `backend`
  支持 `auto`、`libav`、`gstreamer`；默认 `auto` 会落到 `libav`。
- `decoder`
  支持 `auto`、`software`、`hardware`。`libav` 当前只提供软解；RK 硬解扩展走 `gstreamer`。
- `onvifProfile`
  支持 `auto`、`main`、`minor`。为了适配嵌入式平台性能，`auto` 模式下默认优先选择辅码流（分辨率最小的）。摄像头界面切换主/辅码流时会更新该字段。
- `cameraName`
  直接作为界面展示名，不走翻译系统；需要中文显示时请直接写中文。

### 4.1 `url` conventions

支持这几种写法：

```yaml
"url": "rtsp://192.168.1.100:554/Streaming/Channels/101"
"url": "rtsps://192.168.1.100:322/stream"
"url": "onvif://192.168.1.100:2020"
"url": "onvif://192.168.1.100:2020/onvif/device_service"
```

行为：

- `rtsp://` / `rtsps://`
  直接作为视频流地址使用。
- `onvif://`
  程序会自动执行：
  1. `GetCapabilities`
  2. `GetProfiles`
  3. `GetStreamUri`
  然后自动拿到真实 RTSP 地址。

### 4.2 Camera examples

ONVIF 自动探测：

```yaml
{
  "enabled": true,
  "renderer": "video",
  "url": "onvif://192.168.1.100:2020",
  "backend": "libav",
  "decoder": "auto",
  "onvifProfile": "auto",
  "cameraName": "前门摄像头",
  "username": "admin",
  "password": "YOUR_PASSWORD"
}
```

RTSP 直连：

```yaml
{
  "enabled": true,
  "renderer": "video",
  "url": "rtsp://192.168.1.102:554/live/main",
  "backend": "libav",
  "decoder": "auto",
  "cameraName": "仓库摄像头",
  "username": "admin",
  "password": "YOUR_PASSWORD"
}
```

GStreamer 后端：

```yaml
{
  "enabled": true,
  "renderer": "video",
  "url": "rtsp://192.168.1.102:554/live/main",
  "backend": "gstreamer",
  "decoder": "auto",
  "cameraName": "硬解测试摄像头",
  "username": "admin",
  "password": "YOUR_PASSWORD"
}
```

### 4.3 Removed camera fields

以下字段已经不建议再写：

- `host`
- `onvifPort`
- `rtspUrl`
- `onvifDiscovery`
- `profileToken`
- `deviceServiceUrl`
- `mediaServiceUrl`
- `ptzServiceUrl`
- `ptzEnabled`

原因：

- `host/onvifPort/rtspUrl` 已收口为统一的 `url`
- 其余字段本质都是运行时探测结果，不应该让用户维护
- PTZ 能力由 ONVIF 自动探测，界面只在可用时显示 PTZ 控件

### 4.4 Compatibility

当前代码仍兼容旧摄像头配置：

- 如果没有 `url`，会先尝试旧的 `rtspUrl`
- 如果 `rtspUrl` 也没有，会用旧的 `host + onvifPort` 自动拼成 `onvif://host:port`

这只是迁移兜底，后续建议统一改成 `url`。

## 5. Home Assistant

路径：

- `homeAssistant.enabled`
- `homeAssistant.host`
- `homeAssistant.port`
- `homeAssistant.baseUrl`
- `homeAssistant.token`
- `homeAssistant.actions[]`

动作字段：

- `name`
- `domain`
- `service`
- `entityId`
- `data`

说明：

- 如果配置了 `baseUrl`，优先用它。
- 如果 `baseUrl` 为空，则回退为 `http://host:port`。
- `token` 需要使用 HA 的 long-lived access token。

示例：

```yaml
"homeAssistant": {
  "enabled": true,
  "baseUrl": "http://192.168.1.20:8123",
  "token": "YOUR_TOKEN",
  "actions": [
    {
      "name": "客厅灯",
      "domain": "light",
      "service": "toggle",
      "entityId": "light.living_room"
    },
    {
      "name": "夜间场景",
      "domain": "scene",
      "service": "turn_on",
      "entityId": "scene.night"
    }
  ]
}
```

## 6. Weather

路径：

- `weather.enabled`
- `weather.provider`
- `weather.apiKey`
- `weather.cityAdcode`
- `weather.refreshMinutes`

说明：

- 当前默认按高德天气的配置格式使用。
- `refreshMinutes` 控制轮询间隔。

示例：

```yaml
"weather": {
  "enabled": true,
  "provider": "amap",
  "apiKey": "YOUR_AMAP_KEY",
  "cityAdcode": "110101",
  "refreshMinutes": 30
}
```

## 7. Wi-Fi

路径：

- `wifi.enabled`
- `wifi.interface`
- `wifi.ssid`
- `wifi.password`

说明：

- `interface`
  可写死，例如 `wlan0`；为空时程序会自动探测无线网卡。
- `ssid` / `password`
  用于启动时主动连接指定 Wi‑Fi。

启动策略：

- 如果 `ssid` 非空，优先连接配置指定的网络。
- 如果 `ssid` 为空，恢复上次保存的网络。

内部自动探测：

- `wpa_supplicant` ctrl path
- `wpa_supplicant.conf` 路径

示例：

固定连接某个 Wi‑Fi：

```yaml
"wifi": {
  "enabled": true,
  "interface": "wlan0",
  "ssid": "OfficeWiFi",
  "password": "12345678"
}
```

恢复上次连接：

```yaml
"wifi": {
  "enabled": true,
  "interface": "wlan0",
  "ssid": "",
  "password": ""
}
```

### 7.1 Removed Wi-Fi fields

以下字段已经不建议再写：

- `ctrlPath`
- `configPath`

原因：

- 这些路径属于板级运行环境差异，应由程序自动识别，而不是交给业务配置维护。

## 8. Xiaozhi (AI Voice Assistant)

路径：

- `xiaozhi.enabled`
- `xiaozhi.applicationName`
- `xiaozhi.applicationVersion`
- `xiaozhi.authToken`
- `xiaozhi.autoListenOnConnect`
- `xiaozhi.autoStartAudio`
- `xiaozhi.boardName`
- `xiaozhi.boardType`
- `xiaozhi.configPath`
- `xiaozhi.language`
- `xiaozhi.otaUrl`
- `xiaozhi.soundAppPath`
- `xiaozhi.userAgent`
- `xiaozhi.wakeGpioActiveLow`
- `xiaozhi.wakeGpioDebounceMs`
- `xiaozhi.wakeGpioLine`
- `xiaozhi.websocketHost`
- `xiaozhi.websocketPath`
- `xiaozhi.websocketPort`

说明：

- `enabled`
  小智 AI 语音助手主开关。设为 `true` 开启，`false` 关闭。关闭时不会启动音频子进程、不占用本地 UDP 端口，同时隐藏 UI 上的小智悬浮按钮。
- `autoStartAudio`
  是否自动拉起本地音频服务子进程（`soundAppPath`）。
- `authToken`
  小智云端服务授权凭证。
- `websocketHost` / `websocketPort` / `websocketPath`
  小智服务端 WebSocket 通信地址。

示例：

```yaml
"xiaozhi": {
  "enabled": true,
  "applicationName": "xiaozhi_linux_100ask",
  "applicationVersion": "1.0.0",
  "authToken": "YOUR_AUTH_TOKEN",
  "autoListenOnConnect": false,
  "autoStartAudio": true,
  "boardName": "rk3506_home_gui",
  "boardType": "rk3506_linux_board",
  "configPath": "/etc/xiaozhi.cfg",
  "language": "zh-CN",
  "otaUrl": "https://api.tenclass.net/xiaozhi/ota/",
  "soundAppPath": "xiaozhi_sound_app",
  "userAgent": "weidongshan1",
  "wakeGpioActiveLow": false,
  "wakeGpioDebounceMs": 80,
  "wakeGpioLine": -1,
  "websocketHost": "api.tenclass.net",
  "websocketPath": "/xiaozhi/v1/",
  "websocketPort": 443
}
```

## 9. Practical Recommendations

- 新项目直接按 `url` 写摄像头，不要再混用旧字段。
- 摄像头显示名、HA 动作名等配置文案直接写中文；翻译系统只处理代码里的 `qsTr()` / `tr()` 文案。
- 默认用 `backend: "libav"`；需要验证 RK 硬解时再切到 `backend: "gstreamer"`。
- 不要把自动探测结果写回配置文件。
- 不要提交真实密码、Token、API Key。
- 发布前把示例配置中的敏感信息替换成占位值。

## 10. Localization

中文翻译文件：

- `i18n/home_gui_zh_CN.ts`

资源入口：

- `resources/qml.qrc`

维护流程：

```bash
lupdate -no-obsolete src resources/qml.qrc -ts i18n/home_gui_zh_CN.ts
rg -n 'type="unfinished"|vanished' i18n/home_gui_zh_CN.ts
```

要求：

- 新增 QML 文案必须使用 `qsTr()`。
- 新增 C++ 状态文案必须使用 `tr()`。
- 提交前 `home_gui_zh_CN.ts` 不能包含 `unfinished` 或 `vanished`。
- 当前程序直接加载 `.ts`，不需要生成 `.qm`。
