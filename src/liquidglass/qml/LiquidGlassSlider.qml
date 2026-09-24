import QtQuick
import QtQuick.Shapes

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
    property real scaleFactor: isDragging ? 1.06 : 1.0
    Behavior on scaleFactor {
        NumberAnimation {
            duration: 160
            easing.type: Easing.OutBack
            easing.overshoot: 1.15
        }
    }

    // 滑动受力横向轻微流线型拉长 (保持流体体积守恒)
    property real stretchFactor: isDragging ? Math.min(0.22, progress * 0.35) : 0.0
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
    // 几何基础尺寸与流体坐标
    // ============================================================
    readonly property real pad: 3
    readonly property real trackH: height
    readonly property real trackR: height / 2
    readonly property real baseD: Math.max(1, trackH - pad * 2)
    readonly property real baseR: baseD / 2

    readonly property real minX: pad + baseR
    readonly property real maxX: Math.max(minX, width - pad - baseR)
    readonly property real travelDist: Math.max(1, maxX - minX)

    // 1. 滑动水滴当前坐标与尺寸
    readonly property real curKnobX: minX + travelDist * Math.max(0.0, Math.min(1.0, progress))
    readonly property real curKnobY: height / 2
    readonly property real curKnobR: baseR * scaleFactor
    readonly property real curKnobW: curKnobR * 2 * (1.0 + stretchFactor * 0.25)
    readonly property real curKnobH: Math.min(trackH - 2, curKnobR * 2 / (1.0 + stretchFactor * 0.12))

    // 2. 原点母水滴 (拖拽分离时从原点按表面张力收缩)
    readonly property real motherX: minX
    readonly property real motherY: height / 2
    readonly property real motherStrength: (isDragging && progress > 0.005) ? Math.max(0.0, Math.min(1.0, 1.0 - progress / 0.28)) : 0.0
    readonly property real motherR: baseR * 0.85 * Math.pow(motherStrength, 1.2)

    // 3. Metaball 流体粘连拔丝水带 (Liquid Bridge) 几何状态
    readonly property real dropDist: Math.max(0.0, curKnobX - motherX)
    readonly property real breakDist: baseR * 2.85
    readonly property bool bridgeActive: isDragging && dropDist > 1.5 && dropDist < breakDist && motherR > 1.0

    readonly property real waistFactor: Math.max(0.0, 1.0 - Math.pow(dropDist / breakDist, 1.15))
    readonly property real waistHalfW: Math.min(motherR, curKnobR) * 0.62 * waistFactor

    // 颜色配置
    readonly property color normalKnobColor: "#ffffff"
    readonly property color alertKnobColor: "#ff453a"
    readonly property color currentKnobColor: alertFactor > 0.05 ? alertKnobColor : normalKnobColor

    // ============================================================
    // A. 跑道半透明微晶槽基底
    // ============================================================
    // ============================================================
    // A. 跑道底层纹理源（供 GPU ShaderEffect 实时折射采样）
    // ============================================================
    Item {
        id: trackContent
        anchors.fill: parent
        visible: false

        Rectangle {
            id: trackBg
            anchors.fill: parent
            radius: sliderRoot.trackR
            color: Qt.rgba(0.04, 0.08, 0.16, 0.65)
            border.color: sliderRoot.alertFactor > 0.05 
                          ? Qt.rgba(1.0, 0.35, 0.30, 0.55 * sliderRoot.alertFactor) 
                          : Qt.rgba(1.0, 1.0, 1.0, 0.18)
            border.width: 1

            // 跑道内阴影槽
            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: parent.radius - 1
                color: "transparent"
                border.color: Qt.rgba(0, 0, 0, 0.40)
                border.width: 1
            }

            // 跑道中心指引文字（供水滴划过时以凸透镜物理放大与折射扭曲）
            Text {
                id: trackLabel
                anchors.centerIn: parent
                text: sliderRoot.isAlert ? sliderRoot.alertText : sliderRoot.text
                color: sliderRoot.isAlert ? "#ffffff" : Qt.rgba(1.0, 1.0, 1.0, 0.68)
                font.pixelSize: Math.max(12, Math.round(sliderRoot.height * 0.34))
                font.bold: true
            }

            // 右侧终点指示图标
            Image {
                anchors.right: parent.right
                anchors.rightMargin: parent.height * 0.22
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(parent.height * 0.36)
                height: width
                source: sliderRoot.isAlert ? "qrc:/icons/power.svg" : "qrc:/icons/arrow-right.svg"
                sourceSize: Qt.size(width, height)
                opacity: sliderRoot.isAlert ? 0.95 : 0.45
                smooth: true
            }
        }
    }

    ShaderEffectSource {
        id: trackTextureSource
        sourceItem: trackContent
        hideSource: true
        live: true
        smooth: true
        recursive: false
    }

    // ============================================================
    // B. GPU 真实水滴物理折射、凸透镜放大与融球 ShaderEffect
    // ============================================================
    ShaderEffect {
        id: sliderShader
        anchors.fill: parent
        property variant source: trackTextureSource
        property vector2d resolution: Qt.vector2d(width, height)
        property real progress: sliderRoot.progress
        property real stretchFactor: sliderRoot.stretchFactor
        property real scaleFactor: sliderRoot.scaleFactor
        property real alertFactor: sliderRoot.alertFactor
        property real _pad: 0.0

        fragmentShader: "qrc:/qt/qml/HomeGui/LiquidGlass/shaders/liquid_glass_slider.frag.qsb"
    }

    // ============================================================
    // C. 水滴核心指示图标 (随水滴滑动位置动态居中呈现)
    // ============================================================
    Item {
        id: knobCenterIcon
        x: sliderRoot.curKnobX - width / 2
        y: sliderRoot.curKnobY - height / 2
        width: sliderRoot.curKnobW
        height: sliderRoot.curKnobH
        z: 10

        // 正常态下的水滴中心指示箭头
        Shape {
            visible: !sliderRoot.isAlert
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: sliderRoot.stretchFactor * 2
            width: 14; height: 14
            ShapePath {
                strokeColor: "#1e293b"
                strokeWidth: 2.2
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                fillColor: "transparent"
                startX: 3; startY: 7
                PathLine { x: 11; y: 7 }
            }
            ShapePath {
                strokeColor: "#1e293b"
                strokeWidth: 2.2
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                fillColor: "transparent"
                startX: 7; startY: 3
                PathLine { x: 11; y: 7 }
                PathLine { x: 7; y: 11 }
            }
        }

        // 警报态下的水滴中心白色电源图标
        Image {
            visible: sliderRoot.isAlert
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: sliderRoot.stretchFactor * 2
            width: Math.round(parent.height * 0.44)
            height: width
            source: "qrc:/icons/power.svg"
            sourceSize: Qt.size(width, height)
            smooth: true
        }
    }

    // ============================================================
    // D. 独立手势控制层 (Touch & Drag Gesture Handler)
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
