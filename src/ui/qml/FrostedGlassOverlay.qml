import QtQuick

Item {
    id: root
    anchors.fill: parent

    // ============================================================
    // 公开属性 (Public Properties)
    // ============================================================
    property var sourceItem: null
    property bool activeBlur: false
    property real maxRadius: 46

    // 平滑缓动过渡因子
    property real blurFactor: activeBlur ? 1.0 : 0.0
    Behavior on blurFactor {
        NumberAnimation {
            duration: 240
            easing.type: Easing.OutCubic
        }
    }

    // 动态显隐：完全脱离渲染管线以实现零功耗
    visible: activeBlur || blurFactor > 0.001
    opacity: blurFactor

    // ============================================================
    // 1. 底层场景离屏快速降采样捕获 (1/4 超采样 + 双线性平滑)
    // ============================================================
    Item {
        id: fallbackBackdrop
        anchors.fill: parent
        visible: false
        Rectangle {
            anchors.fill: parent
            color: "#182638"
        }
    }

    ShaderEffectSource {
        id: bgCapture
        anchors.fill: parent
        sourceItem: root.sourceItem ? root.sourceItem : fallbackBackdrop
        textureSize: Qt.size(Math.max(1, Math.round(root.width * 0.25)), Math.max(1, Math.round(root.height * 0.25)))
        smooth: true
        recursive: false
        live: root.activeBlur || root.blurFactor > 0.001
        visible: false
    }

    // ============================================================
    // 2. Qt 6 RHI 纯原生 9-Tap 高斯加权磨砂着色器 (Frosted Glass Shader)
    // ============================================================
    ShaderEffect {
        id: blurShader
        anchors.fill: parent
        z: 1

        property var source: bgCapture
        property vector2d resolution: Qt.vector2d(Math.max(root.width, 1), Math.max(root.height, 1))
        property real blurStrength: root.blurFactor
        property real _pad: 0.0

        vertexShader: "qrc:/shaders/default.vert.qsb"
        fragmentShader: "qrc:/shaders/frosted_glass.frag.qsb"
    }

    // ============================================================
    // 3. 苹果深空夜空磨砂物理基底 (iOS System Material Dark)
    // ============================================================
    Rectangle {
        id: darkMaterialBed
        anchors.fill: parent
        color: Qt.rgba(0.04, 0.08, 0.14, 0.55 * root.blurFactor)
        z: 2
    }

    // ============================================================
    // 4. 苹果次表面晶莹双光源漫反射层 (Aurora Specular Fields)
    // 模拟苹果厚玻璃在左上冰蓝与右下紫晶漫射光下的高级折射光感
    // ============================================================
    Rectangle {
        id: sheenOverlay
        anchors.fill: parent
        opacity: 0.50 * root.blurFactor
        z: 3
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0.18, 0.45, 0.85, 0.16) }
            GradientStop { position: 0.45; color: Qt.rgba(0.05, 0.15, 0.30, 0.03) }
            GradientStop { position: 1.0; color: Qt.rgba(0.40, 0.18, 0.65, 0.12) }
        }
    }

    // 顶部月白发丝高光微光漫射
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.max(1, Math.round(root.height * 0.30))
        z: 4
        opacity: 0.30 * root.blurFactor
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.12) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }
}
