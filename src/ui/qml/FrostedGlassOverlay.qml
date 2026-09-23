import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    id: root
    anchors.fill: parent

    property var sourceItem: null
    property bool activeBlur: false
    property real maxRadius: 46

    // 动态显隐与流畅淡入淡出，不使用时完全脱离渲染管线，零功耗
    visible: opacity > 0.001
    opacity: activeBlur ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation {
            duration: 260
            easing.type: Easing.OutCubic
        }
    }

    // 结合降采样优化 (Downsample Texture Source)
    // 采用 2x 降采样纹理，使 GPU 像素着色开销降低 75%，同时利用硬件线性滤波获得柔和二次平滑
    ShaderEffectSource {
        id: blurSource
        sourceItem: root.sourceItem
        width: Math.max(1, Math.round(root.width / 2))
        height: Math.max(1, Math.round(root.height / 2))
        sourceRect: Qt.rect(0, 0, root.sourceItem ? root.sourceItem.width : root.width, root.sourceItem ? root.sourceItem.height : root.height)
        textureSize: Qt.size(width, height)
        smooth: true
        hideSource: false
        live: root.visible && root.activeBlur
    }

    // 1. 苹果原味动态高斯模糊核心层 (Qt 6 官方 Qt5Compat 硬件加速层)
    FastBlur {
        id: blurEffect
        anchors.fill: parent
        source: blurSource
        radius: root.activeBlur ? root.maxRadius : 0
        cached: false

        Behavior on radius {
            NumberAnimation {
                duration: root.activeBlur ? 260 : 180
                easing.type: Easing.OutCubic
            }
        }
    }

    // 2. 苹果深空夜空磨砂基底（iOS System Material Dark 调色）
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.05, 0.08, 0.52)
    }

    // 3. 苹果液态微光漫反射折射光场（呈现厚重高质感玻璃的折射色散）
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0.20, 0.50, 0.85, 0.12) }
            GradientStop { position: 0.45; color: Qt.rgba(0.05, 0.15, 0.30, 0.02) }
            GradientStop { position: 1.0; color: Qt.rgba(0.40, 0.20, 0.65, 0.08) }
        }
    }
}
