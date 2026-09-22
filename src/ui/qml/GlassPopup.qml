import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import HomeGui 1.0

/**
 * 液态毛玻璃通用基础弹窗 (GlassPopup)
 * 统一承载：
 * 1. 苹果液态磨砂双光源背景（左上冰蓝漫射 + 右下紫晶漫射 + 顶部折射晶线）
 * 2. 柔和暗夜微光背景遮罩
 * 3. 顶部跟手平移下滑把手（支持向下滑拽关闭）
 * 4. 顶部标题栏与关闭按钮
 */
Popup {
    id: root

    property string title: ""
    property int titlePixelSize: Theme.fs(21)
    property int panelRadius: Theme.radiusPopup
    property bool showHeader: true
    property bool showCloseButton: true
    property bool enablePullDownClose: true
    property real dragOffsetY: 0
    property bool isDraggingDown: false

    modal: true
    focus: true
    padding: 0
    topPadding: root.showHeader ? Theme.dp(56) : Theme.dp(16)
    x: Overlay.overlay ? Math.round((Overlay.overlay.width - width) / 2) : 0
    y: (Overlay.overlay ? Math.round((Overlay.overlay.height - height) / 2) : 0) + Math.max(0, root.dragOffsetY)

    Behavior on dragOffsetY {
        enabled: !root.isDraggingDown
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }

    Timer {
        id: autoCloseTimer
        interval: 180
        repeat: false
        onTriggered: {
            root.close()
            root.dragOffsetY = 0
            root.isDraggingDown = false
        }
    }

    onClosed: {
        root.dragOffsetY = 0
        root.isDraggingDown = false
    }

    Overlay.modal: Rectangle {
        color: Theme.colorOverlayModal
    }

    background: Rectangle {
        radius: root.panelRadius
        color: Theme.colorPopupBg
        border.color: Theme.colorPopupBorder
        border.width: 1
        clip: true

        // 左上冰蓝流体漫反射光晕
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: -Theme.dp(40)
            width: Theme.dp(200)
            height: Theme.dp(200)
            radius: width / 2
            color: Qt.rgba(0.20, 0.58, 0.95, 0.14)
        }

        // 右下紫晶微暖光晕
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: -Theme.dp(40)
            width: Theme.dp(180)
            height: Theme.dp(180)
            radius: width / 2
            color: Qt.rgba(0.42, 0.22, 0.68, 0.09)
        }

        // 顶部 1px 月白光折射线
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: root.panelRadius
            anchors.rightMargin: root.panelRadius
            height: 1
            color: "#ffffff"
            opacity: 0.22
        }

        // 顶部居中下滑把手指示条
        Item {
            id: handleBar
            visible: root.enablePullDownClose
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.dp(160)
            height: Theme.dp(20)
            z: 99

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: Theme.dp(5)
                width: Theme.dp(44)
                height: Theme.dp(4)
                radius: Theme.dp(2)
                color: "#ffffff"
                opacity: swipeDownArea.containsPress ? 0.70 : 0.28
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }

            MouseArea {
                id: swipeDownArea
                anchors.fill: parent
                anchors.margins: -Theme.dp(8)
                property real startY: 0
                onPressed: {
                    startY = mouse.y
                    root.isDraggingDown = true
                }
                onPositionChanged: {
                    var dy = mouse.y - startY
                    if (dy > 0) {
                        root.dragOffsetY = dy
                    } else {
                        root.dragOffsetY = dy * 0.2
                    }
                }
                onReleased: {
                    root.isDraggingDown = false
                    if (root.dragOffsetY > Theme.dp(55)) {
                        root.dragOffsetY = root.height
                        autoCloseTimer.start()
                    } else {
                        root.dragOffsetY = 0
                    }
                }
                onCanceled: {
                    root.isDraggingDown = false
                    root.dragOffsetY = 0
                }
            }
        }

        // 顶部标题行
        Item {
            id: headerRow
            visible: root.showHeader
            anchors.top: parent.top
            anchors.topMargin: Theme.dp(16)
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.dp(18)
            anchors.rightMargin: Theme.dp(18)
            height: visible ? Theme.dp(36) : 0
            z: 10

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                color: "#f4f8fb"
                font.pixelSize: root.titlePixelSize
                font.bold: true
                visible: text.length > 0
            }

            CloseButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: Theme.dp(34)
                implicitHeight: Theme.dp(34)
                iconSize: Theme.dp(15)
                visible: root.showCloseButton
                onClicked: root.close()
            }
        }
    }
}
