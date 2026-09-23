# Project Guide - Embedded Qt Control System (RK3568/RK3506)

本文件定义了针对 Rockchip 嵌入式平台开发的项目规范。在执行任务时必须严格遵守。

## 1. 常用命令

### 环境与编译 (Buildroot/uClibc)

```bash
cmake -S . -B build && cmake --build build
bash scripts/build.sh macos
bash scripts/build.sh rk3506
```

### 运行与调试

```bash
./build/home_gui
```

### 中文语言资源

程序启动时会固定安装中文翻译器：

- Qt 内置控件翻译从 Qt 的 `qt_zh_CN` 资源加载。
- 应用文案从 `:/i18n/home_gui_zh_CN.ts` 加载。
- `resources/qml.qrc` 必须包含 `i18n/home_gui_zh_CN.ts`，否则运行时不会生效。

新增 `qsTr()` 或 `tr()` 文案后，执行：

```bash
lupdate -no-obsolete src resources/qml.qrc -ts i18n/home_gui_zh_CN.ts
```

然后补齐 `i18n/home_gui_zh_CN.ts` 中的中文翻译，确保没有 `type="unfinished"`。本项目当前直接加载 `.ts`，不依赖 `.qm`。

---

## 2. 硬件架构适配 (800x480),支持后续分辨率升级

项目必须支持在不同能力的芯片上运行，通过配置文件或宏切换渲染模式：

| 平台 | 渲染后端 (QPA) | 加速技术 | 建议 UI 框架 |
| :--- | :--- | :--- | :--- |
| **RK3568** | `eglfs` | Mali-G52 GPU (OpenGL ES 3.2) | Qt Quick / QML |
| **RK3506** | `linuxfb` | RGA 2D Accelerator |Qt Quick / QML |

### 屏幕规范
* **固定分辨率**: 800 x 480。(后续可能升级)
* **交互设计**: 点击目标尺寸必须大于 40x40 像素（适配工业触摸屏）。
* **字体**: 统一使用开源字体（如 Source Han Sans），避免系统字体缺失导致的方框。

---

## 3. 模块化架构要求

项目必须采用 **插件化/模块化** 设计，严禁在 `mainwindow` 中堆砌代码：

* **Core (`/src/core`)**: 配置解析 (YAML)、全局状态管理、模块生命周期。
* **Video (`/src/modules/video`)**:
    - 集成 RTSP/ONVIF。
    - 视频后端使用可扩展的 `VideoBackend` 抽象，当前支持 `libav` 和 `gstreamer`。
    - `libav` 是默认后端；`gstreamer` 用于 RK 平台硬解扩展。
    - 不再通过外部 `ffmpeg` 命令拉流。
* **IoT (`/src/modules/homeassistant`)**:
    - 使用 `qmqtt` 库与 Home Assistant 通讯。
    - 采用订阅发布模式，解耦 UI 与通信逻辑。
* **UI (`/src/ui`)**:
    - UI 逻辑与业务逻辑通过 `Signal/Slot` 彻底分离。
    - 针对 RK3506 运行时，自动禁用复杂的 QML 阴影和模糊特效。

---

## 4. 技术栈规范与约束

* **语言**: C++17, 采用现代标准 C++。
* **库依赖**:
    - Qt 6.x (Buildroot uClibc 环境)。
    - **fkYAML**: 用于模块化配置文件。
    - **Qt Network/WebSockets**: 用于 HA 连接。
* **内存管理**:
    - 嵌入式环境内存有限，必须检查 `QObject` 的父子关系以防内存泄漏。
    - 频繁刷新的图像数据必须使用 `QSharedPointer` 或直接操作内存池。
* **配置化**:
    - 所有模块的开关和参数（如 RTSP 地址、HA 实体 ID）必须从 `config.yaml` 读取。
    - 支持热加载配置。
    - ONVIF PTZ 能力、Profile、Service URL 等运行期探测结果不要写入配置。

---

## 5. 编码风格规范

* **命名**: 类名 `PascalCase`，函数/变量 `camelCase`，成员变量 `m_variableName`。
* **异步操作**: 网络通信、ONVIF 设备发现、视频流初始化必须在独立线程执行，禁止阻塞 GUI 线程。
* **错误处理**: 所有的硬件调用（如 RGA 转换、视频解码）必须有完善的返回值校验和 Log 输出。

---

## 6. 协作指令

* **新增功能前**: 先分析 `src/core` 下的基类，确保新模块继承自项目定义的接口。
* **修改 UI 前**: 确认是否会引入 CPU 密集型的渲染操作，特别是在 RK3506 环境下。
* **提交代码前**: 确保包含了必要的 `include` 保护和 Doxygen 风格的代码注释。

## 7. Liquid Glass UI

`LiquidGlassSurface.qml` provides SDF rounded geometry, broad convex-lens magnification, refraction and chromatic aberration, Regular/Clear materials, backdrop blur and saturation, edge highlights, adaptive tint, press response, metaball merging and progressive edge blur. `GlassRuntime` supplies Android gravity-sensor tilt, sampled backdrop luminance, high-contrast and reduce-motion settings, and battery-saver state. Reusable controls include `LiquidGlassButton`, `LiquidGlassFab`, `LiquidGlassTabBar`, `LiquidGlassTabLayout`, `LiquidGlassChip`, `LiquidGlassChipGroup`, `LiquidGlassListItem`, `LiquidGlassListGroup`, `LiquidGlassToast` and `LiquidGlassDialog`.

These surfaces require the Qt Quick scene graph with a GPU-backed render loop. Use `eglfs` on RK3568; `linuxfb` has no Qt Quick shader backend, so select `reducedEffects: true` there. `platform.reducedEffects` remains an explicit flat/high-contrast fallback. See [Qt ShaderEffect](https://doc.qt.io/qt-6/qml-qtquick-shadereffect.html) and [Qt for Embedded Linux](https://doc.qt.io/qt-6/embedded-linux.html).
