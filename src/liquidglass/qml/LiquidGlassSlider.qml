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
    Rectangle {
        id: trackBg
        anchors.fill: parent
        radius: trackR
        color: Qt.rgba(0.04, 0.08, 0.16, 0.65)
        border.color: alertFactor > 0.05 
                      ? Qt.rgba(1.0, 0.35, 0.30, 0.55 * alertFactor) 
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

        // 跑道中心指引文字（供水滴划过时以凸透镜物理放大）
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

    // ============================================================
    // B. 水滴黏滞拖尾流光槽 (Viscous Liquid Trail)
    // ============================================================
    Item {
        id: liquidTrailLayer
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: sliderRoot.pad
        width: Math.max(sliderRoot.baseD, sliderRoot.curKnobX + sliderRoot.curKnobW * 0.5 - sliderRoot.pad)
        visible: sliderRoot.progress > 0.01

        Rectangle {
            anchors.fill: parent
            radius: sliderRoot.baseR
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { 
                    position: 0.0
                    color: sliderRoot.isAlert ? Qt.rgba(1.0, 0.25, 0.2, 0.10) : Qt.rgba(1.0, 1.0, 1.0, 0.04) 
                }
                GradientStop { 
                    position: 0.70
                    color: sliderRoot.isAlert ? Qt.rgba(1.0, 0.25, 0.2, 0.22) : Qt.rgba(1.0, 1.0, 1.0, 0.12) 
                }
                GradientStop { 
                    position: 1.0
                    color: sliderRoot.isAlert ? Qt.rgba(1.0, 0.25, 0.2, 0.45) : Qt.rgba(1.0, 1.0, 1.0, 0.28) 
                }
            }
        }
    }

    // ============================================================
    // C. 核心液态水滴图元组：母水滴 + 贝塞尔流体拔丝水带 + 滑动水滴
    // ============================================================
    Item {
        id: liquidDropGroup
        anchors.fill: parent

        // 1. 母水滴 (Mother Droplet) - 驻留在原点并随拉远平滑收缩吸入
        Rectangle {
            id: motherDrop
            visible: sliderRoot.motherR > 1.0
            x: sliderRoot.motherX - sliderRoot.motherR
            y: sliderRoot.motherY - sliderRoot.motherR
            width: sliderRoot.motherR * 2
            height: sliderRoot.motherR * 2
            radius: sliderRoot.motherR
            color: sliderRoot.currentKnobColor
            opacity: sliderRoot.motherStrength * 0.95

        }

        // 2. 表面张力粘连流体水带 (Metaball Liquid Bridge)
        Shape {
            id: liquidBridge
            anchors.fill: parent
            visible: sliderRoot.bridgeActive
            layer.enabled: true
            layer.smooth: true

            ShapePath {
                strokeWidth: 0
                fillColor: sliderRoot.currentKnobColor

                startX: sliderRoot.motherX
                startY: sliderRoot.motherY - sliderRoot.motherR * 0.85

                // 上边缘：双向凹陷三次贝塞尔曲线
                PathCubic {
                    control1X: sliderRoot.motherX + sliderRoot.dropDist * 0.35
                    control1Y: sliderRoot.motherY - sliderRoot.waistHalfW
                    control2X: sliderRoot.curKnobX - sliderRoot.curKnobW * 0.22
                    control2Y: sliderRoot.motherY - sliderRoot.waistHalfW * 1.15
                    x: sliderRoot.curKnobX
                    y: sliderRoot.curKnobY - sliderRoot.curKnobH * 0.44
                }

                // 滑动水滴端圆弧衔接
                PathLine {
                    x: sliderRoot.curKnobX
                    y: sliderRoot.curKnobY + sliderRoot.curKnobH * 0.44
                }

                // 下边缘：回程凹陷贝塞尔曲线
                PathCubic {
                    control1X: sliderRoot.curKnobX - sliderRoot.curKnobW * 0.22
                    control1Y: sliderRoot.motherY + sliderRoot.waistHalfW * 1.15
                    control2X: sliderRoot.motherX + sliderRoot.dropDist * 0.35
                    control2Y: sliderRoot.motherY + sliderRoot.waistHalfW
                    x: sliderRoot.motherX
                    y: sliderRoot.motherY + sliderRoot.motherR * 0.85
                }

                // 闭合至母水滴起点
                PathLine {
                    x: sliderRoot.motherX
                    y: sliderRoot.motherY - sliderRoot.motherR * 0.85
                }
            }

            // 水带上方的高光反射线 (Liquid Rim Highlight)
            ShapePath {
                strokeColor: Qt.rgba(1.0, 1.0, 1.0, 0.88 * sliderRoot.waistFactor)
                strokeWidth: 1.2
                fillColor: "transparent"

                startX: sliderRoot.motherX + 2
                startY: sliderRoot.motherY - sliderRoot.motherR * 0.80

                PathCubic {
                    control1X: sliderRoot.motherX + sliderRoot.dropDist * 0.35
                    control1Y: sliderRoot.motherY - sliderRoot.waistHalfW + 0.6
                    control2X: sliderRoot.curKnobX - sliderRoot.curKnobW * 0.22
                    control2Y: sliderRoot.motherY - sliderRoot.waistHalfW * 1.15 + 0.6
                    x: sliderRoot.curKnobX - 2
                    y: sliderRoot.curKnobY - sliderRoot.curKnobH * 0.42
                }
            }
        }

        // 3. 滑动水滴主球体 (Sliding Droplet Body)
        Item {
            id: mainDrop
            x: sliderRoot.curKnobX - sliderRoot.curKnobW / 2
            y: sliderRoot.curKnobY - sliderRoot.curKnobH / 2
            width: sliderRoot.curKnobW
            height: sliderRoot.curKnobH

            // 柔和环境光投射阴影
            Rectangle {
                anchors.fill: parent
                anchors.topMargin: 2
                anchors.bottomMargin: -2
                radius: height / 2
                color: Qt.rgba(0, 0, 0, 0.40)
            }

            // 水滴水银透镜外壳 (Apple Liquid Mercury Sphere)
            Rectangle {
                anchors.fill: parent
                radius: height / 2
                color: sliderRoot.currentKnobColor

                // 穹顶环境高光渐变
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    gradient: Gradient {
                        GradientStop { 
                            position: 0.0
                            color: sliderRoot.isAlert ? Qt.rgba(1.0, 1.0, 1.0, 0.90) : Qt.rgba(1.0, 1.0, 1.0, 0.98) 
                        }
                        GradientStop { 
                            position: 0.45
                            color: sliderRoot.isAlert ? Qt.rgba(1.0, 0.45, 0.40, 0.45) : Qt.rgba(0.92, 0.95, 1.0, 0.55) 
                        }
                        GradientStop { 
                            position: 1.0
                            color: sliderRoot.isAlert ? Qt.rgba(0.85, 0.15, 0.12, 0.95) : Qt.rgba(0.80, 0.84, 0.92, 0.85) 
                        }
                    }
                }

                // 凸透镜文字折射放大层 (Convex Lens Magnified Text)
                // 当水滴划过跑道中间的文字时，在水滴内部以凸透镜物理放大呈现背后的文字！
                Item {
                    id: lensMagnifier
                    anchors.fill: parent
                    clip: true
                    opacity: 0.85

                    Text {
                        x: (trackLabel.x - (mainDrop.x)) * 1.25 - (width * 0.125)
                        y: (trackLabel.y - (mainDrop.y)) * 1.25 - (height * 0.125)
                        text: sliderRoot.isAlert ? sliderRoot.alertText : sliderRoot.text
                        color: sliderRoot.isAlert ? "#ffffff" : Qt.rgba(0.12, 0.16, 0.24, 0.88)
                        font.pixelSize: Math.round(trackLabel.font.pixelSize * 1.25)
                        font.bold: true
                        visible: !sliderRoot.isAlert && (mainDrop.x + mainDrop.width > trackLabel.x) && (mainDrop.x < trackLabel.x + trackLabel.width)
                    }
                }


                // 水滴中心指示箭头 (正常态下深灰色水银触感)
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

                // 水滴中心关机图标 (警报态下白色电源图标)
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

            // 水滴外边缘微晶高保真轮廓线 (Diamond Border)
            Rectangle {
                anchors.fill: parent
                radius: height / 2
                color: "transparent"
                border.width: 1
                border.color: sliderRoot.isAlert ? Qt.rgba(1.0, 0.7, 0.65, 0.9) : Qt.rgba(1.0, 1.0, 1.0, 0.95)
            }
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
