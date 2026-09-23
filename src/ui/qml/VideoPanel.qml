import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

Rectangle {
    id: panel

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)
    property bool fullscreen: false

    signal fullscreenToggleRequested()
    signal cameraSwipeRequested(int offset)

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    radius: fullscreen ? 0 : panelRadius
    color: fullscreen ? "black" : Qt.rgba(0.05, 0.09, 0.14, 0.78)
    border.color: fullscreen ? "transparent" : Qt.rgba(1, 1, 1, 0.12)
    border.width: fullscreen ? 0 : 1
    clip: true

    // 顶部月白玻璃边缘折射高光
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: panel.panelRadius
        anchors.rightMargin: panel.panelRadius
        height: 1
        color: "#ffffff"
        opacity: fullscreen ? 0.0 : 0.18
    }

    Loader {
        id: videoLoader
        anchors.fill: parent
        anchors.margins: fullscreen ? 0 : 1
        active: appController.videoEnabled
        source: active ? "qrc:/qml/VideoSurface.qml" : ""
    }

    MouseArea {
        id: gestureArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        property real pressX: 0
        property real pressY: 0
        property double pressTimestamp: 0
        property bool swipeLocked: false
        property real swipeThreshold: panel.dp(88)
        property real verticalTolerance: panel.dp(42)
        property int maxSwipeDuration: 650

        Timer {
            id: swipeCooldownTimer
            interval: 420
            repeat: false
            onTriggered: gestureArea.swipeLocked = false
        }

        onPressed: {
            pressX = mouse.x
            pressY = mouse.y
            pressTimestamp = Date.now()
        }
        onReleased: {
            const dx = mouse.x - pressX
            const dy = mouse.y - pressY
            const elapsed = Date.now() - pressTimestamp
            if (swipeLocked
                    || elapsed > maxSwipeDuration
                    || Math.abs(dx) < swipeThreshold
                    || Math.abs(dy) > verticalTolerance
                    || Math.abs(dx) <= Math.abs(dy) * 1.35) {
                return
            }
            swipeLocked = true
            swipeCooldownTimer.restart()
            panel.cameraSwipeRequested(dx < 0 ? 1 : -1)
        }
        onCanceled: pressTimestamp = 0
        onDoubleClicked: panel.fullscreenToggleRequested()
    }


    // 退出全屏悬浮胶囊（苹果微晶流光玻璃胶囊）
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: panel.dp(14)
        width: panel.dp(102)
        height: panel.dp(34)
        radius: panel.dp(17)
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: exitFullscreenArea.pressed ? Qt.rgba(0.92, 0.28, 0.28, 0.90) : Qt.rgba(0.14, 0.19, 0.28, 0.88)
            }
            GradientStop {
                position: 1.0
                color: exitFullscreenArea.pressed ? Qt.rgba(0.72, 0.16, 0.20, 0.90) : Qt.rgba(0.06, 0.09, 0.14, 0.88)
            }
        }
        border.color: exitFullscreenArea.pressed ? Qt.rgba(1.0, 0.55, 0.55, 0.85) : Qt.rgba(1.0, 1.0, 1.0, 0.22)
        border.width: 1
        scale: exitFullscreenArea.pressed ? 0.92 : 1.0
        clip: true
        opacity: fullscreen ? 1.0 : 0.0
        visible: opacity > 0.01
        z: 900

        Behavior on scale { NumberAnimation { duration: 90 } }

        RowLayout {
            anchors.centerIn: parent
            spacing: panel.dp(4)
            Image {
                source: "qrc:/icons/close.svg"
                width: panel.dp(12)
                height: panel.dp(12)
                sourceSize: Qt.size(width, height)
                smooth: true
            }
            Text {
                text: qsTr("退出全屏")
                color: "#ffffff"
                font.pixelSize: panel.fs(11)
                font.bold: true
            }
        }

        MouseArea {
            id: exitFullscreenArea
            anchors.fill: parent
            anchors.margins: -panel.dp(6)
            onClicked: panel.fullscreenToggleRequested()
        }
    }

    Rectangle {
        id: ptzPanel
        x: panelMargin
        y: panelMargin
        width: panel.dp(136)
        height: panel.dp(136)
        property real panelMargin: panel.dp(16)
        property bool userMoved: false
        radius: panel.cardRadius
        color: Qt.rgba(0.06, 0.11, 0.17, 0.82)
        border.color: Qt.rgba(1, 1, 1, 0.16)
        border.width: 1
        clip: true
        visible: globalState.onvifAvailable

        // 顶部微白玻璃边缘高光
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: panel.cardRadius
            anchors.rightMargin: panel.cardRadius
            height: 1
            color: "#ffffff"
            opacity: 0.22
        }

        function snapToBottomRight() {
            if (!parent) {
                return
            }
            x = Math.max(panelMargin, parent.width - width - panelMargin)
            y = Math.max(panelMargin, parent.height - height - panelMargin)
        }

        Component.onCompleted: snapToBottomRight()

        Connections {
            target: ptzPanel.parent
            function onWidthChanged() {
                if (ptzPanel.userMoved) {
                    ptzPanel.x = Math.max(ptzPanel.panelMargin,
                                          Math.min(ptzPanel.x, ptzPanel.parent.width - ptzPanel.width - ptzPanel.panelMargin))
                } else {
                    ptzPanel.snapToBottomRight()
                }
            }
            function onHeightChanged() {
                if (ptzPanel.userMoved) {
                    ptzPanel.y = Math.max(ptzPanel.panelMargin,
                                          Math.min(ptzPanel.y, ptzPanel.parent.height - ptzPanel.height - ptzPanel.panelMargin))
                } else {
                    ptzPanel.snapToBottomRight()
                }
            }
        }

        onXChanged: {
            if (!parent) {
                return
            }
            x = Math.max(panelMargin, Math.min(x, parent.width - width - panelMargin))
        }
        onYChanged: {
            if (!parent) {
                return
            }
            y = Math.max(panelMargin, Math.min(y, parent.height - height - panelMargin))
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.OpenHandCursor
            drag.target: parent
            drag.axis: Drag.XAndYAxis
            drag.minimumX: parent.panelMargin
            drag.maximumX: parent.parent.width - parent.width - parent.panelMargin
            drag.minimumY: parent.panelMargin
            drag.maximumY: parent.parent.height - parent.height - parent.panelMargin
            onPressed: parent.userMoved = true
        }

        property string activePtzDirection: ""

        Timer {
            id: ptzRepeatTimer
            interval: 2000
            repeat: true
            onTriggered: {
                if (ptzPanel.activePtzDirection.length > 0) {
                    appController.movePtz(ptzPanel.activePtzDirection)
                }
            }
        }

        function startPtz(dir) {
            activePtzDirection = dir
            appController.movePtz(dir)
            ptzRepeatTimer.restart()
        }

        function stopPtz() {
            activePtzDirection = ""
            ptzRepeatTimer.stop()
            appController.movePtz("stop")
        }

        GridLayout {
            anchors.centerIn: parent
            columns: 3
            rowSpacing: panel.dp(6)
            columnSpacing: panel.dp(6)
            z: 1

            Item { width: panel.dp(34); height: panel.dp(34) }

            // 上（苹果微光玻璃纽扣）
            Rectangle {
                implicitWidth: panel.dp(34)
                implicitHeight: panel.dp(34)
                radius: panel.dp(8)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: upKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.28) : (upKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.09))
                    }
                    GradientStop {
                        position: 1.0
                        color: upKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : (upKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.06) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                    }
                }
                border.color: upKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.45) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                border.width: 1
                scale: upKeyArea.pressed ? 0.90 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 80 } }

                Canvas {
                    anchors.centerIn: parent
                    width: panel.dp(12)
                    height: panel.dp(10)
                    readonly property color arrowColor: globalState.ptzAvailable ? (upKeyArea.pressed ? "#ffffff" : "#e0effa") : "#526573"
                    onArrowColorChanged: requestPaint()
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.fillStyle = arrowColor
                        ctx.beginPath()
                        ctx.moveTo(width / 2, 0)
                        ctx.lineTo(width, height)
                        ctx.lineTo(0, height)
                        ctx.closePath()
                        ctx.fill()
                    }
                }

                MouseArea {
                    id: upKeyArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: globalState.ptzAvailable
                    onPressed: ptzPanel.startPtz("up")
                    onReleased: ptzPanel.stopPtz()
                    onCanceled: ptzPanel.stopPtz()
                }
            }

            Item { width: panel.dp(34); height: panel.dp(34) }

            // 左（苹果微光玻璃纽扣）
            Rectangle {
                implicitWidth: panel.dp(34)
                implicitHeight: panel.dp(34)
                radius: panel.dp(8)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: leftKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.28) : (leftKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.09))
                    }
                    GradientStop {
                        position: 1.0
                        color: leftKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : (leftKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.06) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                    }
                }
                border.color: leftKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.45) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                border.width: 1
                scale: leftKeyArea.pressed ? 0.90 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 80 } }

                Canvas {
                    anchors.centerIn: parent
                    width: panel.dp(10)
                    height: panel.dp(12)
                    readonly property color arrowColor: globalState.ptzAvailable ? (leftKeyArea.pressed ? "#ffffff" : "#e0effa") : "#526573"
                    onArrowColorChanged: requestPaint()
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.fillStyle = arrowColor
                        ctx.beginPath()
                        ctx.moveTo(0, height / 2)
                        ctx.lineTo(width, 0)
                        ctx.lineTo(width, height)
                        ctx.closePath()
                        ctx.fill()
                    }
                }

                MouseArea {
                    id: leftKeyArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: globalState.ptzAvailable
                    onPressed: ptzPanel.startPtz("left")
                    onReleased: ptzPanel.stopPtz()
                    onCanceled: ptzPanel.stopPtz()
                }
            }

            // 停（苹果微光玻璃纽扣）
            Rectangle {
                implicitWidth: panel.dp(34)
                implicitHeight: panel.dp(34)
                radius: panel.dp(8)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: stopKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.28) : (stopKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.09))
                    }
                    GradientStop {
                        position: 1.0
                        color: stopKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : (stopKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.06) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                    }
                }
                border.color: stopKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.45) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                border.width: 1
                scale: stopKeyArea.pressed ? 0.90 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 80 } }

                Rectangle {
                    anchors.centerIn: parent
                    width: panel.dp(9)
                    height: panel.dp(9)
                    radius: panel.dp(2)
                    color: globalState.ptzAvailable ? (stopKeyArea.pressed ? "#ffffff" : "#e0effa") : "#526573"
                }

                MouseArea {
                    id: stopKeyArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: globalState.ptzAvailable
                    onClicked: ptzPanel.stopPtz()
                }
            }

            // 右（苹果微光玻璃纽扣）
            Rectangle {
                implicitWidth: panel.dp(34)
                implicitHeight: panel.dp(34)
                radius: panel.dp(8)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: rightKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.28) : (rightKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.09))
                    }
                    GradientStop {
                        position: 1.0
                        color: rightKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : (rightKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.06) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                    }
                }
                border.color: rightKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.45) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                border.width: 1
                scale: rightKeyArea.pressed ? 0.90 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 80 } }

                Canvas {
                    anchors.centerIn: parent
                    width: panel.dp(10)
                    height: panel.dp(12)
                    readonly property color arrowColor: globalState.ptzAvailable ? (rightKeyArea.pressed ? "#ffffff" : "#e0effa") : "#526573"
                    onArrowColorChanged: requestPaint()
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.fillStyle = arrowColor
                        ctx.beginPath()
                        ctx.moveTo(width, height / 2)
                        ctx.lineTo(0, 0)
                        ctx.lineTo(0, height)
                        ctx.closePath()
                        ctx.fill()
                    }
                }

                MouseArea {
                    id: rightKeyArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: globalState.ptzAvailable
                    onPressed: ptzPanel.startPtz("right")
                    onReleased: ptzPanel.stopPtz()
                    onCanceled: ptzPanel.stopPtz()
                }
            }

            Item { width: panel.dp(34); height: panel.dp(34) }

            // 下（苹果微光玻璃纽扣）
            Rectangle {
                implicitWidth: panel.dp(34)
                implicitHeight: panel.dp(34)
                radius: panel.dp(8)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: downKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.28) : (downKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.09))
                    }
                    GradientStop {
                        position: 1.0
                        color: downKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : (downKeyArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.06) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                    }
                }
                border.color: downKeyArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.45) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                border.width: 1
                scale: downKeyArea.pressed ? 0.90 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 80 } }

                Canvas {
                    anchors.centerIn: parent
                    width: panel.dp(12)
                    height: panel.dp(10)
                    readonly property color arrowColor: globalState.ptzAvailable ? (downKeyArea.pressed ? "#ffffff" : "#e0effa") : "#526573"
                    onArrowColorChanged: requestPaint()
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.fillStyle = arrowColor
                        ctx.beginPath()
                        ctx.moveTo(width / 2, height)
                        ctx.lineTo(width, 0)
                        ctx.lineTo(0, 0)
                        ctx.closePath()
                        ctx.fill()
                    }
                }

                MouseArea {
                    id: downKeyArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: globalState.ptzAvailable
                    onPressed: ptzPanel.startPtz("down")
                    onReleased: ptzPanel.stopPtz()
                    onCanceled: ptzPanel.stopPtz()
                }
            }

            Item { width: panel.dp(34); height: panel.dp(34) }
        }
    }
}
