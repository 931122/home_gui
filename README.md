# Project Guide - Embedded Qt Control System (Embedded Linux / Android / Desktop)

本文件定义了针对通用嵌入式 Linux（如 RK3568、RK3506、全志、NXP 等）、Android 移动端及桌面开发平台的项目架构与规范。在执行任务时必须严格遵守。

## 1. 常用命令

### 环境与编译

```bash
# 本地 Native 构建（自动检测 Linux / macOS）
bash scripts/build.sh native

# macOS 本地构建
bash scripts/build.sh macos

# 统一 Linux 交叉编译（使用单一通用工具链 cmake/toolchains/linux-cross.cmake）
# 方式 A：Buildroot SDK 模式（自动识别 32/64 位架构，如 RK3506、RK3568、全志等）
bash scripts/build.sh buildroot /path/to/buildroot/output
bash scripts/build.sh rk3506    # RK3506 预设快捷构建
bash scripts/build.sh rk3568    # RK3568 预设快捷构建

# 方式 B：通用独立工具链 / Yocto / Linaro 模式
CROSS_COMPILE_PREFIX=aarch64-linux-gnu- SYSROOT=/path/to/sysroot bash scripts/build.sh cross

# Android APK 一键打包（默认 arm64-v8a）
bash scripts/build.sh android
```

### Android 移动端编译说明

运行 `bash scripts/build.sh android` 可一键编译并生成 Release APK，脚本会自动处理 FFmpeg 交叉编译（带 MediaCodec 硬解补丁）、QSB Shader 编译以及 APK 签名打包。

#### 前置环境要求

- **Qt 6 for Android**：建议 Qt 6.6.3+（需包含 `android_arm64_v8a` 架构套件及同版本 Host `gcc_64`）。
- **Android SDK & NDK**：
  - Android SDK (API Level 31+)；
  - Android NDK (推荐 r25c 或 r21e)；
  - Build Tools (33.0.0+)；
  - Java JDK 17 (或 JDK 11)。
- **OpenSSL for Android**：推荐使用 [KDAB/android_openssl](https://github.com/KDAB/android_openssl)。

#### 环境变量配置（可选）

若工具链未安装在标准默认路径，可通过环境变量指定：

```bash
export QT_ANDROID_DIR="$HOME/Android/Qt/6.6.3/android_arm64_v8a"
export ANDROID_SDK_ROOT="$HOME/Android/Sdk"
export ANDROID_NDK_ROOT="$HOME/Android/ndk/android-ndk-r25c"
export ANDROID_OPENSSL_ROOT="$HOME/Android/android_openssl"
export JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"

# 执行一键打包
bash scripts/build.sh android
```

#### 产物位置与特性

- **输出路径**：`build-android/home_gui.apk`
- **特性支持**：
  - 自动适配手机竖屏与横屏模式（横屏下自动启用 Dock 工具栏扩展）；
  - 完整集成 Liquid Glass 2.0 动效与重力感应器倾斜高光联动；
  - 首次启动自动释放可写配置文件，支持热重载。


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

| 平台与硬件能力 | 渲染后端 (QPA) | 加速技术 | 建议 UI 框架 / 特效策略 |
| :--- | :--- | :--- | :--- |
| **GPU 硬件加速平台** (如 RK3568/RK3588/全志/i.MX8/树莓派) | `eglfs` / `wayland` | GPU (OpenGL ES 3.0+ / Vulkan) | Qt Quick / QML (默认开启全功能液态玻璃光影特效) |
| **纯软件光栅化平台** (如 RK3506 / 无 3D GPU 低算力板卡) | `linuxfb` | 2D 加速器 / CPU 纯软件光栅化 | Qt Quick / QML (自动开启 `reducedEffects` 降级特效保流畅度) |
| **Android 移动平台** | `android` | Adreno / Mali GPU (MediaCodec 硬解) | Qt Quick / QML (支持重力陀螺仪倾斜高光联动) |
| **桌面开发环境** (Linux / macOS / Windows) | `xcb` / `wayland` / `cocoa` / `windows` | Desktop OpenGL / Metal / DirectX | Qt Quick / QML (本地极速预览与开发调试) |

### 屏幕规范
* **基准分辨率**: 800 x 480 (横屏) / 480 x 800 (竖屏)，支持任意分辨率自适应拉伸与动态重布局。
* **交互设计**: 点击目标尺寸必须大于 40x40 像素（适配工业触摸屏）。
* **字体**: 统一使用开源字体（如 Source Han Sans），避免系统字体缺失导致的方框。

---

## 3. 模块化架构要求

项目必须采用 **插件化/模块化** 设计，严禁在 `mainwindow` 中堆砌代码：

* **Core (`/src/core`)**: 配置解析 (YAML)、全局状态管理、模块生命周期。
* **Video (`/src/modules/video`)**:
    - 集成 RTSP/ONVIF。
    - 视频后端使用可扩展的 `VideoBackend` 抽象，当前支持 `libav` 和 `gstreamer`。
    - `libav` 是默认后端；`gstreamer` 用于嵌入式 Linux 平台硬解扩展。
    - 不再通过外部 `ffmpeg` 命令拉流。
* **IoT (`/src/modules/homeassistant`)**:
    - 使用 `qmqtt` 库与 Home Assistant 通讯。
    - 采用订阅发布模式，解耦 UI 与通信逻辑。
* **UI (`/src/ui`)**:
    - UI 逻辑与业务逻辑通过 `Signal/Slot` 彻底分离。
    - 针对纯软件渲染（linuxfb 模式）或低性能设备，运行时自动启用特效降级，禁用复杂的 QML 实时阴影和多通道模糊。

---

## 4. 技术栈规范与约束

* **语言**: C++17, 采用现代标准 C++。
* **库依赖**:
    - Qt 6.x (Buildroot / Yocto / Desktop / Android)。
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
* **修改 UI 前**: 确认是否会引入 CPU 密集型的渲染操作，特别是在无 3D GPU 的纯软件渲染（linuxfb）环境下。
* **提交代码前**: 确保包含了必要的 `include` 保护和 Doxygen 风格的代码注释。

## 7. Liquid Glass UI

`LiquidGlassSurface.qml` provides SDF rounded geometry, broad convex-lens magnification, refraction and chromatic aberration, Regular/Clear materials, backdrop blur and saturation, edge highlights, adaptive tint, press response, metaball merging and progressive edge blur. `GlassRuntime` supplies Android gravity-sensor tilt, sampled backdrop luminance, high-contrast and reduce-motion settings, and battery-saver state. Reusable controls include `LiquidGlassButton`, `LiquidGlassFab`, `LiquidGlassTabBar`, `LiquidGlassTabLayout`, `LiquidGlassChip`, `LiquidGlassChipGroup`, `LiquidGlassListItem`, `LiquidGlassListGroup`, `LiquidGlassToast` and `LiquidGlassDialog`.

These surfaces require the Qt Quick scene graph with a GPU-backed render loop. Use `eglfs` / `wayland` on platforms with a 3D GPU (e.g. RK3568, i.MX8); on platforms without a GPU (e.g. RK3506 running `linuxfb`), the runtime automatically switches to `reducedEffects: true`. `platform.reducedEffects` remains an explicit flat/high-contrast fallback. See [Qt ShaderEffect](https://doc.qt.io/qt-6/qml-qtquick-shadereffect.html) and [Qt for Embedded Linux](https://doc.qt.io/qt-6/embedded-linux.html).
