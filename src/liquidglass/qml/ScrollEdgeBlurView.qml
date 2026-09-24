import QtQuick

Item {
    id: root

    // ============================================================
    // 公开属性 (Public API - ScrollEdgeBlurView)
    // ============================================================
    property Item backgroundSource: null
    
    // 渐进方向: "top" (顶部边缘渐变), "bottom" (底部边缘渐变)
    property string edge: "top"
    
    // 模糊强度与高度
    property real blurDepth: 40
    property int materialVariant: LiquidGlassSurface.MaterialVariant.Regular

    implicitHeight: blurDepth
    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined

    // 1. 纯原生透镜渐进模糊表面 (Progressive Blur Surface)
    LiquidGlassSurface {
        id: blurSurface
        anchors.fill: parent
        backgroundSource: root.backgroundSource
        materialVariant: root.materialVariant
        cornerRadius: 0
        baseOpacity: 0.55
        progressiveMode: root.edge === "bottom" ? 2 : 1
        tintColor: Qt.rgba(0.04, 0.08, 0.14, 0.25)
        tintStrength: 0.20
        highlightIntensity: 0.35
        noiseAmount: 0.012
        dispersion: 0.10
        edgeFresnelPower: 3.0
    }

    // 2. 微光发丝收口线
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: root.edge === "top" ? undefined : parent.top
        anchors.bottom: root.edge === "top" ? parent.bottom : undefined
        height: 1
        color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
    }
}
