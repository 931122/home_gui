import QtQuick

Item {
    id: root

    // ============================================================
    // 公开属性 (Public Properties - 对标 OliverZhaohaibin/Qt-liquid-glass-widgets)
    // ============================================================

    // 背景捕获源（若未指定则启用独立微晶材质渲染）
    property Item backgroundSource: null

    // 材质参数（通透纯白微晶，彻底消除暗紫色杂色）
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
    // 列表滚动或外部移动同步驱动因子（用于在 Flickable 滑动时实时驱动背景采样更新）
    property real scrollSync: 0
    // 凸透镜物理放大率（0.0 为标准折射，>0 产生水滴凸透镜光学放大感）
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
        interval: 32 // ~30fps 节省算力并保持有机流动感
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
    // 背景捕获与安全回退纹理 (ShaderEffectSource - 杜绝非法纹理类型)
    // ============================================================
    readonly property rect _capturedRect: {
        var _sync = root.scrollSync // 显式绑定滚动与位移，滑动列表时每帧同步重新计算背景物理采样区域
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
        sourceItem: root.backgroundSource
        sourceRect: root._capturedRect
        textureSize: Qt.size(Math.max(1, root.width * 0.25), Math.max(1, root.height * 0.25))
        smooth: true
        anchors.fill: parent
        recursive: false
        live: root.visible && (root.backgroundSource !== null)
        visible: false
    }

    // 安全默认微晶底板（必须通过 ShaderEffectSource 提供合法的 OpenGL 纹理，杜绝驱动崩溃为洋红）
    Item {
        id: fallbackItem
        width: 64
        height: 64
        visible: false
        Rectangle {
            anchors.fill: parent
            color: "#16202e"
        }
    }

    ShaderEffectSource {
        id: fallbackCapture
        sourceItem: fallbackItem
        recursive: false
        visible: false
    }

    // ============================================================
    // 真实光学液态玻璃 Shader (Qt 6 RHI / GLSL 适配)
    // 包含：圆角 SDF 法线、引力透镜逆幂折射、双对称高光瓣、菲涅尔微晶边缘
    // ============================================================
    ShaderEffect {
        id: glassShader
        anchors.fill: parent

        property variant source: root.backgroundSource ? bgCapture : fallbackCapture
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

    // 物理圆角边缘保护遮罩（仅微弱保底边框，杜绝任何人工死白线）
    Rectangle {
        id: maskRect
        anchors.fill: parent
        radius: root.cornerRadius
        color: "transparent"
        border.color: Qt.rgba(1, 1, 1, 0.10)
        border.width: 1
    }

    // 内容容器
    Item {
        id: contentContainer
        anchors.fill: parent
    }
}
