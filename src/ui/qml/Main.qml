import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

ApplicationWindow {
    id: root
    width: 800
    height: 480
    visible: true
    title: qsTr("Embedded Qt Control System")
    color: "#0e1724"

    // 自适应响应式基准系统：
    // 横屏以 800x480 为基准；竖屏以 480x800 为基准，完美适配手机、平板、RK3506 及桌面
    readonly property bool isAndroidPlatform: (typeof appController !== "undefined" && appController.isAndroid) || Qt.platform.os === "android"
    readonly property bool isPortrait: width < height
    readonly property real designWidth: isPortrait ? 480 : 800
    readonly property real designHeight: isPortrait ? 800 : 480
    readonly property real sx: width / designWidth
    readonly property real sy: height / designHeight
    readonly property real su: Math.max(0.65, isAndroidPlatform ? Math.max(Math.min(sx, sy), (sx * 0.15 + sy * 0.85)) : Math.min(sx, sy))
    readonly property int chipRadius: dp(10)
    readonly property int cardRadius: dp(14)
    readonly property int panelRadius: dp(18)
    property bool videoFullscreen: false
    readonly property real landscapeTopBarHeight: root.dp(44)
    readonly property real landscapeStatusDockHeight: root.dp(86)
    readonly property real landscapeItemSpacing: root.dp(8)
    readonly property real landscapeMargin: root.dp(10)

    // 横屏下可用垂直净高度
    readonly property real landscapeAvailableHeight: Math.max(root.dp(200), (height - landscapeMargin * 2))

    // 横屏下视频卡片的黄金高度（填满中间所有垂直剩余空间）
    readonly property real landscapeVideoCardHeight: Math.max(root.dp(160), 
        landscapeAvailableHeight - landscapeTopBarHeight - landscapeStatusDockHeight - (landscapeItemSpacing * 2))

    // 核心黄金法则：视控区统一宽度由 16:9 比例主导，并约束在总宽度的 50% ~ 68% 之间
    readonly property real landscapeVisionWidth: {
        if (isPortrait) return width - root.dp(20)
        var idealW = Math.round(landscapeVideoCardHeight * (16 / 9))
        var availTotalW = width - (landscapeMargin * 2) - landscapeItemSpacing
        var maxW = Math.round(availTotalW * 0.68)
        var minW = Math.round(availTotalW * 0.50)
        return Math.min(maxW, Math.max(minW, idealW))
    }

    FontLoader {
        id: emojiLoader
        source: "qrc:/fonts/Symbola.ttf"
    }
    readonly property string emojiFontFamily: (emojiLoader.status === FontLoader.Ready && emojiLoader.name.length > 0) ? emojiLoader.name : "Symbola"

    Component.onCompleted: Theme.scaleUnit = root.su
    onSuChanged: Theme.scaleUnit = root.su

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }
    readonly property bool hasOpenPopup: (cookerPopup.opened) ||
                                         (washerPopup.opened) ||
                                         (steamerPopup.opened) ||
                                         (calendarLoader.status === Loader.Ready && calendarLoader.item && calendarLoader.item.opened) ||
                                         (weatherLoader.status === Loader.Ready && weatherLoader.item && weatherLoader.item.opened) ||
                                         (cameraLoader.status === Loader.Ready && cameraLoader.item && cameraLoader.item.opened) ||
                                         (settingsLoader.status === Loader.Ready && settingsLoader.item && settingsLoader.item.opened) ||
                                         (wifiLoader.status === Loader.Ready && wifiLoader.item && wifiLoader.item.anyPopupOpened) ||
                                         (haSidebar.moreDevicesOpened)

    function closeTopPopup() {
        if (haSidebar.moreDevicesOpened) {
            haSidebar.closeMoreDevices()
            return true
        }
        if (wifiLoader.status === Loader.Ready && wifiLoader.item) {
            if (wifiLoader.item.closeTopPopup && wifiLoader.item.closeTopPopup()) {
                return true
            }
        }
        if (cookerPopup.opened) {
            if (cookerPopup.showRecipeDetail) {
                cookerPopup.showRecipeDetail = false
                return true
            }
            if (cookerPopup.currentView !== "main") {
                cookerPopup.currentView = "main"
                return true
            }
            cookerPopup.close()
            return true
        }
        if (washerPopup.opened) {
            washerPopup.close()
            return true
        }
        if (steamerPopup.opened) {
            steamerPopup.close()
            return true
        }
        if (settingsLoader.status === Loader.Ready && settingsLoader.item && settingsLoader.item.opened) {
            settingsLoader.item.close()
            return true
        }
        if (cameraLoader.status === Loader.Ready && cameraLoader.item && cameraLoader.item.opened) {
            cameraLoader.item.close()
            return true
        }
        if (weatherLoader.status === Loader.Ready && weatherLoader.item && weatherLoader.item.opened) {
            weatherLoader.item.close()
            return true
        }
        if (calendarLoader.status === Loader.Ready && calendarLoader.item && calendarLoader.item.opened) {
            calendarLoader.item.close()
            return true
        }
        if (root.videoFullscreen) {
            root.videoFullscreen = false
            return true
        }
        return false
    }

    Shortcut {
        sequence: "Back"
        onActivated: root.closeTopPopup()
    }
    Shortcut {
        sequence: "Escape"
        onActivated: root.closeTopPopup()
    }

    function openCalendarPopup() {
        calendarLoader.active = true
        if (calendarLoader.status === Loader.Ready) {
            calendarLoader.item.openForToday()
        }
    }
    function openWeatherPopup() {
        weatherLoader.active = true
        if (weatherLoader.status === Loader.Ready) {
            weatherLoader.item.open()
        }
    }
    function openCameraPopup() {
        root.videoFullscreen = false
        cameraLoader.active = true
        if (cameraLoader.status === Loader.Ready) {
            cameraLoader.item.open()
        }
    }
    function openSettingsPopup() {
        settingsLoader.active = true
        if (settingsLoader.status === Loader.Ready && settingsLoader.item) {
            settingsLoader.item.open()
        }
    }

    // 主屏幕完整场景层（背景 + 顶栏 + 实时监控视频 + 智能家居侧边栏）
    Item {
        id: mainSceneLayer
        anchors.fill: parent

        // 背景层容器：现代苹果通透深空极光星云（明澈清朗，提供极高对比度折射色彩基底）
        Item {
            id: ambientBackdrop
            anchors.fill: parent

            // 1. 基底流体光幕（明澈高级的深空宝石蓝与黛青渐变：彻底消除暗沉死黑）
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#2e4a73" }
                    GradientStop { position: 0.35; color: "#213654" }
                    GradientStop { position: 0.75; color: "#182840" }
                    GradientStop { position: 1.0; color: "#121e30" }
                }
            }

            // 2. 左上方极光：明亮电光冰川蔚蓝（为左侧视频监控卡片与顶栏提供通透冷光与多重折射）
            Rectangle {
                id: auroraBlue
                x: -root.dp(30)
                y: -root.dp(30)
                width: Math.min(root.width * 0.85, root.dp(480))
                height: width
                radius: width / 2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0.22, 0.68, 1.0, 0.58) }
                    GradientStop { position: 0.35; color: Qt.rgba(0.14, 0.50, 0.90, 0.32) }
                    GradientStop { position: 0.70; color: Qt.rgba(0.08, 0.30, 0.65, 0.10) }
                    GradientStop { position: 1.0; color: "transparent" }
                }

                // 微流体轻柔呼吸漂移动画
                SequentialAnimation on x {
                    loops: Animation.Infinite
                    NumberAnimation { to: -root.dp(15); duration: 7000; easing.type: Easing.InOutSine }
                    NumberAnimation { to: -root.dp(45); duration: 7000; easing.type: Easing.InOutSine }
                }
                SequentialAnimation on y {
                    loops: Animation.Infinite
                    NumberAnimation { to: -root.dp(45); duration: 8500; easing.type: Easing.InOutSine }
                    NumberAnimation { to: -root.dp(15); duration: 8500; easing.type: Easing.InOutSine }
                }
            }

            // 3. 右上方侧边栏正后方：纯净天青与翡翠碧波流光（为右侧设备卡片液态玻璃提供通透的光学折射高光）
            Rectangle {
                id: auroraCyan
                x: root.width - width + root.dp(50)
                y: -root.dp(30)
                width: Math.min(root.width * 0.80, root.dp(450))
                height: width
                radius: width / 2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0.12, 0.84, 0.88, 0.52) }
                    GradientStop { position: 0.38; color: Qt.rgba(0.08, 0.58, 0.72, 0.28) }
                    GradientStop { position: 0.72; color: Qt.rgba(0.05, 0.35, 0.52, 0.08) }
                    GradientStop { position: 1.0; color: "transparent" }
                }

                SequentialAnimation on y {
                    loops: Animation.Infinite
                    NumberAnimation { to: -root.dp(10); duration: 9000; easing.type: Easing.InOutSine }
                    NumberAnimation { to: -root.dp(40); duration: 9000; easing.type: Easing.InOutSine }
                }
            }

            // 4. 右侧偏下侧边栏后方：幻彩霞光薰衣草梦幻紫（冷暖交汇，让下方设备卡片折射出 visionOS 般的梦幻紫霞）
            Rectangle {
                id: auroraPurple
                x: root.width - width * 0.85
                y: root.height * 0.45
                width: Math.min(root.width * 0.65, root.dp(360))
                height: width
                radius: width / 2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0.70, 0.36, 0.95, 0.42) }
                    GradientStop { position: 0.40; color: Qt.rgba(0.48, 0.22, 0.78, 0.22) }
                    GradientStop { position: 0.72; color: Qt.rgba(0.25, 0.12, 0.48, 0.06) }
                    GradientStop { position: 1.0; color: "transparent" }
                }

                SequentialAnimation on x {
                    loops: Animation.Infinite
                    NumberAnimation { to: root.width - width * 0.78; duration: 8000; easing.type: Easing.InOutSine }
                    NumberAnimation { to: root.width - width * 0.92; duration: 8000; easing.type: Easing.InOutSine }
                }
            }

            // 5. 中下方至右下方：明澈晨曦微温琥珀金光（提供高对比度色温与晶莹折射边缘）
            Rectangle {
                id: auroraAmber
                x: root.width * 0.25
                y: root.height - height + root.dp(50)
                width: Math.min(root.width * 0.75, root.dp(420))
                height: width * 0.68
                radius: width / 2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(1.0, 0.68, 0.28, 0.35) }
                    GradientStop { position: 0.36; color: Qt.rgba(0.88, 0.46, 0.32, 0.18) }
                    GradientStop { position: 0.70; color: Qt.rgba(0.45, 0.22, 0.40, 0.05) }
                    GradientStop { position: 1.0; color: "transparent" }
                }

                SequentialAnimation on y {
                    loops: Animation.Infinite
                    NumberAnimation { to: root.height - height + root.dp(30); duration: 7500; easing.type: Easing.InOutSine }
                    NumberAnimation { to: root.height - height + root.dp(60); duration: 7500; easing.type: Easing.InOutSine }
                }
            }
        }

        // 主体响应式自适应容器（横屏为“左视控+右边栏”；竖屏为“顶栏+中视频+底设备栏全宽滑动”）
        Item {
            id: mainContainer
            anchors.fill: parent
            anchors.margins: root.videoFullscreen ? 0 : root.dp(10)
            focus: true
            Keys.onBackPressed: {
                if (root.closeTopPopup()) {
                    event.accepted = true
                }
            }
            Keys.onEscapePressed: {
                if (root.closeTopPopup()) {
                    event.accepted = true
                }
            }

            // 顶部栏：摄像头切换、系统设置、天气、时间
            TopBar {
                id: topBar
                visible: !root.videoFullscreen
                anchors.top: parent.top
                anchors.left: parent.left
                width: root.isPortrait ? parent.width : root.landscapeVisionWidth
                height: root.isPortrait ? root.dp(52) : root.landscapeTopBarHeight
                scaleUnit: root.su
                panelRadius: root.panelRadius
                chipRadius: root.chipRadius
                onCameraClicked: root.openCameraPopup()
                onSettingsClicked: root.openSettingsPopup()
                onWeatherClicked: root.openWeatherPopup()
                onCalendarClicked: root.openCalendarPopup()
            }

            // 视频监控核心卡片（大幅放大，严格 16:9 黄金比例）
            VideoPanel {
                id: mainVideoCard
                anchors.top: root.videoFullscreen ? parent.top : topBar.bottom
                anchors.topMargin: root.videoFullscreen ? 0 : root.dp(8)
                anchors.left: parent.left
                anchors.right: (root.videoFullscreen || root.isPortrait) ? parent.right : undefined
                width: (root.videoFullscreen || root.isPortrait) ? undefined : root.landscapeVisionWidth
                anchors.bottom: root.videoFullscreen ? parent.bottom : undefined
                height: {
                    if (root.videoFullscreen) return undefined
                    if (root.isPortrait) return Math.min(Math.round(parent.height * 0.36), Math.round(parent.width * 9 / 16 + root.dp(48)))
                    return Math.round(root.landscapeVisionWidth * 9 / 16)
                }
                radius: root.videoFullscreen ? 0 : root.cardRadius
                border.width: root.videoFullscreen ? 0 : 1
                scaleUnit: root.su
                panelRadius: root.panelRadius
                cardRadius: root.cardRadius
                fullscreen: root.videoFullscreen
                z: root.videoFullscreen ? 1000 : 0
                onFullscreenToggleRequested: {
                    if (cameraLoader.status === Loader.Ready && cameraLoader.item.opened) {
                        cameraLoader.item.close()
                    }
                    root.videoFullscreen = !root.videoFullscreen
                }
                onCameraSwipeRequested: appController.selectRelativeCamera(offset)
            }

            // 视频下方全屋状态卡片（显示全屋灯光开启情况 + 浴霸暖风 + 空气净化器）
            VideoBottomStatusCard {
                id: videoBottomStatus
                visible: !root.videoFullscreen && !root.isPortrait
                anchors.top: mainVideoCard.bottom
                anchors.topMargin: root.dp(8)
                anchors.bottom: parent.bottom
                anchors.left: mainVideoCard.left
                anchors.right: mainVideoCard.right
                scaleUnit: root.su
                panelRadius: root.panelRadius
                cardRadius: root.cardRadius
                onFullscreenRequested: {
                    if (cameraLoader.status === Loader.Ready && cameraLoader.item.opened) {
                        cameraLoader.item.close()
                    }
                    root.videoFullscreen = true
                }
            }

            // 智能家居控制面板（自适应填满右侧剩余宽度，宽屏下自动启用 2 列核心卡片，无多余滚动）
            HaSidebar {
                id: haSidebar
                visible: !root.videoFullscreen
                anchors.top: root.isPortrait ? mainVideoCard.bottom : parent.top
                anchors.topMargin: root.isPortrait ? root.dp(10) : 0
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.left: root.isPortrait ? parent.left : mainVideoCard.right
                anchors.leftMargin: root.isPortrait ? 0 : root.dp(10)
                scaleUnit: root.su
                panelRadius: root.panelRadius
                cardRadius: root.cardRadius
                backgroundSource: ambientBackdrop
                onCookerRequested: function(actionModel) {
                    cookerPopup.openWithAction(actionModel)
                }
                onWasherRequested: function(actionModel) {
                    washerPopup.openWithAction(actionModel)
                }
                onSteamerRequested: function(actionModel) {
                    steamerPopup.openWithAction(actionModel)
                }
            }
        }
    }

    // 苹果级高斯毛玻璃动态虚化层（Qt 6 原生 RHI 硬件加速）
    FrostedGlassOverlay {
        id: frostedGlassOverlay
        anchors.fill: parent
        sourceItem: mainSceneLayer
        activeBlur: root.hasOpenPopup
        z: 90
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 1.0 - appController.brightness
        visible: !appController.hardwareBrightnessAvailable && opacity > 0.01
        z: 98
    }

    XiaozhiOverlay {
        anchors.fill: parent
        scaleUnit: root.su
        panelRadius: root.panelRadius
        z: 1200
    }

    Loader {
        id: calendarLoader
        active: false
        source: "CalendarPopup.qml"
        onLoaded: {
            item.scaleUnit = root.su
            item.panelRadius = root.panelRadius
            item.closed.connect(function() { root.forceActiveFocus() })
            item.openForToday()
        }
    }

    Loader {
        id: weatherLoader
        active: false
        source: "WeatherPopup.qml"
        onLoaded: {
            item.scaleUnit = root.su
            item.panelRadius = root.panelRadius
            item.cardRadius = root.cardRadius
            item.closed.connect(function() { root.forceActiveFocus() })
            item.open()
        }
    }

    Loader {
        id: cameraLoader
        active: false
        source: "CameraPopup.qml"
        onLoaded: {
            item.scaleUnit = root.su
            item.panelRadius = root.panelRadius
            item.cardRadius = root.cardRadius
            item.chipRadius = root.chipRadius
            item.closed.connect(function() { root.forceActiveFocus() })
            root.videoFullscreen = false
            item.open()
        }
    }

    Loader {
        id: settingsLoader
        active: false
        source: "SettingsPopup.qml"
        property bool signalConnected: false
        onLoaded: {
            if (item) {
                item.scaleUnit = root.su
                item.panelRadius = root.panelRadius
                item.cardRadius = root.cardRadius
                item.chipRadius = root.chipRadius
                item.closed.connect(function() { root.forceActiveFocus() })
                if (!signalConnected && item.wifiRequested) {
                    item.wifiRequested.connect(wifiLoader.openWifi)
                    signalConnected = true
                }
                item.open()
            }
        }
    }

    Loader {
        id: wifiLoader
        active: false
        source: "WifiPopups.qml"
        function openWifi() {
            active = true
            if (status === Loader.Ready) item.openWifiPopup()
        }
        onLoaded: {
            item.scaleUnit = root.su
            item.panelRadius = root.panelRadius
            item.cardRadius = root.cardRadius
            item.chipRadius = root.chipRadius
            item.openWifiPopup()
        }
    }

    CookerPopup {
        id: cookerPopup
        scaleUnit: root.su
        panelRadius: root.panelRadius
        cardRadius: root.cardRadius
        chipRadius: root.chipRadius
        onClosed: root.forceActiveFocus()
    }

    WasherPopup {
        id: washerPopup
        scaleUnit: root.su
        panelRadius: root.panelRadius
        cardRadius: root.cardRadius
        chipRadius: root.chipRadius
        onClosed: root.forceActiveFocus()
    }

    SteamerPopup {
        id: steamerPopup
        scaleUnit: root.su
        panelRadius: root.panelRadius
        cardRadius: root.cardRadius
        chipRadius: root.chipRadius
        onClosed: root.forceActiveFocus()
    }

    // 灭屏防误触与轻触唤醒层 (Tap to Wake)
    TapToWakeOverlay {
        id: tapToWakeOverlay
    }

    // Android 启动屏过渡层：与 Linux BootSplash 保持完全一致的视觉排版与停留动效
    BootSplashView {
        id: bootSplashOverlay
        visible: ((typeof appController !== "undefined" && appController.isAndroid) || Qt.platform.os === "android") && !finished
    }

    // 全局 Liquid Glass 2.0 悬浮高透微晶 Toast 胶囊
    LiquidGlassToast {
        id: globalToast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.dp(24)
        backgroundSource: mainSceneLayer
        z: 1100
    }

    function showToast(message, icon) {
        globalToast.show(message, icon)
    }
}
