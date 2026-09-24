import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

GlassPopup {
    id: root
    title: qsTr("系统设置")

    signal wifiRequested()

    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height
    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.92, Theme.dp(isPortrait ? 440 : 420))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.92, Theme.dp(isPortrait ? 580 : 400))

    property real scaleUnit: Theme.scaleUnit
    property int cardRadius: Theme.radiusCard
    property int chipRadius: Theme.radiusChip
    readonly property bool isAndroidPlatform: Theme.isAndroidPlatform

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

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

            // 亮度控制卡片（iOS 风格磨砂卡片 + 定制磨砂滑条）
            Rectangle {
                Layout.fillWidth: true
                radius: root.cardRadius
                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.09)
                border.width: 1
                implicitHeight: root.dp(100)
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

            // Wi-Fi 设置卡片（Linux 专享底层配置，Android 由系统原生接管故隐藏）
            Rectangle {
                id: wifiCard
                visible: !root.isAndroidPlatform
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
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Repeater {
                                model: [
                                    { label: "自动检测", val: "auto" },
                                    { label: "OLED 模式", val: "oled" },
                                    { label: "LCD 模式", val: "lcd" }
                                ]

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: root.dp(28)
                                    radius: root.dp(6)
                                    readonly property bool isSelected: {
                                        if (typeof appController === "undefined") return false
                                        return appController.screenPanelConfig === modelData.val
                                    }
                                    color: isSelected ? Qt.rgba(0.20, 0.60, 1.0, 0.35) : Qt.rgba(1.0, 1.0, 1.0, 0.08)
                                    border.color: isSelected ? Qt.rgba(0.40, 0.75, 1.0, 0.75) : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        color: parent.isSelected ? "#ffffff" : "#cbd5e1"
                                        font.pixelSize: root.fs(11)
                                        font.bold: parent.isSelected
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (typeof appController !== "undefined") {
                                                appController.setScreenPanelConfig(modelData.val)
                                            }
                                        }
                                    }
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

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Repeater {
                                model: [
                                    { label: "1分钟", sec: 60 },
                                    { label: "3分钟", sec: 180 },
                                    { label: "5分钟", sec: 300 },
                                    { label: "永不息屏", sec: 0 }
                                ]

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: root.dp(28)
                                    radius: root.dp(6)
                                    readonly property bool isSelected: {
                                        if (typeof appController === "undefined") return false
                                        return appController.screenIdleSeconds === modelData.sec
                                    }
                                    color: isSelected ? Qt.rgba(0.20, 0.60, 1.0, 0.35) : Qt.rgba(1.0, 1.0, 1.0, 0.08)
                                    border.color: isSelected ? Qt.rgba(0.40, 0.75, 1.0, 0.75) : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        color: parent.isSelected ? "#ffffff" : "#cbd5e1"
                                        font.pixelSize: root.fs(11)
                                        font.bold: parent.isSelected
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (typeof appController !== "undefined") {
                                                appController.setScreenIdleSeconds(modelData.sec)
                                            }
                                        }
                                    }
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
        }
    }
}
