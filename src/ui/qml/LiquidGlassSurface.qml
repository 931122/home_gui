import QtQuick

Item {
    id: root

    // ============================================================
    // 公开属性 (Public API - 对标 Android Liquid Glass 2.0 / VisionOS)
    // ============================================================

    // 材质变体定义
    enum MaterialVariant {
        Regular = 0, // 强调可读性，深邃高斯磨砂与微晶吸光基底 (Apple Material Regular)
        Clear   = 1  // 强调极致高透、纯净冰晶、强透镜压缩折射与光谱色散 (Apple Material Clear)
    }

    property int materialVariant: LiquidGlassSurface.MaterialVariant.Regular

    // 背景捕获源
    property Item backgroundSource: null

    // 核心透镜与材质参数
    property real baseOpacity: materialVariant === LiquidGlassSurface.MaterialVariant.Clear ? 0.30 : 0.46
    property color tintColor: Qt.rgba(1.0, 1.0, 1.0, 0.12)
    property real tintStrength: materialVariant === LiquidGlassSurface.MaterialVariant.Clear ? 0.15 : 0.28
    property real noiseAmount: 0.016
    property real distortionStrength: 0.015
    property real highlightIntensity: 0.85
    property real edgeFresnelPower: 2.2
    property color edgeHighlightColor: Qt.rgba(1.0, 1.0, 1.0, 0.95)
    property real cornerRadius: 14
    property real elevation: 0
    property color shadowColor: Qt.rgba(0, 0, 0, 0.3)

    // 🌈 物理色散 (Chromatic Aberration - 三通道光谱分离)
    property real dispersion: 0.16

    // 🌫️ 滚动边缘渐进模糊 (Progressive Blur)
    // 0: 全局均匀, 1: 顶部渐进 (从 top 模糊渐变到 bottom 清晰), 2: 底部渐进
    property int progressiveMode: 0

    // 💡 传感器与倾斜高光 (Sensor / Tilt Vector)
    // 可外接陀螺仪/加速度计，或随触摸/指针倾斜
    property vector2d tilt: Qt.vector2d(0.0, 0.0)

    // 🫧 液态融合 (Metaball smin 双玻璃形状黏连合并)
    property vector2d secondaryPos: Qt.vector2d(0, 0)
    property vector2d secondarySize: Qt.vector2d(0, 0)
    property real secondaryRadius: 0
    property real secondaryActive: 0.0
    property real sminFactor: 18.0

    // 👆 按压液态凸起/凹陷幅度
    property real pressBulge: 1.0

    // 🌓 亮度与背景感知 (Luminance Sensing)
    // 0.0 ~ 1.0: 供上层容器或文字图标自适应深浅色反色
    readonly property real detectedLuminance: _calcLuminance
    readonly property bool isDarkBackground: detectedLuminance < 0.45

    // ♿ 无障碍降级与节能模式 (Accessibility / Battery Saver Fallback)
    // 在高对比度或极端低功耗芯片上，直接退化为原生扁平高对比度矩形，零 Shader 开销
    property bool accessibleFallback: false

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

    // 根据背景染色粗略计算的明度（着色器内部有精确逐像素感知）
    readonly property real _calcLuminance: {
        var c = root.tintColor
        return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b
    }

    Timer {
        id: animTimer
        running: !root.accessibleFallback && root.visible && root.opacity > 0.01
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
        if (pressed) baseHighlight += 0.40
        else if (hovered) baseHighlight += 0.22
        _effectiveHighlight = baseHighlight

        var baseDistortion = distortionStrength
        if (pressed) baseDistortion += 0.010
        else if (hovered) baseDistortion += 0.005
        _effectiveDistortion = baseDistortion
    }

    Behavior on _effectiveHighlight {
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }
    Behavior on _effectiveDistortion {
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }

    // ============================================================
    // 0. ♿ 无障碍降级备用层 (High-Contrast Accessible Mode)
    // ============================================================
    Rectangle {
        id: accessibleBed
        anchors.fill: parent
        radius: root.cornerRadius
        color: root.isDarkBackground ? "#111827" : "#f3f4f6"
        border.width: 2
        border.color: root.isDarkBackground ? "#38bdf8" : "#0284c7"
        visible: root.accessibleFallback
        z: 0
    }

    // ============================================================
    // 1. 苹果原生微晶实体底板 (Crystal Glass Bed)
    // ============================================================
    Rectangle {
        id: crystalBed
        anchors.fill: parent
        radius: root.cornerRadius
        color: root.tintColor
        opacity: Math.max(0.18, root.baseOpacity)
        visible: !root.accessibleFallback
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
    // 2. 背景捕获与安全回退纹理 (ShaderEffectSource)
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
        live: !root.accessibleFallback && root.visible
        visible: false
    }

    // ============================================================
    // 3. Liquid Glass 2.0 纯原生单 Pass 透镜光学着色器 (Qt 6 QSB)
    // 真实 SDF 圆角折射 + 三通道物理色散 + 传感器法线高光 + smin 液态融合 + 双材质
    // ============================================================
    ShaderEffect {
        id: glassShader
        anchors.fill: parent
        visible: !root.accessibleFallback
        z: 1

        property var source: bgCapture
        property real hasSource: root.backgroundSource ? 1.0 : 0.0

        property real time: root.animationTime
        property real opacity_: root.baseOpacity
        property color tint: root.tintColor
        property color edgeColor: root.edgeHighlightColor
        property vector2d pointer: Qt.vector2d(root._normalizedPointer.x, root._normalizedPointer.y)
        property vector2d resolution: Qt.vector2d(Math.max(root.width, 1), Math.max(root.height, 1))
        property real tintStr: root.tintStrength
        property real noise: root.noiseAmount
        property real distortion: root._effectiveDistortion
        property real highlight: root._effectiveHighlight
        property real fresnel: root.edgeFresnelPower
        property real hoverState: root.hovered ? 1.0 : 0.0
        property real pressState: root.pressed ? 1.0 : 0.0
        property real cornerRadius: root.cornerRadius
        property real lensMagnification: root.lensMagnification
        
        // Liquid Glass 2.0 新增统一属性
        property real materialStyle: root.materialVariant === LiquidGlassSurface.MaterialVariant.Clear ? 1.0 : 0.0
        property real dispersion: root.dispersion
        property real progressiveMode: root.progressiveMode
        property vector2d tilt: root.tilt
        property vector2d secondaryPos: root.secondaryPos
        property vector2d secondarySize: root.secondarySize
        property real secondaryRadius: root.secondaryRadius
        property real secondaryActive: root.secondaryActive
        property real sminFactor: root.sminFactor
        property real pressBulge: root.pressBulge
        property real _pad0: 0.0
        property real _pad1: 0.0

        vertexShader: "qrc:/shaders/default.vert.qsb"
        fragmentShader: "qrc:/shaders/liquid_glass_surface.frag.qsb"
    }

    // ============================================================
    // 4. 苹果凸透镜穹顶微弧光反光层 (Crescent Lens Specular Highlight)
    // ============================================================
    Rectangle {
        id: topSheen
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        height: Math.max(2, Math.round(parent.height * 0.44))
        radius: root.cornerRadius
        visible: !root.accessibleFallback
        z: 2

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Qt.rgba(1.0, 1.0, 1.0, root.pressed ? 0.35 : (root.hovered ? 0.24 : 0.16))
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
        visible: !root.accessibleFallback
        border.color: root.pressed 
                    ? Qt.rgba(1.0, 1.0, 1.0, 0.42) 
                    : (root.hovered ? Qt.rgba(1.0, 1.0, 1.0, 0.28) : Qt.rgba(1.0, 1.0, 1.0, 0.16))
        z: 3
    }

    // 内容容器
    Item {
        id: contentContainer
        anchors.fill: parent
        z: 10
    }
}
