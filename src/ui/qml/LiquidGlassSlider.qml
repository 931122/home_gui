import QtQuick

Item {
    id: sliderRoot

    // ============================================================
    // 公开属性 (Public Properties)
    // ============================================================
    property real progress: 0.0 // 0.0 ~ 1.0 滑动进度
    property bool isDragging: false
    property bool interactive: false // 若由外部 MouseArea 驱动则设为 false
    property string text: qsTr("向右滑动关闭")
    property string alertText: qsTr("松手以关闭")
    property color alertColor: "#ff453a"
    property Item backgroundSource: null
    property real scrollSync: 0

    // 交互信号
    signal triggered()
    signal canceled()

    // 内部缓动状态
    readonly property bool isAlert: progress >= 0.60
    property real alertFactor: isAlert ? 1.0 : 0.0
    Behavior on alertFactor { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }

    // 苹果物理手感：按住/拖动时水滴轻微饱满放大
    property real scaleFactor: isDragging ? 1.08 : 1.0
    Behavior on scaleFactor {
        NumberAnimation {
            duration: 160
            easing.type: Easing.OutBack
            easing.overshoot: 1.15
        }
    }

    // 滑动受力横向轻微拉长拉伸
    property real stretchFactor: isDragging ? Math.min(0.22, progress * 0.32) : 0.0
    Behavior on stretchFactor { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

    // 回弹弹性动画
    NumberAnimation {
        id: springResetAnim
        target: sliderRoot
        property: "progress"
        to: 0.0
        duration: 320
        easing.type: Easing.OutBack
        easing.overshoot: 1.25
    }

    function reset() {
        springResetAnim.start()
    }

    // ============================================================
    // 1. 底层实体层：跑道与微晶基底
    // ============================================================
    Item {
        id: contentLayer
        anchors.fill: parent

        // 跑道半透明微晶槽基底
        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: Qt.rgba(0.04, 0.08, 0.16, 0.52)
            border.color: sliderRoot.alertFactor > 0.1 
                          ? Qt.rgba(1.0, 0.42, 0.35, 0.45 * sliderRoot.alertFactor) 
                          : Qt.rgba(1.0, 1.0, 1.0, 0.18)
            border.width: 1

            // 跑道中心指引文字
            Text {
                id: trackLabel
                anchors.centerIn: parent
                text: sliderRoot.isAlert ? sliderRoot.alertText : sliderRoot.text
                color: sliderRoot.isAlert ? "#ffffff" : Qt.rgba(1.0, 1.0, 1.0, 0.65)
                font.pixelSize: Math.max(12, Math.round(sliderRoot.height * 0.35))
                font.bold: true
            }

            // 右侧终点指示图标
            Image {
                anchors.right: parent.right
                anchors.rightMargin: parent.height * 0.22
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(parent.height * 0.38)
                height: width
                source: sliderRoot.isAlert ? "qrc:/icons/power.svg" : "qrc:/icons/arrow-right.svg"
                sourceSize: Qt.size(width, height)
                opacity: sliderRoot.isAlert ? 0.95 : 0.40
            }

            // 原生实体水滴滑块基底（保证在任何平台都具有饱满水银圆球触感）
            readonly property real pad: 3
            readonly property real knobRadius: (height / 2 - pad) * 0.80 * sliderRoot.scaleFactor
            readonly property real minX: pad + (height / 2 - pad)
            readonly property real maxX: width - pad - (height / 2 - pad)
            readonly property real knobCenterX: minX + (maxX - minX) * sliderRoot.progress

            Rectangle {
                id: physicalKnob
                width: parent.knobRadius * 2 * (1.0 + sliderRoot.stretchFactor * 0.22)
                height: parent.knobRadius * 2 / (1.0 + sliderRoot.stretchFactor * 0.12)
                radius: height / 2
                x: parent.knobCenterX - width / 2
                anchors.verticalCenter: parent.verticalCenter
                color: sliderRoot.isAlert ? Qt.rgba(1.0, 0.28, 0.24, 0.85) : Qt.rgba(1.0, 1.0, 1.0, 0.88)

                // 水滴顶部月牙穹顶高光
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 1
                    height: parent.height * 0.45
                    radius: height / 2
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.70) }
                        GradientStop { position: 1.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.0) }
                    }
                }

                // 水银微晶外轮廓
                border.width: 1
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.95)
            }
        }
    }

    // 捕获内容层作为纹理
    ShaderEffectSource {
        id: contentSource
        sourceItem: contentLayer
        recursive: false
        live: sliderRoot.visible
        visible: false
    }

    // ============================================================
    // 2. 核心数学着色器：Metaball smin 融球流体、按压放大、凸透镜光学放大
    // ============================================================
    ShaderEffect {
        id: fluidShader
        anchors.fill: parent

        property var source: contentSource
        property real progress: sliderRoot.progress
        property real stretchFactor: sliderRoot.stretchFactor
        property real scaleFactor: sliderRoot.scaleFactor
        property real alertFactor: sliderRoot.alertFactor
        property point resolution: Qt.point(Math.max(sliderRoot.width, 1), Math.max(sliderRoot.height, 1))

        vertexShader: "qrc:/shaders/default.vert.qsb"
        fragmentShader: "qrc:/shaders/liquid_glass_slider.frag.qsb"
    }

    // ============================================================
    // 3. 独立手势控制层 (Touch & Drag Gesture Handler)
    // ============================================================
    MouseArea {
        id: dragArea
        anchors.fill: parent
        enabled: sliderRoot.interactive
        hoverEnabled: sliderRoot.interactive
        preventStealing: sliderRoot.isDragging

        property real startX: 0
        property real startProgress: 0

        onPressed: {
            startX = mouse.x
            startProgress = sliderRoot.progress
            springResetAnim.stop()
            sliderRoot.isDragging = true
        }

        onPositionChanged: {
            if (!sliderRoot.isDragging) return
            var dx = mouse.x - startX
            var travelDist = Math.max(1, sliderRoot.width - sliderRoot.height)
            var curP = startProgress + dx / travelDist
            sliderRoot.progress = Math.max(0.0, Math.min(1.0, curP))
        }

        onReleased: {
            sliderRoot.isDragging = false
            if (sliderRoot.progress >= 0.60) {
                sliderRoot.progress = 1.0
                sliderRoot.triggered()
            } else {
                sliderRoot.reset()
                sliderRoot.canceled()
            }
        }

        onCanceled: {
            sliderRoot.isDragging = false
            sliderRoot.reset()
            sliderRoot.canceled()
        }
    }
}
