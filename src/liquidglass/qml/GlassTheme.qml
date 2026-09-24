pragma Singleton
import QtQuick

QtObject {
    id: theme

    // 缩放基准单元（由 Main.qml 动态同步，完美适配各种分辨率和横竖屏）
    property real scaleUnit: 1.0

    // 是否为 Android 平台
    readonly property bool isAndroidPlatform: (typeof appController !== "undefined" && appController.isAndroid) || Qt.platform.os === "android"

    // 标准通用圆角系统
    readonly property int radiusChip: dp(10)
    readonly property int radiusCard: dp(14)
    readonly property int radiusPanel: dp(18)
    readonly property int radiusPopup: dp(22)

    // 统一液态毛玻璃调色板与材质规范
    readonly property color colorOverlayModal: Qt.rgba(0.02, 0.04, 0.07, 0.18)
    readonly property color colorPopupBg: Qt.rgba(0.07, 0.12, 0.18, 0.88)
    readonly property color colorPopupBorder: Qt.rgba(1.0, 1.0, 1.0, 0.16)
    readonly property color colorCardBg: Qt.rgba(1.0, 1.0, 1.0, 0.05)
    readonly property color colorCardBorder: Qt.rgba(1.0, 1.0, 1.0, 0.09)

    // 响应式尺寸换算
    function dp(value) {
        return Math.max(1, Math.round(value * scaleUnit))
    }

    // 响应式字号换算（带 Android 平台视觉补偿）
    function fs(value) {
        if (isAndroidPlatform) {
            return Math.max(11, Math.round(value * scaleUnit * 1.12))
        }
        return Math.max(10, Math.round(value * scaleUnit))
    }
}
