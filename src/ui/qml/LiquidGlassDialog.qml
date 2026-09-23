import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

Dialog {
    id: root

    property string titleText: ""
    property Item backgroundSource: typeof glassRuntime !== "undefined" ? glassRuntime.backdropSource : null
    property color overLightTextColor: "#111827"
    property color overDarkTextColor: "#f8fafc"
    property bool animateShow: true
    property bool dimBehind: true

    modal: true
    focus: true
    padding: 0
    width: Math.min(parent ? parent.width * 0.92 : Theme.dp(360), Theme.dp(440))
    x: parent ? (parent.width - width) / 2 : 0

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, root.dimBehind ? 0.38 : 0.0) }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: root.animateShow && (typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled) ? 180 : 0 }
        NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: root.animateShow && (typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled) ? 180 : 0; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: root.animateShow && (typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled) ? 130 : 0 }
    }

    background: Item {
        implicitHeight: contentColumn.implicitHeight + Theme.dp(32)
        clip: true
        LiquidGlassSurface {
            anchors.fill: parent
            backgroundSource: root.backgroundSource
            cornerRadius: Theme.dp(18)
            baseOpacity: 0.72
            tintColor: Qt.rgba(0.12, 0.20, 0.30, 0.62)
            tintStrength: 0.28
            blurAmount: 0.9
        }
        Rectangle {
            anchors.fill: parent
            radius: Theme.dp(18)
            color: "transparent"
            border.color: Qt.rgba(1, 1, 1, 0.22)
            border.width: 1
        }
    }

    contentItem: ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        spacing: Theme.dp(12)
        anchors.margins: Theme.dp(20)

        Label {
            Layout.fillWidth: true
            text: root.titleText || root.title
            color: typeof glassRuntime !== "undefined" && glassRuntime.backdropLuminance > 0.55 ? root.overLightTextColor : root.overDarkTextColor
            font.pixelSize: Theme.fs(18)
            font.bold: true
            visible: text.length > 0
        }
    }
}
