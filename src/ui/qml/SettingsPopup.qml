import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0

GlassPopup {
    id: root
    title: qsTr("系统设置")

    signal wifiRequested()
    signal intercomLoginRequested()

    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height
    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.92, Theme.dp(isPortrait ? 440 : 420))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.92, Theme.dp(isPortrait ? 580 : 400))

    property real scaleUnit: Theme.scaleUnit
    property int cardRadius: Theme.radiusCard
    property int chipRadius: Theme.radiusChip
    readonly property bool isMobilePlatform: Theme.isMobilePlatform
    readonly property bool isAndroidPlatform: Theme.isAndroidPlatform
    readonly property bool isIosPlatform: Theme.isIosPlatform

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    FileDialog {
        id: configFileDialog
        title: qsTr("选择 HomeGui 配置文件 (YAML)")
        nameFilters: root.isMobilePlatform
                     ? [qsTr("所有文件 (*)")]
                     : [qsTr("YAML 配置文件 (*.yaml *.yml)"), qsTr("所有文件 (*)")]
        onAccepted: {
            if (typeof appController !== "undefined") {
                appController.loadConfigFile(selectedFile)
            }
        }
    }

    // 滚动区域 (支持当内容超出弹窗高度时上下平滑滚动)
    Flickable {
        id: scrollArea
        anchors.fill: parent
        anchors.leftMargin: Theme.dp(18)
        anchors.rightMargin: Theme.dp(18)
        anchors.bottomMargin: Theme.dp(16)
        contentWidth: width
        contentHeight: cardColumn.implicitHeight + Theme.dp(8)
        clip: true
        boundsBehavior: Flickable.DragAndOvershootBounds

        // 半透明优雅滚动条
        ScrollBar.vertical: ScrollBar {
            id: vbar
            active: scrollArea.moving || scrollArea.dragging
            policy: ScrollBar.AsNeeded
            visible: scrollArea.contentHeight > scrollArea.height

            contentItem: Rectangle {
                implicitWidth: root.dp(4)
                radius: width / 2
                color: Qt.rgba(1.0, 1.0, 1.0, vbar.active ? 0.50 : 0.22)
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        ColumnLayout {
            id: cardColumn
            width: scrollArea.width
            spacing: root.dp(12)

            // 配置文件选择与管理卡片
            Rectangle {
                Layout.fillWidth: true
                radius: root.cardRadius
                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.09)
                border.width: 1
                implicitHeight: configCol.implicitHeight + root.dp(28)
                clip: true

                ColumnLayout {
                    id: configCol
                    anchors.fill: parent
                    anchors.margins: root.dp(14)
                    spacing: root.dp(10)

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(10)

                        Rectangle {
                            width: root.dp(32)
                            height: root.dp(32)
                            radius: root.dp(8)
                            color: Qt.rgba(0.20, 0.60, 1.0, 0.20)
                            border.color: Qt.rgba(0.40, 0.75, 1.0, 0.35)
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "CFG"
                                color: "#38bdf8"
                                font.pixelSize: root.fs(10)
                                font.bold: true
                            }
                        }

                        Column {
                            Layout.fillWidth: true
                            spacing: root.dp(2)

                            Text {
                                text: qsTr("配置文件 (YAML)")
                                color: "#f0f6fa"
                                font.pixelSize: root.fs(15)
                                font.bold: true
                            }

                            Text {
                                text: (typeof appController !== "undefined" && appController.configStatusText)
                                      ? appController.configStatusText
                                      : qsTr("未选择配置文件")
                                color: (typeof appController !== "undefined" && appController.configLoaded) ? "#4ade80" : "#fbbf24"
                                font.pixelSize: root.fs(11)
                            }
                        }

                        RowLayout {
                            spacing: root.dp(6)

                            GlassButton {
                                scaleUnit: root.scaleUnit
                                styleType: "accent"
                                implicitWidth: root.dp(84)
                                implicitHeight: root.dp(28)
                                text: qsTr("选择配置...")
                                textPixelSize: root.fs(11)
                                onClicked: configFileDialog.open()
                            }

                            GlassButton {
                                visible: typeof appController !== "undefined" && appController.configFilePath.length > 0
                                scaleUnit: root.scaleUnit
                                styleType: "neutral"
                                implicitWidth: root.dp(56)
                                implicitHeight: root.dp(28)
                                text: qsTr("清除")
                                textPixelSize: root.fs(11)
                                onClicked: {
                                    if (typeof appController !== "undefined") {
                                        appController.resetConfigFile()
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: root.dp(30)
                        radius: root.dp(6)
                        color: Qt.rgba(0.0, 0.0, 0.0, 0.25)
                        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: root.dp(10)
                            anchors.rightMargin: root.dp(10)
                            spacing: root.dp(6)

                            Text {
                                text: qsTr("路径:")
                                color: "#64748b"
                                font.pixelSize: root.fs(11)
                            }

                            Text {
                                Layout.fillWidth: true
                                text: (typeof appController !== "undefined" && appController.configFilePath.length > 0)
                                      ? appController.configFilePath
                                      : qsTr("无 (可在下方快速选择或点击上方按钮浏览选择)")
                                color: (typeof appController !== "undefined" && appController.configFilePath.length > 0) ? "#cbd5e1" : "#94a3b8"
                                font.pixelSize: root.fs(11)
                                elide: Text.ElideMiddle
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(6)
                        visible: typeof appController !== "undefined" && appController.candidateConfigFiles && appController.candidateConfigFiles.length > 0

                        Text {
                            text: qsTr("检测到的本地候选配置:")
                            color: "#94a3b8"
                            font.pixelSize: root.fs(11)
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Repeater {
                                model: (typeof appController !== "undefined") ? appController.candidateConfigFiles : []

                                Rectangle {
                                    id: candidateChip
                                    implicitWidth: chipRow.implicitWidth + root.dp(16)
                                    implicitHeight: root.dp(26)
                                    radius: root.dp(6)
                                    readonly property bool isCurrent: typeof appController !== "undefined" && appController.configFilePath === modelData
                                    color: isCurrent ? Qt.rgba(0.20, 0.60, 1.0, 0.30) : Qt.rgba(1.0, 1.0, 1.0, 0.08)
                                    border.color: isCurrent ? Qt.rgba(0.40, 0.75, 1.0, 0.75) : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                    border.width: 1

                                    RowLayout {
                                        id: chipRow
                                        anchors.centerIn: parent
                                        spacing: root.dp(4)

                                        Text {
                                            text: modelData.split('/').pop()
                                            color: candidateChip.isCurrent ? "#ffffff" : "#cbd5e1"
                                            font.pixelSize: root.fs(11)
                                            font.bold: candidateChip.isCurrent
                                        }

                                        Text {
                                            visible: candidateChip.isCurrent
                                            text: "✓"
                                            color: "#38bdf8"
                                            font.pixelSize: root.fs(10)
                                            font.bold: true
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (typeof appController !== "undefined") {
                                                appController.loadConfigFile(modelData)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 亮度控制卡片（iOS 风格磨砂卡片 + 定制磨砂滑条）
            Rectangle {
                visible: !root.isIosPlatform || (typeof appController !== "undefined" && appController.hardwareBrightnessAvailable)
                Layout.fillWidth: true
                radius: root.cardRadius
                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.09)
                border.width: 1
                implicitHeight: visible ? root.dp(100) : 0
                clip: true

                Column {
                    anchors.fill: parent
                    anchors.margins: root.dp(14)
                    spacing: root.dp(8)

                    RowLayout {
                        width: parent.width

                        Text {
                            text: appController.hardwareBrightnessAvailable
                                  ? qsTr("屏幕亮度")
                                  : qsTr("屏幕亮度 (软件)")
                            color: "#f0f6fa"
                            font.pixelSize: root.fs(15)
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        // 亮度百分比药丸胶囊
                        Rectangle {
                            implicitWidth: percentText.implicitWidth + root.dp(14)
                            implicitHeight: root.dp(22)
                            radius: height / 2
                            color: Qt.rgba(0.20, 0.60, 1.0, 0.18)
                            border.color: Qt.rgba(0.40, 0.75, 1.0, 0.35)
                            border.width: 1

                            Text {
                                id: percentText
                                anchors.centerIn: parent
                                text: Math.round(appController.brightness * 100) + "%"
                                color: "#85d8ff"
                                font.pixelSize: root.fs(12)
                                font.bold: true
                            }
                        }
                    }

                    // iOS 风格毛玻璃滑块
                    Slider {
                        id: brightnessSlider
                        width: parent.width
                        from: 0.01
                        to: 1.0
                        value: appController.brightness
                        onMoved: appController.setBrightness(value)

                        background: Rectangle {
                            x: brightnessSlider.leftPadding
                            y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                            implicitWidth: 200
                            implicitHeight: root.dp(8)
                            width: brightnessSlider.availableWidth
                            height: implicitHeight
                            radius: height / 2
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.06)
                            border.width: 0.5

                            Rectangle {
                                width: brightnessSlider.visualPosition * parent.width
                                height: parent.height
                                radius: parent.radius
                                color: Qt.rgba(0.22, 0.66, 1.0, 0.85)
                            }
                        }

                        handle: Rectangle {
                            x: brightnessSlider.leftPadding + brightnessSlider.visualPosition * (brightnessSlider.availableWidth - width)
                            y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                            implicitWidth: root.dp(22)
                            implicitHeight: root.dp(22)
                            radius: width / 2
                            color: brightnessSlider.pressed ? "#e0f2fe" : "#ffffff"
                            border.color: Qt.rgba(0.0, 0.0, 0.0, 0.18)
                            border.width: 1
                            scale: brightnessSlider.pressed ? 1.15 : 1.0

                            Behavior on scale { NumberAnimation { duration: 80 } }

                            Rectangle {
                                anchors.centerIn: parent
                                width: root.dp(8)
                                height: root.dp(8)
                                radius: width / 2
                                color: Qt.rgba(0.15, 0.55, 0.95, 0.40)
                                visible: brightnessSlider.pressed
                            }
                        }
                    }
                }
            }

            // Wi-Fi 设置卡片（Linux 专享底层配置，移动端由系统原生接管故隐藏）
            Rectangle {
                id: wifiCard
                visible: !root.isMobilePlatform
                Layout.fillWidth: true
                radius: root.cardRadius
                color: wifiMouseArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.10) : Qt.rgba(1.0, 1.0, 1.0, 0.05)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.09)
                border.width: 1
                implicitHeight: visible ? root.dp(80) : 0
                scale: wifiMouseArea.pressed ? 0.98 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 100 } }
                Behavior on color { ColorAnimation { duration: 100 } }

                MouseArea {
                    id: wifiMouseArea
                    anchors.fill: parent
                    onClicked: root.wifiRequested()
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: root.dp(16)
                    spacing: root.dp(12)

                    // Wi-Fi 苹果微图标
                    Rectangle {
                        width: root.dp(36)
                        height: root.dp(36)
                        radius: root.dp(10)
                        color: Qt.rgba(0.20, 0.60, 1.0, 0.20)
                        border.color: Qt.rgba(0.40, 0.75, 1.0, 0.35)
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "W"
                            color: "#38bdf8"
                            font.pixelSize: root.fs(14)
                            font.bold: true
                        }
                    }

                    Column {
                        spacing: root.dp(3)

                        Text {
                            text: "Wi-Fi 网络"
                            color: "#f0f6fa"
                            font.pixelSize: root.fs(15)
                            font.bold: true
                        }

                        Text {
                            text: appController.wifiAvailable ? (appController.wifiInterface || "在线") : "硬件不可用"
                            color: "#8aa5b7"
                            font.pixelSize: root.fs(11)
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: appController.wifiStatus
                        color: "#85d8ff"
                        font.pixelSize: root.fs(12)
                        font.bold: true
                    }

                    Text {
                        text: "›"
                        color: Qt.rgba(1.0, 1.0, 1.0, 0.35)
                        font.pixelSize: root.fs(18)
                        font.bold: true
                    }
                }
            }

            // 屏幕电源、材质识别与熄屏设置卡片 (预留人体传感器扩展)
            Rectangle {
                Layout.fillWidth: true
                radius: root.cardRadius
                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.09)
                border.width: 1
                implicitHeight: screenPowerCol.implicitHeight + root.dp(28)
                clip: true

                ColumnLayout {
                    id: screenPowerCol
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: root.dp(14)
                    spacing: root.dp(12)

                    // 1. 屏幕材质模式选择行
                    ColumnLayout {
                        visible: !root.isIosPlatform
                        Layout.fillWidth: true
                        spacing: root.dp(6)

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: qsTr("屏幕材质:")
                                color: "#f0f6fa"
                                font.pixelSize: root.fs(13)
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: {
                                    var act = (typeof appController !== "undefined") ? appController.screenPanelType : "OLED"
                                    var cfg = (typeof appController !== "undefined") ? appController.screenPanelConfig : "auto"
                                    return cfg === "auto" ? qsTr("硬件检测为: %1").arg(act) : qsTr("手动强制: %1").arg(act)
                                }
                                color: "#38bdf8"
                                font.pixelSize: root.fs(11)
                            }
                        }

                        // 三段式材质切换按钮组
                        LiquidGlassTabBar {
                            Layout.fillWidth: true
                            implicitHeight: root.dp(32)
                            tabMode: LiquidGlassTabBar.TabMode.ModeFixed
                            model: [
                                { label: qsTr("自动检测"), val: "auto" },
                                { label: qsTr("OLED 模式"), val: "oled" },
                                { label: qsTr("LCD 模式"), val: "lcd" }
                            ]
                            currentValue: (typeof appController !== "undefined") ? appController.screenPanelConfig : "auto"
                            accentColor: "#38bdf8"
                            onTabSelected: function(index, value) {
                                if (typeof appController !== "undefined") {
                                    appController.setScreenPanelConfig(value)
                                }
                            }
                        }
                    }

                    // 2. 自动熄屏待机时间选择
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(6)

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: qsTr("自动息屏时间:")
                                color: "#f0f6fa"
                                font.pixelSize: root.fs(13)
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: {
                                    if (typeof appController === "undefined") return ""
                                    var s = appController.screenIdleSeconds
                                    if (s === 0) return qsTr("常亮不灭")
                                    return qsTr("%1秒后息屏").arg(s)
                                }
                                color: "#94a3b8"
                                font.pixelSize: root.fs(11)
                            }
                        }

                        LiquidGlassTabBar {
                            Layout.fillWidth: true
                            implicitHeight: root.dp(32)
                            tabMode: LiquidGlassTabBar.TabMode.ModeFixed
                            model: [
                                { label: qsTr("1分钟"), sec: 60 },
                                { label: qsTr("3分钟"), sec: 180 },
                                { label: qsTr("5分钟"), sec: 300 },
                                { label: qsTr("永不息屏"), sec: 0 }
                            ]
                            currentValue: (typeof appController !== "undefined") ? appController.screenIdleSeconds : 60
                            accentColor: "#38bdf8"
                            onTabSelected: function(index, value) {
                                if (typeof appController !== "undefined") {
                                    appController.setScreenIdleSeconds(value)
                                }
                            }
                        }
                    }

                    // 3. 立即熄屏操作与人体雷达提示
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(8)

                        Column {
                            Layout.fillWidth: true
                            spacing: root.dp(2)

                            Text {
                                text: (typeof appController !== "undefined" && appController.screenPanelType === "OLED")
                                      ? qsTr("OLED 像素级纯黑灭屏 · 轻触屏幕任意处即恢复")
                                      : qsTr("LCD 物理背光关断灭屏 · 轻触屏幕任意处即恢复")
                                color: "#94a3b8"
                                font.pixelSize: root.fs(11)
                            }

                            Text {
                                text: (typeof appController !== "undefined" && appController.humanPresenceDetected)
                                      ? qsTr("人体雷达: 检测到人 (保持常亮)")
                                      : qsTr("唤醒架构: 已就绪 (预留人体雷达联动端口)")
                                color: (typeof appController !== "undefined" && appController.humanPresenceDetected) ? "#4ade80" : "#64748b"
                                font.pixelSize: root.fs(10)
                            }
                        }

                        GlassButton {
                            scaleUnit: root.scaleUnit
                            styleType: "neutral"
                            implicitWidth: root.dp(76)
                            implicitHeight: root.dp(28)
                            text: qsTr("立即灭屏")
                            textPixelSize: root.fs(11)
                            onClicked: {
                                if (typeof appController !== "undefined") {
                                    appController.requestSleep()
                                }
                            }
                        }
                    }
                }
            }

            // 小爱家庭传声 (Intercom) 设置卡片
            Rectangle {
                Layout.fillWidth: true
                radius: root.cardRadius
                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.09)
                border.width: 1
                implicitHeight: intercomCol.implicitHeight + root.dp(28)
                clip: true

                property bool showAdvanced: true

                ColumnLayout {
                    id: intercomCol
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: root.dp(14)
                    spacing: root.dp(10)

                    // 1. 标题与状态
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(10)

                        Rectangle {
                            width: root.dp(32)
                            height: root.dp(32)
                            radius: root.dp(8)
                            color: Qt.rgba(0.95, 0.45, 0.15, 0.20)
                            border.color: Qt.rgba(0.95, 0.55, 0.25, 0.35)
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "MIC"
                                color: "#fb923c"
                                font.pixelSize: root.fs(10)
                                font.bold: true
                            }
                        }

                        Column {
                            Layout.fillWidth: true
                            spacing: root.dp(2)

                            Text {
                                text: qsTr("小爱家庭传声 (Intercom)")
                                color: "#f0f6fa"
                                font.pixelSize: root.fs(15)
                                font.bold: true
                            }

                            Text {
                                text: (typeof appController !== "undefined" && appController.intercomStatus)
                                      ? appController.intercomStatus
                                      : qsTr("未配置小爱凭证")
                                color: (typeof appController !== "undefined" && appController.intercomServiceToken && appController.intercomServiceToken.length > 0) ? "#4ade80" : "#94a3b8"
                                font.pixelSize: root.fs(11)
                            }
                        }

                        GlassButton {
                            scaleUnit: root.scaleUnit
                            styleType: "neutral"
                            implicitWidth: root.dp(80)
                            implicitHeight: root.dp(28)
                            text: intercomCol.parent.showAdvanced ? qsTr("收起手动") : qsTr("手动输入")
                            textPixelSize: root.fs(11)
                            onClicked: {
                                intercomCol.parent.showAdvanced = !intercomCol.parent.showAdvanced
                            }
                        }
                    }

                    // 2. 方式一：扫码登录设置 (推荐)
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: quickLoginCol.implicitHeight + root.dp(18)
                        radius: root.chipRadius
                        color: Qt.rgba(0.22, 0.65, 1.0, 0.12)
                        border.color: Qt.rgba(0.40, 0.75, 1.0, 0.28)
                        border.width: 1

                        ColumnLayout {
                            id: quickLoginCol
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.margins: root.dp(10)
                            spacing: root.dp(8)

                            RowLayout {
                                spacing: root.dp(6)
                                Text {
                                    text: "📷"
                                    font.pixelSize: root.fs(12)
                                }
                                Text {
                                    text: qsTr("方式一：扫码登录 (推荐)")
                                    color: "#93c5fd"
                                    font.pixelSize: root.fs(12)
                                    font.bold: true
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("使用小米手机或米家 App 扫码，或在系统浏览器中快速完成授权登录")
                                color: "#94a3b8"
                                font.pixelSize: root.fs(11)
                                wrapMode: Text.WordWrap
                            }

                            GlassButton {
                                Layout.fillWidth: true
                                scaleUnit: root.scaleUnit
                                styleType: "primary"
                                implicitHeight: root.dp(36)
                                text: qsTr("🌐 打开扫码登录 / 网页授权")
                                textPixelSize: root.fs(12)
                                onClicked: {
                                    console.log("[SettingsPopup] 扫码登录 requested!")
                                    root.intercomLoginRequested()
                                }
                            }
                        }
                    }

                    // 3. 方式二：手动输入凭证逻辑
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: manualInputCol.implicitHeight + root.dp(18)
                        radius: root.chipRadius
                        color: Qt.rgba(1.0, 1.0, 1.0, 0.04)
                        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                        border.width: 1

                        ColumnLayout {
                            id: manualInputCol
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.margins: root.dp(10)
                            spacing: root.dp(8)

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: root.dp(6)

                                Text {
                                    text: "📝"
                                    font.pixelSize: root.fs(12)
                                }
                                Text {
                                    text: qsTr("方式二：手动填入凭证")
                                    color: "#fb923c"
                                    font.pixelSize: root.fs(12)
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                GlassButton {
                                    scaleUnit: root.scaleUnit
                                    styleType: "neutral"
                                    implicitWidth: root.dp(60)
                                    implicitHeight: root.dp(24)
                                    text: intercomCol.parent.showAdvanced ? qsTr("收起") : qsTr("展开")
                                    textPixelSize: root.fs(10)
                                    onClicked: {
                                        intercomCol.parent.showAdvanced = !intercomCol.parent.showAdvanced
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("直接填入小米账号 ID、Token 或音箱通话凭证免密直连")
                                color: "#94a3b8"
                                font.pixelSize: root.fs(11)
                                wrapMode: Text.WordWrap
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: root.dp(6)
                                visible: intercomCol.parent.showAdvanced

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: root.dp(8)

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: root.dp(32)
                                        radius: root.dp(6)
                                        color: Qt.rgba(0.0, 0.0, 0.0, 0.35)
                                        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                        border.width: 1

                                        TextInput {
                                            id: advUserIdInput
                                            anchors.fill: parent
                                            anchors.leftMargin: root.dp(8)
                                            anchors.rightMargin: root.dp(8)
                                            verticalAlignment: TextInput.AlignVCenter
                                            text: (typeof appController !== "undefined" && appController.intercomUserId) ? appController.intercomUserId : ""
                                            color: "#f8fafc"
                                            font.pixelSize: root.fs(11)
                                            selectByMouse: true

                                            Text {
                                                anchors.fill: parent
                                                verticalAlignment: Text.AlignVCenter
                                                text: qsTr("userId (小米账号ID)")
                                                color: "#64748b"
                                                font.pixelSize: root.fs(11)
                                                visible: !advUserIdInput.text && !advUserIdInput.activeFocus
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: root.dp(32)
                                        radius: root.dp(6)
                                        color: Qt.rgba(0.0, 0.0, 0.0, 0.35)
                                        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                        border.width: 1

                                        TextInput {
                                            id: advDeviceIdInput
                                            anchors.fill: parent
                                            anchors.leftMargin: root.dp(8)
                                            anchors.rightMargin: root.dp(8)
                                            verticalAlignment: TextInput.AlignVCenter
                                            text: (typeof appController !== "undefined" && appController.intercomDeviceId) ? appController.intercomDeviceId : ""
                                            color: "#f8fafc"
                                            font.pixelSize: root.fs(11)
                                            selectByMouse: true

                                            Text {
                                                anchors.fill: parent
                                                verticalAlignment: Text.AlignVCenter
                                                text: qsTr("deviceId (留空自动生成)")
                                                color: "#64748b"
                                                font.pixelSize: root.fs(11)
                                                visible: !advDeviceIdInput.text && !advDeviceIdInput.activeFocus
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: root.dp(32)
                                    radius: root.dp(6)
                                    color: Qt.rgba(0.0, 0.0, 0.0, 0.35)
                                    border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                    border.width: 1

                                    TextInput {
                                        id: advTokenInput
                                        anchors.fill: parent
                                        anchors.leftMargin: root.dp(8)
                                        anchors.rightMargin: root.dp(8)
                                        verticalAlignment: TextInput.AlignVCenter
                                        text: (typeof appController !== "undefined" && appController.intercomServiceToken) ? appController.intercomServiceToken : ""
                                        color: "#f8fafc"
                                        font.pixelSize: root.fs(11)
                                        selectByMouse: true
                                        clip: true

                                        Text {
                                            anchors.fill: parent
                                            verticalAlignment: Text.AlignVCenter
                                            text: qsTr("serviceToken (直接粘贴抓取的Token)")
                                            color: "#64748b"
                                            font.pixelSize: root.fs(11)
                                            visible: !advTokenInput.text && !advTokenInput.activeFocus
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: root.dp(32)
                                    radius: root.dp(6)
                                    color: Qt.rgba(0.0, 0.0, 0.0, 0.35)
                                    border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                    border.width: 1

                                    TextInput {
                                        id: advMicoapiTokenInput
                                        anchors.fill: parent
                                        anchors.leftMargin: root.dp(8)
                                        anchors.rightMargin: root.dp(8)
                                        verticalAlignment: TextInput.AlignVCenter
                                        text: (typeof appController !== "undefined" && appController.micoapiServiceToken) ? appController.micoapiServiceToken : ""
                                        color: "#f8fafc"
                                        font.pixelSize: root.fs(11)
                                        selectByMouse: true
                                        clip: true

                                        Text {
                                            anchors.fill: parent
                                            verticalAlignment: Text.AlignVCenter
                                            text: qsTr("micoapiServiceToken (音箱通话凭证，以sXEJM开头)")
                                            color: "#64748b"
                                            font.pixelSize: root.fs(11)
                                            visible: !advMicoapiTokenInput.text && !advMicoapiTokenInput.activeFocus
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: root.dp(8)

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: root.dp(32)
                                        radius: root.dp(6)
                                        color: Qt.rgba(0.0, 0.0, 0.0, 0.35)
                                        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                        border.width: 1

                                        TextInput {
                                            id: advHomeIdInput
                                            anchors.fill: parent
                                            anchors.leftMargin: root.dp(8)
                                            anchors.rightMargin: root.dp(8)
                                            verticalAlignment: TextInput.AlignVCenter
                                            text: (typeof appController !== "undefined" && appController.intercomHomeId) ? appController.intercomHomeId : ""
                                            color: "#f8fafc"
                                            font.pixelSize: root.fs(11)
                                            selectByMouse: true

                                            Text {
                                                anchors.fill: parent
                                                verticalAlignment: Text.AlignVCenter
                                                text: qsTr("homeId (家庭ID，留空自动拉取)")
                                                color: "#64748b"
                                                font.pixelSize: root.fs(11)
                                                visible: !advHomeIdInput.text && !advHomeIdInput.activeFocus
                                            }
                                        }
                                    }

                                    GlassButton {
                                        scaleUnit: root.scaleUnit
                                        styleType: "primary"
                                        implicitWidth: root.dp(80)
                                        implicitHeight: root.dp(32)
                                        text: qsTr("保存凭证")
                                        textPixelSize: root.fs(11)
                                        onClicked: {
                                            if (typeof appController !== "undefined") {
                                                appController.saveIntercomSettings(advUserIdInput.text, advTokenInput.text, advDeviceIdInput.text, advHomeIdInput.text, advMicoapiTokenInput.text)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 4. 当前对讲房间列表状态
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(8)

                        Text {
                            text: qsTr("已同步房间: %1 个").arg((typeof appController !== "undefined" && appController.intercomRooms) ? appController.intercomRooms.length : 1)
                            color: "#94a3b8"
                            font.pixelSize: root.fs(11)
                        }

                        Item { Layout.fillWidth: true }

                        GlassButton {
                            scaleUnit: root.scaleUnit
                            styleType: "neutral"
                            implicitWidth: root.dp(88)
                            implicitHeight: root.dp(28)
                            text: qsTr("刷新房间列表")
                            textPixelSize: root.fs(11)
                            onClicked: {
                                if (typeof appController !== "undefined") {
                                    appController.refreshIntercomRooms()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
