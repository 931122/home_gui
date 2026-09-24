import QtQuick

Item {
    id: root
    implicitWidth: 36
    implicitHeight: 36
    property color iconColor: clickArea.pressed ? "#ff6b6b" : (clickArea.containsMouse ? "#ffffff" : Qt.rgba(1, 1, 1, 0.70))
    property int hitMargin: -8
    property int iconSize: 13

    signal clicked()

    readonly property bool isPressed: clickArea.pressed
    readonly property bool isHovered: clickArea.containsMouse

    // 苹果微弹簧弹性缩放反馈
    scale: isPressed ? 0.91 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: root.isPressed ? 75 : 180
            easing.type: root.isPressed ? Easing.OutQuad : Easing.OutBack
            easing.overshoot: 1.15
        }
    }

    // 0. 悬浮暗影（立体脱离底层背景）
    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        radius: width / 2
        color: Qt.rgba(0, 0, 0, root.isPressed ? 0.35 : (root.isHovered ? 0.25 : 0.15))
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    // 1. 苹果正圆形磨砂微晶玻璃纽扣底座
    Rectangle {
        id: body
        anchors.fill: parent
        radius: width / 2
        clip: true

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: root.isPressed ? Qt.rgba(1, 1, 1, 0.28) : (root.isHovered ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(1, 1, 1, 0.10))
            }
            GradientStop {
                position: 1.0
                color: root.isPressed ? Qt.rgba(1, 1, 1, 0.14) : (root.isHovered ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(1, 1, 1, 0.04))
            }
        }
        border.color: root.isPressed ? Qt.rgba(1, 1, 1, 0.45) : (root.isHovered ? Qt.rgba(1, 1, 1, 0.30) : Qt.rgba(1, 1, 1, 0.18))
        border.width: 1


        // 3. 触压波纹提亮
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "#ffffff"
            opacity: root.isPressed ? 0.15 : 0.0
            Behavior on opacity { NumberAnimation { duration: 80 } }
        }
    }

    // 4. 双重矢量抗锯齿交叉矩形 (免疫字体缺失导致的方框方块)
    Item {
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize

        Rectangle {
            anchors.centerIn: parent
            width: Math.round(parent.width * 1.30)
            height: 2
            radius: 1
            rotation: 45
            color: root.iconColor
            antialiasing: true
            Behavior on color { ColorAnimation { duration: 100 } }
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.round(parent.width * 1.30)
            height: 2
            radius: 1
            rotation: -45
            color: root.iconColor
            antialiasing: true
            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    MouseArea {
        id: clickArea
        anchors.fill: parent
        anchors.margins: root.hitMargin
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
