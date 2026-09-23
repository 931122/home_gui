import QtQuick

Item {
    id: root

    // ============================================================
    // 公开属性 (Public API - LiquidGlassChip)
    // ============================================================
    property string text: ""
    property string iconSource: ""
    property bool checked: false
    property bool checkable: true
    property Item backgroundSource: null
    property color accentColor: "#38bdf8"
    function dp(v) { return (typeof Theme !== "undefined" && Theme) ? Theme.dp(v) : v }
    function fs(v) { return (typeof Theme !== "undefined" && Theme) ? Theme.fs(v) : v }

    property real cornerRadius: root.dp(16)
    property int materialVariant: LiquidGlassSurface.MaterialVariant.Regular

    signal clicked()

    implicitWidth: chipRow.implicitWidth + root.dp(24)
    implicitHeight: root.dp(32)

    // 弹性轻按反馈
    property real _scale: chipArea.pressed ? 0.95 : 1.0
    scale: _scale
    Behavior on _scale {
        enabled: typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled
        NumberAnimation { duration: 120; easing.type: Easing.OutQuad }
    }

    // 1. 底层液态微晶芯片表面 (LiquidGlassSurface)
    LiquidGlassSurface {
        id: surface
        anchors.fill: parent
        backgroundSource: root.backgroundSource
        cornerRadius: root.cornerRadius
        materialVariant: root.materialVariant
        baseOpacity: root.checked ? 0.50 : 0.28
        tintColor: root.checked 
                   ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.6) 
                   : Qt.rgba(1.0, 1.0, 1.0, 0.10)
        tintStrength: root.checked ? 0.40 : 0.15
        highlightIntensity: chipArea.pressed ? 0.95 : (root.checked ? 0.85 : 0.65)
        edgeFresnelPower: 2.2
        hovered: chipArea.containsMouse
        pressed: chipArea.pressed
        pointerPosition: Qt.point(chipArea.mouseX, chipArea.mouseY)
        z: 0
    }

    // 2. 状态微细描边
    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: "transparent"
        border.width: 1
        border.color: root.checked ? Qt.rgba(1, 1, 1, 0.40) : Qt.rgba(1, 1, 1, 0.14)
        z: 1
    }

    // 3. 内容横向排布 (Icon + Text)
    Row {
        id: chipRow
        anchors.centerIn: parent
        spacing: root.dp(6)
        z: 5

        Image {
            id: chipIcon
            width: root.dp(14)
            height: root.dp(14)
            anchors.verticalCenter: parent.verticalCenter
            source: root.iconSource
            sourceSize: Qt.size(width, height)
            smooth: true
            visible: root.iconSource !== "" && status === Image.Ready
        }

        Text {
            id: chipText
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font.pixelSize: root.fs(12)
            font.bold: root.checked
            color: root.checked ? "#ffffff" : (surface.isDarkBackground ? "#e2e8f0" : "#1e293b")

            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }
    }

    MouseArea {
        id: chipArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (root.checkable) {
                root.checked = !root.checked
            }
            root.clicked()
        }
    }
}
