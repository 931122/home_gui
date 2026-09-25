# iOS 平台适配与编译指南

本项目已完成对 iOS (iPhone / iPad) 的全套构建与界面适配。

## 目录结构

- `ios/Info.plist.in`: iOS 应用程序配置模板，包含本地网络权限（`NSLocalNetworkUsageDescription`）、Bonjour 发现服务（Home Assistant）、屏幕旋转、全屏及文件共享配置。
- `ios/LaunchScreen.storyboard`: 深色无缝启动屏幕，与应用主题色 `#0E1724` 一致。
- `ios/Assets.xcassets/`: iOS 标准资源目录（包含 App 图标等）。

## 环境要求

1. **macOS** 系统，安装 **Xcode 14+** 及 Command Line Tools (`xcode-select --install`)。
2. **Qt 6 for iOS**（Qt 6.2 及以上版本，支持 iOS arm64 / 模拟器架构）：
   - 可通过 Qt 官方在线安装程序（Qt Online Installer）勾选安装 `Qt 6.x -> iOS`；
   - 默认安装路径通常为 `~/Qt/6.x.x/ios`。

## 编译方法

### 方法一：使用项目统一构建脚本（推荐）

```bash
# 自动检测本地环境并配置编译 iOS 模拟器/真机
./scripts/build.sh ios

# 指定架构或目标类型
./scripts/build.sh ios simulator    # 针对 iOS 模拟器
./scripts/build.sh ios device       # 针对 iPhone/iPad 真机

# 如果 Qt 安装在自定义路径，可传入环境变量：
QT_IOS_ROOT=/path/to/Qt/6.6.3/ios ./scripts/build.sh ios
```

### 方法二：使用 CMake 直接生成 Xcode 工程

```bash
# 1. 查找并使用 Qt iOS 的 qt-cmake 工具
~/Qt/6.6.3/ios/bin/qt-cmake -S . -B build-ios -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0

# 2. 直接在 Xcode 中打开工程进行签名、调试与安装
open build-ios/home_gui.xcodeproj
```

在 Xcode 中：
1. 选中 `home_gui` Target -> **Signing & Capabilities**；
2. 勾选 **Automatically manage signing**，选择您的 Apple 开发者 Team；
3. 选择目标设备（真机或 Simulator），点击 **Run (Cmd+R)** 即可。

## 本地网络与配置文件说明

1. **配置文件加载方式**：
   - **方式一（系统文件共享导入）**：开启了 `UIFileSharingEnabled`，应用安装后可以在 Mac Finder / iTunes 或 iOS 自带的“文件”App -> `我的 iPhone` -> `智能中控` 目录下直接放入或替换 `config.yaml`。
   - **方式二（应用内浏览选择）**：在应用右侧栏点击“系统设置”，通过“选择配置...”从文件系统中选择任意 YAML 配置文件，或在检测到的候选配置中一键切换。
   - **配置记忆与清除**：选中的配置文件会自动保存并持久化生效；在设置中点击“清除”可重置为空配置。应用安装包默认不内置任何配置，保证私密数据安全。
2. **本地网络权限**：
   - iOS 14+ 访问局域网 IP（如连接 Home Assistant、局域网 RTSP/ONVIF 摄像头）需要本地网络授权，初次启动时系统会自动弹出权限授权对话框，请点击“允许”。
