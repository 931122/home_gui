import QtQuick
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0

Item {
    id: root

    property real scaleUnit: 1.0
    property int cornerRadius: dp(14)
    property bool isCapsule: false
    property string label: ""
    property string iconText: ""
    property string iconSource: ""
    property color activeColor: Qt.rgba(1, 1, 1, 0.08)
    property color pressedColor: Qt.rgba(1, 1, 1, 0.28)
    property color activeBorderColor: Qt.rgba(1, 1, 1, 0.18)
    property color pressedBorderColor: Qt.rgba(1, 1, 1, 0.40)
    property color activeIconColor: "#8fe4ff"
    property color pressedIconColor: "#ffffff"
    property color activeTextColor: "#f0f6fc"
    property color pressedTextColor: "#ffffff"
    property int iconPixelSize: fs(14)
    property int textPixelSize: fs(13)
    property bool boldIcon: false
    property bool boldText: true
    property bool fluidTouchRefraction: true

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

    // Clear material gives compact controls the same refractive lens as the shared glass surfaces.
    LiquidGlassSurface {
        id: glassBody
        anchors.fill: parent
        cornerRadius: root.effectiveRadius
        materialVariant: LiquidGlassSurface.MaterialVariant.Clear
        tintColor: root.activeColor
        baseOpacity: 0.22
        tintStrength: 0.24
        blurAmount: 0.28
        distortionStrength: 0.035
        lensMagnification: 0.36
        highlightIntensity: 0.78
        fluidTouchRefraction: root.fluidTouchRefraction
        hovered: root.isHovered
        pressed: root.isPressed
        pointerPosition: Qt.point(clickArea.mouseX, clickArea.mouseY)

        Row {
            id: contentRow
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
    }

    Rectangle {
        anchors.fill: parent
        radius: root.effectiveRadius
        color: "transparent"
        border.width: 1
        border.color: root.isPressed ? root.pressedBorderColor
                                     : (root.isHovered ? Qt.rgba(1, 1, 1, 0.30) : root.activeBorderColor)
    }

    MouseArea {
        id: clickArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
