import QtQuick
import QtQuick.Layouts
import HomeGui 1.0

Item {
    id: root

    property real scaleUnit: 1.0
    property string text: ""
    property string iconSource: ""
    property string styleType: "primary" // "primary" | "danger" | "accent" | "secondary" | "neutral"
    property alias type: root.styleType
    property bool isCapsule: true // 苹果经典全圆角胶囊药丸形态
    property int cornerRadius: dp(8)
    property int iconPixelSize: fs(13)
    property int textPixelSize: fs(11)
    property bool boldText: true
    property bool disabled: false

    signal clicked()

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    implicitWidth: dp(96)
    implicitHeight: dp(32)

    readonly property int effectiveRadius: isCapsule ? Math.round(height / 2) : cornerRadius
    readonly property bool isPressed: clickArea.pressed && !disabled
    readonly property bool isHovered: clickArea.containsMouse && !disabled

    // 禁用态半透明
    opacity: disabled ? 0.40 : 1.0
    Behavior on opacity { NumberAnimation { duration: 150 } }

    // 苹果仿生微弹簧下沉回弹动效
    scale: isPressed ? 0.962 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: root.isPressed ? 75 : 180
            easing.type: root.isPressed ? Easing.OutQuad : Easing.OutBack
            easing.overshoot: 1.12
        }
    }

    // 0. 悬浮弥散软环境光晕 / 投影 (苹果拟物光学悬浮感)
    Rectangle {
        id: ambientGlow
        anchors.fill: parent
        anchors.margins: -root.dp(1)
        radius: root.effectiveRadius + root.dp(1)
        z: 0
        visible: !root.disabled
        color: {
            if (root.styleType === "primary") {
                return Qt.rgba(0.20, 0.85, 0.45, root.isPressed ? 0.28 : (root.isHovered ? 0.20 : 0.12))
            }
            if (root.styleType === "danger") {
                return Qt.rgba(0.95, 0.27, 0.27, root.isPressed ? 0.28 : (root.isHovered ? 0.20 : 0.12))
            }
            if (root.styleType === "accent") {
                return Qt.rgba(0.15, 0.65, 1.00, root.isPressed ? 0.28 : (root.isHovered ? 0.20 : 0.12))
            }
            // secondary / neutral 悬浮深灰软投影
            return Qt.rgba(0.0, 0.0, 0.0, root.isPressed ? 0.35 : (root.isHovered ? 0.25 : 0.15))
        }
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    // 1. 玻璃主体晶体基座 (通透半流体材质)
    Rectangle {
        id: glassBody
        anchors.fill: parent
        radius: root.effectiveRadius
        clip: true
        z: 1

        // 苹果液态微光晶体（通透微渐变，绝不死板发闷）
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: {
                    if (root.disabled) return Qt.rgba(0.18, 0.22, 0.28, 0.35)
                    var p = root.isPressed
                    var h = root.isHovered
                    if (root.styleType === "danger") {
                        return p ? Qt.rgba(1.00, 0.32, 0.30, 0.72) : (h ? Qt.rgba(0.98, 0.28, 0.26, 0.50) : Qt.rgba(0.95, 0.25, 0.23, 0.38))
                    }
                    if (root.styleType === "accent") {
                        return p ? Qt.rgba(0.18, 0.75, 1.00, 0.72) : (h ? Qt.rgba(0.12, 0.65, 0.98, 0.50) : Qt.rgba(0.10, 0.58, 0.92, 0.38))
                    }
                    if (root.styleType === "secondary" || root.styleType === "neutral") {
                        return p ? Qt.rgba(1.0, 1.0, 1.0, 0.24) : (h ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.10))
                    }
                    // 默认 primary (苹果生机翡翠微光)
                    return p ? Qt.rgba(0.24, 0.88, 0.50, 0.72) : (h ? Qt.rgba(0.20, 0.80, 0.44, 0.50) : Qt.rgba(0.16, 0.72, 0.38, 0.38))
                }
            }
            GradientStop {
                position: 1.0
                color: {
                    if (root.disabled) return Qt.rgba(0.10, 0.14, 0.18, 0.25)
                    var p = root.isPressed
                    var h = root.isHovered
                    if (root.styleType === "danger") {
                        return p ? Qt.rgba(0.80, 0.18, 0.18, 0.58) : (h ? Qt.rgba(0.72, 0.15, 0.15, 0.35) : Qt.rgba(0.65, 0.12, 0.12, 0.24))
                    }
                    if (root.styleType === "accent") {
                        return p ? Qt.rgba(0.08, 0.48, 0.85, 0.58) : (h ? Qt.rgba(0.06, 0.40, 0.78, 0.35) : Qt.rgba(0.05, 0.35, 0.70, 0.24))
                    }
                    if (root.styleType === "secondary" || root.styleType === "neutral") {
                        return p ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : (h ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.04))
                    }
                    // 默认 primary
                    return p ? Qt.rgba(0.12, 0.65, 0.32, 0.58) : (h ? Qt.rgba(0.10, 0.55, 0.26, 0.35) : Qt.rgba(0.08, 0.48, 0.22, 0.24))
                }
            }
        }

        // 2. 360° 穹顶边缘全反射微晶描边 (模拟光线全反射 Specular Rim)
        border.width: 1
        border.color: {
            if (root.disabled) return Qt.rgba(1.0, 1.0, 1.0, 0.08)
            var p = root.isPressed
            var h = root.isHovered
            if (root.styleType === "danger") {
                return p ? Qt.rgba(1.0, 0.65, 0.65, 0.90) : (h ? Qt.rgba(1.0, 0.52, 0.52, 0.70) : Qt.rgba(1.0, 0.42, 0.42, 0.50))
            }
            if (root.styleType === "accent") {
                return p ? Qt.rgba(0.60, 0.92, 1.00, 0.90) : (h ? Qt.rgba(0.45, 0.85, 1.00, 0.70) : Qt.rgba(0.35, 0.78, 0.98, 0.50))
            }
            if (root.styleType === "secondary" || root.styleType === "neutral") {
                return p ? Qt.rgba(1.0, 1.0, 1.0, 0.45) : (h ? Qt.rgba(1.0, 1.0, 1.0, 0.30) : Qt.rgba(1.0, 1.0, 1.0, 0.18))
            }
            // primary
            return p ? Qt.rgba(0.60, 1.00, 0.72, 0.90) : (h ? Qt.rgba(0.48, 0.95, 0.62, 0.70) : Qt.rgba(0.38, 0.88, 0.52, 0.50))
        }

        // 3. 苹果凸透镜穹顶曲面高光 (Crescent Lens Specular Highlight)
        // 柔和羽化渐变弧光，赋予按钮水珠般的饱满光泽
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
                    color: Qt.rgba(1.0, 1.0, 1.0, root.isPressed ? 0.55 : (root.isHovered ? 0.42 : 0.28))
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
            opacity: root.disabled ? 0.10 : 1.0
            Behavior on opacity { NumberAnimation { duration: 100 } }
        }

        // 4. 触控压感水波提亮层 (Liquid Touch Flash)
        Rectangle {
            anchors.fill: parent
            radius: root.effectiveRadius
            color: "#ffffff"
            opacity: root.isPressed ? 0.12 : 0.0
            Behavior on opacity { NumberAnimation { duration: root.isPressed ? 60 : 140 } }
        }

        // 5. 内容排版（图标 + 苹果 SF 质感文字）
        RowLayout {
            anchors.centerIn: parent
            spacing: root.dp(6)

            Image {
                visible: root.iconSource !== ""
                source: root.iconSource
                Layout.preferredWidth: root.iconPixelSize
                Layout.preferredHeight: root.iconPixelSize
                sourceSize.width: root.iconPixelSize
                sourceSize.height: root.iconPixelSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                visible: root.text !== ""
                text: root.text
                color: "#ffffff"
                font.pixelSize: root.textPixelSize
                font.bold: root.boldText
                font.weight: root.boldText ? Font.DemiBold : Font.Normal
                Layout.alignment: Qt.AlignVCenter
            }
        }

        MouseArea {
            id: clickArea
            anchors.fill: parent
            hoverEnabled: !root.disabled
            cursorShape: root.disabled ? Qt.ArrowCursor : Qt.PointingHandCursor
            onClicked: {
                if (!root.disabled) {
                    root.clicked()
                }
            }
        }
    }
}
