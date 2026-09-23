import QtQuick
import QtQuick.Controls

Item {
    id: root

    // ============================================================
    // 公开属性 (Public API)
    // ============================================================
    property string text: ""
    property string subText: ""
    property string iconSource: ""
    property bool checked: false
    property bool checkable: true
    property color accentColor: "#38bdf8"
    property Item backgroundSource: null
    property real cornerRadius: root.dp(14)
    property bool isAvailable: true

    // 滑动关闭支持
    property bool isSlideToTurnOff: false
    property real slideProgress: 0.0
    property bool isDraggingSlide: false
    property bool showSlideHint: false

    signal clicked()
    signal slideCompleted()

    implicitWidth: root.dp(120)
    implicitHeight: root.dp(52)

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    // ============================================================
    // 物理弹性按压手感 (Spring Touch Dynamics)
    // ============================================================
    property real _pressScale: 1.0
    scale: _pressScale

    Behavior on _pressScale {
        NumberAnimation { duration: 140; easing.type: Easing.OutBack }
    }

    SequentialAnimation {
        id: shakeAnim
        NumberAnimation { target: cardContainer; property: "x"; from: 0; to: -root.dp(6); duration: 45; easing.type: Easing.OutQuad }
        NumberAnimation { target: cardContainer; property: "x"; from: -root.dp(6); to: root.dp(6); duration: 60; easing.type: Easing.InOutQuad }
        NumberAnimation { target: cardContainer; property: "x"; from: root.dp(6); to: -root.dp(4); duration: 50; easing.type: Easing.InOutQuad }
        NumberAnimation { target: cardContainer; property: "x"; from: -root.dp(4); to: root.dp(4); duration: 50; easing.type: Easing.InOutQuad }
        NumberAnimation { target: cardContainer; property: "x"; from: root.dp(4); to: 0; duration: 45; easing.type: Easing.InQuad }
    }

    Timer {
        id: slideHintTimer
        interval: 2200
        onTriggered: root.showSlideHint = false
    }

    NumberAnimation {
        id: resetSlideAnim
        target: root
        property: "slideProgress"
        to: 0.0
        duration: 240
        easing.type: Easing.OutBack
    }

    // ============================================================
    // 按钮视觉主容器
    // ============================================================
    Item {
        id: cardContainer
        anchors.fill: parent

        // 1. 悬浮暗色软阴影 (Elevation Drop Shadow)
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: root.dp(3)
            anchors.bottomMargin: -root.dp(3)
            radius: root.cornerRadius
            color: Qt.rgba(0, 0, 0, 0.35)
            opacity: btnArea.pressed ? 0.20 : 0.45
            z: 0
        }

        // 2. 真实液态玻璃光学着色器层 (LiquidGlassSurface Shader)
        LiquidGlassSurface {
            id: glassShader
            anchors.fill: parent
            backgroundSource: root.backgroundSource
            cornerRadius: root.cornerRadius
            baseOpacity: root.checked ? 0.50 : 0.36
            tintColor: root.checked 
                       ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.65) 
                       : Qt.rgba(1.0, 1.0, 1.0, 0.12)
            tintStrength: root.checked ? 0.45 : 0.18
            highlightIntensity: btnArea.pressed ? 0.95 : (root.checked ? 0.88 : 0.70)
            edgeFresnelPower: 2.2
            hovered: btnArea.containsMouse
            pressed: btnArea.pressed
            pointerPosition: Qt.point(btnArea.mouseX, btnArea.mouseY)
            opacity: root.isAvailable ? 1.0 : 0.3
            z: 1
        }
        
        // 3. 状态轮廓保护层（仅在激活或滑动警告时呈现极微发丝边缘，杜绝任何人工死白月牙）
        Rectangle {
            anchors.fill: parent
            radius: root.cornerRadius
            color: "transparent"
            border.color: (root.isSlideToTurnOff && root.checked && root.showSlideHint)
                          ? Qt.rgba(1, 0.8, 0.4, 0.65)
                          : (root.checked ? Qt.rgba(1, 1, 1, 0.22) : Qt.rgba(1, 1, 1, 0.10))
            border.width: 1
            z: 2
        }

        // 6. 内容排布 (Content Layout)
        Row {
            id: contentRow
            anchors.fill: parent
            anchors.margins: root.dp(8)
            spacing: root.dp(10)
            opacity: (root.isSlideToTurnOff && root.checked && (root.isDraggingSlide || root.slideProgress > 0.01))
                     ? Math.max(0.08, 1.0 - root.slideProgress * 2.5) : 1.0
            z: 10

            // 左侧：晶莹通透的水晶圆盘图标底座
            Rectangle {
                width: root.dp(36)
                height: root.dp(36)
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                gradient: Gradient {
                    GradientStop { position: 0.0; color: root.checked ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(1, 1, 1, 0.12) }
                    GradientStop { position: 1.0; color: root.checked ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(1, 1, 1, 0.04) }
                }
                border.color: root.checked ? Qt.rgba(1, 1, 1, 0.35) : Qt.rgba(1, 1, 1, 0.15)
                border.width: 1

                Image {
                    anchors.centerIn: parent
                    width: root.dp(20)
                    height: root.dp(20)
                    source: root.iconSource
                    sourceSize: Qt.size(width, height)
                    smooth: true
                    visible: status === Image.Ready
                }
            }

            // 中间文本区域
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - root.dp(36 + 10 + 30)
                spacing: root.dp(2)

                Text {
                    width: parent.width
                    text: root.text
                    color: root.checked ? "#ffffff" : "#d8e2ec"
                    font.pixelSize: root.fs(13)
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: root.subText
                    color: root.checked ? "#86efac" : "#8ea1b4"
                    font.pixelSize: root.fs(10)
                    font.bold: root.checked
                    elide: Text.ElideRight
                    visible: text !== ""
                }
            }

            // 右侧滑动关机提示小胶囊
            Rectangle {
                width: root.dp(26)
                height: root.dp(24)
                radius: root.dp(12)
                anchors.verticalCenter: parent.verticalCenter
                color: root.showSlideHint ? Qt.rgba(1, 1, 1, 0.30) : Qt.rgba(1, 1, 1, 0.10)
                border.color: root.showSlideHint ? Qt.rgba(1, 1, 1, 0.50) : Qt.rgba(1, 1, 1, 0.20)
                border.width: 1
                visible: root.isSlideToTurnOff && root.checked && !root.isDraggingSlide && root.slideProgress <= 0.01

                Image {
                    anchors.centerIn: parent
                    width: root.dp(11)
                    height: root.dp(11)
                    source: "qrc:/icons/arrow-right.svg"
                    sourceSize: Qt.size(width, height)
                    smooth: true
                }

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: root.isSlideToTurnOff && root.checked && !root.isDraggingSlide && !root.showSlideHint
                    NumberAnimation { to: 0.35; duration: 900; easing.type: Easing.InOutQuad }
                    NumberAnimation { to: 0.95; duration: 900; easing.type: Easing.InOutQuad }
                }
            }
        }

        // ============================================================
        // 7. 苹果纯正水滴液态微透镜滑动关机跑道 (Apple Liquid Glass Slider)
        // ============================================================
        LiquidGlassSlider {
            id: slideCapsuleTrack
            anchors.fill: parent
            anchors.margins: root.dp(4)
            visible: root.isSlideToTurnOff && root.checked && (root.isDraggingSlide || root.slideProgress > 0.005)
            z: 25
            progress: root.slideProgress
            isDragging: root.isDraggingSlide
            interactive: false
            backgroundSource: root.backgroundSource
        }
    }

    // ============================================================
    // 交互与手势处理 (MouseArea)
    // ============================================================
    MouseArea {
        id: btnArea
        anchors.fill: parent
        property real startX: 0
        property real startY: 0
        property bool dragTriggered: false

        onPressed: {
            startX = mouse.x
            startY = mouse.y
            dragTriggered = false
            root._pressScale = 0.965
            resetSlideAnim.stop()

            if (root.isSlideToTurnOff && root.checked) {
                btnArea.preventStealing = true
            }
        }

        onPositionChanged: {
            var dx = mouse.x - startX
            var dy = mouse.y - startY

            if (root.isSlideToTurnOff && root.checked) {
                if (!root.isDraggingSlide && Math.abs(dy) > root.dp(12) && Math.abs(dy) > Math.abs(dx) * 1.6) {
                    btnArea.preventStealing = false
                    return
                }
                if (!dragTriggered && (dx > root.dp(4) || Math.abs(dx) > Math.abs(dy))) {
                    dragTriggered = true
                    root.isDraggingSlide = true
                    btnArea.preventStealing = true
                }
                if (root.isDraggingSlide) {
                    var travelDist = Math.max(1, root.width - root.dp(48))
                    var curDist = Math.max(0, Math.min(travelDist, dx))
                    root.slideProgress = curDist / travelDist
                }
            }
        }

        onReleased: {
            btnArea.preventStealing = false
            root._pressScale = 1.0

            if (root.isSlideToTurnOff && root.checked) {
                if (root.isDraggingSlide) {
                    root.isDraggingSlide = false
                    if (root.slideProgress >= 0.60) {
                        root.slideProgress = 0.0
                        root.slideCompleted()
                    } else {
                        resetSlideAnim.start()
                    }
                }
            }
        }

        onCanceled: {
            btnArea.preventStealing = false
            root._pressScale = 1.0
            if (root.isSlideToTurnOff && root.checked) {
                root.isDraggingSlide = false
                resetSlideAnim.start()
            }
        }

        onClicked: {
            if (dragTriggered) return

            if (root.isSlideToTurnOff && root.checked) {
                root.showSlideHint = true
                slideHintTimer.restart()
                shakeAnim.restart()
                return
            }

            root.clicked()
        }
    }
}
