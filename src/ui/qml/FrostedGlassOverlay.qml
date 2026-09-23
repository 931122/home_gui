import QtQuick
import QtQuick.Effects

Item {
    id: root
    anchors.fill: parent

    property var sourceItem: null
    property bool activeBlur: false
    property real maxRadius: 46

    // 动态显隐与流畅淡入淡出，不使用时完全脱离渲染管线，零功耗
    visible: activeBlur || opacity > 0.001
    opacity: activeBlur ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation {
            duration: 260
            easing.type: Easing.OutCubic
        }
    }

    // 1. Qt 6 官方原生 RHI 高斯毛玻璃核心层（直接接入 sourceItem，GPU 硬件级平滑虚化）
    MultiEffect {
        id: blurEffect
        anchors.fill: parent
        source: root.sourceItem
        autoPaddingEnabled: false
        blurEnabled: true
        blur: root.activeBlur ? 1.0 : 0.0
        blurMax: 48
        saturation: 0.15
        brightness: 0.02

        Behavior on blur {
            NumberAnimation {
                duration: root.activeBlur ? 260 : 180
                easing.type: Easing.OutCubic
            }
        }
    }

    // 2. 苹果暗夜微光物理吸收层（Dark Tint Absorber - 轻盈通透，不遮盖模糊光感）
    Rectangle {
        id: tintOverlay
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.05, 0.09, 0.30)
    }

    // 3. 苹果次表面晶莹漫反射层（Subsurface Specular Noise & Sheen）
    Rectangle {
        id: sheenOverlay
        anchors.fill: parent
        opacity: 0.40
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.10) }
            GradientStop { position: 0.4; color: Qt.rgba(1.0, 1.0, 1.0, 0.02) }
            GradientStop { position: 1.0; color: Qt.rgba(0.0, 0.0, 0.0, 0.15) }
        }
    }
}
