import QtQuick

Item {
    id: root

    // ============================================================
    // 公开属性 (Public Properties - 对标 Apple Liquid Glassmorphism)
    // ============================================================

    // 背景捕获源
    property Item backgroundSource: null

    // 材质参数
    property real baseOpacity: 0.42
    property color tintColor: Qt.rgba(1.0, 1.0, 1.0, 0.12)
    property real tintStrength: 0.25
    property real noiseAmount: 0.016
    property real distortionStrength: 0.015
    property real highlightIntensity: 0.85
    property real edgeFresnelPower: 2.2
    property color edgeHighlightColor: Qt.rgba(1.0, 1.0, 1.0, 0.95)
    property real cornerRadius: 14
    property real elevation: 0
    property color shadowColor: Qt.rgba(0, 0, 0, 0.3)

    // 交互状态追踪
    property bool hovered: false
    property bool pressed: false
    property point pointerPosition: Qt.point(width / 2, height / 2)
    property real scrollSync: 0
    property real lensMagnification: 0.0

    // 动画时间基准
    property real animationTime: 0

    // 内容插槽
    default property alias content: contentContainer.data

    // 内部计算状态
    property real _effectiveHighlight: highlightIntensity
    property real _effectiveDistortion: distortionStrength

    readonly property point _normalizedPointer: Qt.point(
        pointerPosition.x / Math.max(width, 1),
        pointerPosition.y / Math.max(height, 1)
    )

    Timer {
        id: animTimer
        running: root.visible && root.opacity > 0.01
        repeat: true
        interval: 32
        onTriggered: root.animationTime += 0.032
    }

    onHoveredChanged: updateEffectiveValues()
    onPressedChanged: updateEffectiveValues()
    onHighlightIntensityChanged: updateEffectiveValues()
    onDistortionStrengthChanged: updateEffectiveValues()

    function updateEffectiveValues() {
        var baseHighlight = highlightIntensity
        if (pressed) baseHighlight += 0.35
        else if (hovered) baseHighlight += 0.20
        _effectiveHighlight = baseHighlight

        var baseDistortion = distortionStrength
        if (pressed) baseDistortion += 0.008
        else if (hovered) baseDistortion += 0.004
        _effectiveDistortion = baseDistortion
    }

    Behavior on _effectiveHighlight {
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }
    Behavior on _effectiveDistortion {
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }

    // ============================================================
    // 1. 苹果原生高透微晶实体底板 (Crystal Glass Bed)
    // 无论 GPU 着色器是否离屏抓取就绪，底层都拥有纯净通透的微晶光泽
    // ============================================================
    Rectangle {
        id: crystalBed
        anchors.fill: parent
        radius: root.cornerRadius
        color: root.tintColor
        opacity: Math.max(0.18, root.baseOpacity)
        z: 0

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Qt.rgba(
                    Math.min(1.0, root.tintColor.r * 1.25 + 0.08),
                    Math.min(1.0, root.tintColor.g * 1.25 + 0.08),
                    Math.min(1.0, root.tintColor.b * 1.25 + 0.08),
                    Math.min(1.0, root.tintColor.a * 1.20)
                )
            }
            GradientStop {
                position: 1.0
                color: Qt.rgba(
                    root.tintColor.r * 0.75,
                    root.tintColor.g * 0.75,
                    root.tintColor.b * 0.75,
                    root.tintColor.a * 0.85
                )
            }
        }
    }

    // ============================================================
    // 2. 背景捕获与安全回退纹理
    // ============================================================
    Item {
        id: defaultBackdropItem
        anchors.fill: parent
        visible: false
        Rectangle {
            anchors.fill: parent
            color: "#182638"
        }
    }

    readonly property rect _capturedRect: {
        var _sync = root.scrollSync
        if (!root.backgroundSource || !root.visible || root.width <= 0 || root.height <= 0) {
            return Qt.rect(0, 0, 1, 1)
        }
        try {
            var pt = root.mapToItem(root.backgroundSource, 0, 0)
            return Qt.rect(Math.max(0, pt.x), Math.max(0, pt.y), Math.max(1, root.width), Math.max(1, root.height))
        } catch(e) {
            return Qt.rect(0, 0, 1, 1)
        }
    }

    ShaderEffectSource {
        id: bgCapture
        sourceItem: root.backgroundSource ? root.backgroundSource : defaultBackdropItem
        sourceRect: root._capturedRect
        textureSize: Qt.size(Math.max(1, Math.round(root.width * 0.25)), Math.max(1, Math.round(root.height * 0.25)))
        smooth: true
        anchors.fill: parent
        recursive: false
        live: root.visible
        visible: false
    }

    // ============================================================
    // 3. 真实光学液态玻璃 Shader (Qt 6 RHI 预编译 QSB 着色器)
    // 包含：圆角 SDF 法线、引力透镜逆幂折射、双对称高光瓣、菲涅尔微晶边缘
    // ============================================================
    ShaderEffect {
        id: glassShader
        anchors.fill: parent
        z: 1

        property var source: bgCapture
        property real hasSource: root.backgroundSource ? 1.0 : 0.0

        property real time: root.animationTime
        property real opacity_: root.baseOpacity
        property color tint: root.tintColor
        property real tintStr: root.tintStrength
        property real noise: root.noiseAmount
        property real distortion: root._effectiveDistortion
        property real highlight: root._effectiveHighlight
        property real fresnel: root.edgeFresnelPower
        property color edgeColor: root.edgeHighlightColor
        property point pointer: root._normalizedPointer
        property real hoverState: root.hovered ? 1.0 : 0.0
        property real pressState: root.pressed ? 1.0 : 0.0
        property point resolution: Qt.point(root.width, root.height)
        property real cornerRadius: root.cornerRadius
        property real lensMagnification: root.lensMagnification

        vertexShader: "qrc:/shaders/default.vert.qsb"
        fragmentShader: "qrc:/shaders/liquid_glass_surface.frag.qsb"
    }

    // ============================================================
    // 4. 苹果凸透镜穹顶微弧光反光层 (Crescent Lens Specular Highlight)
    // 模拟真实厚玻璃表面张力产生的微弧光
    // ============================================================
    Rectangle {
        id: topSheen
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        height: Math.max(2, Math.round(parent.height * 0.44))
        radius: root.cornerRadius
        z: 2

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Qt.rgba(1.0, 1.0, 1.0, root.pressed ? 0.32 : (root.hovered ? 0.22 : 0.14))
            }
            GradientStop {
                position: 1.0
                color: Qt.rgba(1.0, 1.0, 1.0, 0.0)
            }
        }
    }

    // ============================================================
    // 5. 360° 物理全反射微晶描边 (360° Specular Rim)
    // ============================================================
    Rectangle {
        id: maskRect
        anchors.fill: parent
        radius: root.cornerRadius
        color: "transparent"
        border.width: 1
        border.color: root.pressed 
                    ? Qt.rgba(1.0, 1.0, 1.0, 0.38) 
                    : (root.hovered ? Qt.rgba(1.0, 1.0, 1.0, 0.25) : Qt.rgba(1.0, 1.0, 1.0, 0.14))
        z: 3
    }

    // 内容容器
    Item {
        id: contentContainer
        anchors.fill: parent
        z: 10
    }
}
