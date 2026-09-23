import QtQuick
import QtQuick.Controls
import HomeGui 1.0

Item {
    id: root

    // ============================================================
    // 公开属性 (Public API - LiquidGlassToast)
    // ============================================================
    property string message: ""
    property string iconSource: ""
    property Item backgroundSource: null
    property int duration: 2500

    function dp(v) { return (typeof Theme !== "undefined" && Theme) ? Theme.dp(v) : v }
    function fs(v) { return (typeof Theme !== "undefined" && Theme) ? Theme.fs(v) : v }

    implicitWidth: toastRow.implicitWidth + root.dp(36)
    implicitHeight: root.dp(44)

    function show(msg, icon) {
        if (msg) root.message = msg
        if (icon) root.iconSource = icon
        hideTimer.stop()
        opacity = 1.0
        yOffset = 0
        hideTimer.interval = root.duration
        hideTimer.restart()
    }

    property real yOffset: 20
    opacity: 0.0
    visible: opacity > 0.01

    Behavior on opacity {
        NumberAnimation { duration: typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled ? 220 : 0; easing.type: Easing.OutQuad }
    }
    Behavior on yOffset {
        NumberAnimation { duration: typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled ? 260 : 0; easing.type: Easing.OutBack }
    }

    Timer {
        id: hideTimer
        onTriggered: {
            root.opacity = 0.0
            root.yOffset = 20
        }
    }

    // 1. 悬浮柔和阴影
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: root.dp(4)
        anchors.bottomMargin: -root.dp(4)
        radius: root.height / 2
        color: Qt.rgba(0, 0, 0, 0.45)
        z: 0
    }

    // 2. Clear 极致高透液态玻璃胶囊表面
    LiquidGlassSurface {
        id: toastSurface
        anchors.fill: parent
        backgroundSource: root.backgroundSource
        cornerRadius: root.height / 2
        materialVariant: LiquidGlassSurface.MaterialVariant.Clear
        baseOpacity: 0.38
        tintColor: Qt.rgba(0.08, 0.16, 0.28, 0.45)
        tintStrength: 0.22
        highlightIntensity: 0.90
        dispersion: 0.20
        lensMagnification: 0.08
        z: 1
    }

    // 3. 内容横向排布
    Row {
        id: toastRow
        anchors.centerIn: parent
        spacing: root.dp(8)
        z: 10

        Image {
            id: toastIcon
            width: root.dp(18)
            height: root.dp(18)
            anchors.verticalCenter: parent.verticalCenter
            source: root.iconSource
            sourceSize: Qt.size(width, height)
            smooth: true
            visible: root.iconSource !== "" && status === Image.Ready
        }

        Text {
            id: toastText
            anchors.verticalCenter: parent.verticalCenter
            text: root.message
            font.pixelSize: root.fs(13)
            font.bold: true
            color: "#ffffff"
        }
    }
}
