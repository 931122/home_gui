import QtQuick

/**
 * 屏幕熄灭纯黑防误触遮罩 (Tap to Wake Overlay)
 * 当屏幕进入灭屏状态时拦截所有底层 UI 触摸，
 * 手指轻触屏幕任意位置即刻唤醒恢复正常背光。
 */
Rectangle {
    id: root

    anchors.fill: parent
    color: "#000000"
    z: 99999

    readonly property bool isScreenOff: (typeof appController !== "undefined") ? appController.isScreenOff : false
    visible: isScreenOff

    // 吞噬所有 QML 层事件，防止任何形式的底层触控穿透
    MouseArea {
        anchors.fill: parent
        preventStealing: true
    }
}
