import QtQuick 2.12
import HomeGui 1.0

Item {
    id: root

    property real scaleUnit: 1.0
    property int cornerRadius: dp(14)
    property bool isCapsule: true
    property string label: ""
    property string iconText: ""
    property string iconSource: ""
    property color activeColor: Qt.rgba(1, 1, 1, 0.08)
    property color pressedColor: Qt.rgba(1, 1, 1, 0.22)
    property color activeBorderColor: Qt.rgba(1, 1, 1, 0.18)
    property color pressedBorderColor: Qt.rgba(1, 1, 1, 0.40)
    property color activeGlowColor: "transparent"
    property color pressedGlowColor: "transparent"
    property color activeInnerColor: "transparent"
    property color pressedInnerColor: "transparent"
    property color activeIconColor: "#8fe4ff"
    property color pressedIconColor: "#ffffff"
    property color activeTextColor: "#f0f6fc"
    property color pressedTextColor: "#ffffff"
    property int iconPixelSize: fs(14)
    property int textPixelSize: fs(13)
    property bool boldIcon: false
    property bool boldText: true

    signal clicked()

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    implicitWidth: dp(88)
    implicitHeight: dp(36)

    readonly property int effectiveRadius: isCapsule ? Math.round(height / 2) : cornerRadius
    readonly property bool isPressed: clickArea.pressed
    readonly property bool isHovered: clickArea.containsMouse

    // 苹果微弹簧下沉回弹动效
    scale: isPressed ? 0.952 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: root.isPressed ? 75 : 180
            easing.type: root.isPressed ? Easing.OutQuad : Easing.OutBack
            easing.overshoot: 1.12
        }
    }

    // 0. 悬浮暗影（立体脱离背景）
    Rectangle {
        anchors.fill: parent
        anchors.margins: -root.dp(1)
        radius: root.effectiveRadius + root.dp(1)
        color: Qt.rgba(0, 0, 0, root.isPressed ? 0.30 : (root.isHovered ? 0.20 : 0.12))
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    // 1. 晶莹玻璃主体
    Rectangle {
        id: glassBody
        anchors.fill: parent
        radius: root.effectiveRadius
        clip: true

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: root.isPressed ? (root.pressedColor !== Qt.rgba(1,1,1,0.22) ? root.pressedColor : Qt.rgba(1, 1, 1, 0.22)) : (root.isHovered ? Qt.rgba(1, 1, 1, 0.15) : (root.activeColor !== Qt.rgba(1,1,1,0.08) ? root.activeColor : Qt.rgba(1, 1, 1, 0.10)))
            }
            GradientStop {
                position: 1.0
                color: root.isPressed ? Qt.rgba(1, 1, 1, 0.12) : (root.isHovered ? Qt.rgba(1, 1, 1, 0.07) : (root.activeInnerColor !== "transparent" ? root.activeInnerColor : Qt.rgba(1, 1, 1, 0.04)))
            }
        }
        border.color: root.isPressed ? (root.pressedBorderColor !== Qt.rgba(1,1,1,0.40) ? root.pressedBorderColor : Qt.rgba(1, 1, 1, 0.45)) : (root.isHovered ? Qt.rgba(1, 1, 1, 0.30) : (root.activeBorderColor !== Qt.rgba(1,1,1,0.18) ? root.activeBorderColor : Qt.rgba(1, 1, 1, 0.20)))
        border.width: 1

        // 2. 穹顶透镜曲面高光
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 1
            anchors.leftMargin: Math.max(2, Math.round(root.effectiveRadius * 0.4))
            anchors.rightMargin: Math.max(2, Math.round(root.effectiveRadius * 0.4))
            height: Math.max(2, Math.round(parent.height * 0.48))
            radius: root.effectiveRadius
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(1.0, 1.0, 1.0, root.isPressed ? 0.50 : (root.isHovered ? 0.38 : 0.24))
                }
                GradientStop {
                    position: 0.85
                    color: Qt.rgba(1.0, 1.0, 1.0, 0.02)
                }
                GradientStop {
                    position: 1.0
                    color: "transparent"
                }
            }
        }

        // 3. 触压波纹提亮
        Rectangle {
            anchors.fill: parent
            radius: root.effectiveRadius
            color: "#ffffff"
            opacity: root.isPressed ? 0.12 : 0.0
            Behavior on opacity { NumberAnimation { duration: 80 } }
        }
    }

    Row {
        anchors.centerIn: parent
        spacing: root.dp(6)

        Image {
            visible: root.iconSource !== ""
            source: root.iconSource
            width: root.iconPixelSize
            height: root.iconPixelSize
            sourceSize.width: root.iconPixelSize
            sourceSize.height: root.iconPixelSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: root.iconSource === "" && root.iconText !== ""
            text: root.iconText
            color: root.isPressed ? root.pressedIconColor : root.activeIconColor
            font.pixelSize: root.iconPixelSize
            font.bold: root.boldIcon
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: root.label !== ""
            text: root.label
            color: root.isPressed ? root.pressedTextColor : root.activeTextColor
            font.pixelSize: root.textPixelSize
            font.bold: root.boldText
            font.weight: root.boldText ? Font.DemiBold : Font.Normal
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: clickArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
