import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

Popup {
    id: cameraPopupRoot

    property real scaleUnit: 1.0
    property int panelRadius: dp(22)
    property int cardRadius: dp(14)
    property int chipRadius: dp(10)
    property bool popupOpened: opened

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height
    modal: true
    focus: true
    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.96, dp(isPortrait ? 460 : 640))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.94, dp(isPortrait ? 560 : 380))
    anchors.centerIn: Overlay.overlay
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property real dragOffsetY: 0
    property bool isDraggingDown: false

    Behavior on dragOffsetY {
        enabled: !cameraPopupRoot.isDraggingDown
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }

    Timer {
        id: autoCloseTimer
        interval: 180
        repeat: false
        onTriggered: {
            cameraPopupRoot.close()
            cameraPopupRoot.dragOffsetY = 0
            cameraPopupRoot.isDraggingDown = false
        }
    }

    onClosed: {
        cameraPopupRoot.dragOffsetY = 0
        cameraPopupRoot.isDraggingDown = false
    }

    onOpened: appController.refreshCameraPreviews()

    Overlay.modal: Rectangle {
        color: Theme.colorOverlayModal
    }

    background: Rectangle {
        radius: cameraPopupRoot.panelRadius
        color: Qt.rgba(0.07, 0.12, 0.18, 0.88)
        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.16)
        border.width: 1
        clip: true

        // 左上冰蓝流体漫反射光晕
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: -cameraPopupRoot.dp(50)
            width: cameraPopupRoot.dp(220)
            height: cameraPopupRoot.dp(220)
            radius: width / 2
            color: Qt.rgba(0.20, 0.58, 0.95, 0.14)
        }

        // 右下紫晶微暖光晕
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: -cameraPopupRoot.dp(50)
            width: cameraPopupRoot.dp(200)
            height: cameraPopupRoot.dp(200)
            radius: width / 2
            color: Qt.rgba(0.42, 0.22, 0.68, 0.08)
        }

        // 顶部 1px 月白光折射线（避开两端大圆角）
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: cameraPopupRoot.panelRadius
            anchors.rightMargin: cameraPopupRoot.panelRadius
            height: 1
            color: "#ffffff"
            opacity: 0.22
        }
    }

    // 顶部居中下滑把手指示条 (支持向下滑动关闭)
    Item {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: cameraPopupRoot.dp(160)
        height: cameraPopupRoot.dp(20)
        z: 99

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: cameraPopupRoot.dp(5)
            width: cameraPopupRoot.dp(44)
            height: cameraPopupRoot.dp(4)
            radius: cameraPopupRoot.dp(2)
            color: "#ffffff"
            opacity: swipeDownArea.containsPress ? 0.70 : 0.28
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        MouseArea {
            id: swipeDownArea
            anchors.fill: parent
            anchors.margins: -cameraPopupRoot.dp(8)
            property real startY: 0
            onPressed: {
                startY = mouse.y
                cameraPopupRoot.isDraggingDown = true
            }
            onPositionChanged: {
                var dy = mouse.y - startY
                if (dy > 0) {
                    cameraPopupRoot.dragOffsetY = dy
                } else {
                    cameraPopupRoot.dragOffsetY = dy * 0.2
                }
            }
            onReleased: {
                cameraPopupRoot.isDraggingDown = false
                if (cameraPopupRoot.dragOffsetY > cameraPopupRoot.dp(55)) {
                    cameraPopupRoot.dragOffsetY = cameraPopupRoot.height
                    autoCloseTimer.start()
                } else {
                    cameraPopupRoot.dragOffsetY = 0
                }
            }
            onCanceled: {
                cameraPopupRoot.isDraggingDown = false
                cameraPopupRoot.dragOffsetY = 0
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: cameraPopupRoot.dp(16)
        spacing: cameraPopupRoot.dp(12)

        // 标题与关闭按钮
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: qsTr("监控摄像头切换")
                color: "#f4f8fb"
                font.pixelSize: cameraPopupRoot.fs(22)
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            CloseButton {
                implicitWidth: cameraPopupRoot.dp(36)
                implicitHeight: cameraPopupRoot.dp(36)
                iconSize: cameraPopupRoot.dp(16)
                onClicked: cameraPopupRoot.close()
            }
        }

        // 码流切换控制栏（Main / Minor 磨砂药丸）
        RowLayout {
            Layout.fillWidth: true
            visible: appController.onvifProfileSwitchSupported
            spacing: cameraPopupRoot.dp(8)

            Text {
                text: qsTr("码流选择")
                color: "#c2d6e3"
                font.pixelSize: cameraPopupRoot.fs(12)
                font.bold: true
            }

            // 主码流（苹果微光玻璃胶囊）
            Rectangle {
                implicitWidth: cameraPopupRoot.dp(70)
                implicitHeight: cameraPopupRoot.dp(28)
                radius: cameraPopupRoot.dp(8)
                readonly property bool isSelected: appController.onvifCurrentProfile === "main"
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: parent.isSelected
                               ? (mainStreamArea.pressed ? Qt.rgba(0.12, 0.58, 1.0, 0.95) : Qt.rgba(0.08, 0.52, 1.0, 0.88))
                               : (mainStreamArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.07))
                    }
                    GradientStop {
                        position: 1.0
                        color: parent.isSelected
                               ? (mainStreamArea.pressed ? Qt.rgba(0.02, 0.40, 0.90, 0.95) : Qt.rgba(0.0, 0.35, 0.80, 0.88))
                               : (mainStreamArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                    }
                }
                border.color: parent.isSelected
                              ? Qt.rgba(0.70, 0.90, 1.0, 0.85)
                              : (mainStreamArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.25) : Qt.rgba(1.0, 1.0, 1.0, 0.10))
                border.width: 1
                scale: mainStreamArea.pressed ? 0.94 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 90 } }

                MouseArea {
                    id: mainStreamArea
                    anchors.fill: parent
                    enabled: !parent.isSelected
                    onClicked: appController.selectOnvifProfile("main")
                }

                Text {
                    anchors.centerIn: parent
                    text: qsTr("主流")
                    color: parent.isSelected ? "#ffffff" : "#c5d7e5"
                    font.pixelSize: cameraPopupRoot.fs(11)
                    font.bold: true
                }
            }

            // 子码流（苹果微光玻璃胶囊）
            Rectangle {
                implicitWidth: cameraPopupRoot.dp(70)
                implicitHeight: cameraPopupRoot.dp(28)
                radius: cameraPopupRoot.dp(8)
                readonly property bool isSelected: appController.onvifCurrentProfile === "minor"
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: parent.isSelected
                               ? (minorStreamArea.pressed ? Qt.rgba(0.12, 0.58, 1.0, 0.95) : Qt.rgba(0.08, 0.52, 1.0, 0.88))
                               : (minorStreamArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.07))
                    }
                    GradientStop {
                        position: 1.0
                        color: parent.isSelected
                               ? (minorStreamArea.pressed ? Qt.rgba(0.02, 0.40, 0.90, 0.95) : Qt.rgba(0.0, 0.35, 0.80, 0.88))
                               : (minorStreamArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                    }
                }
                border.color: parent.isSelected
                              ? Qt.rgba(0.70, 0.90, 1.0, 0.85)
                              : (minorStreamArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.25) : Qt.rgba(1.0, 1.0, 1.0, 0.10))
                border.width: 1
                scale: minorStreamArea.pressed ? 0.94 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 90 } }

                MouseArea {
                    id: minorStreamArea
                    anchors.fill: parent
                    enabled: !parent.isSelected
                    onClicked: appController.selectOnvifProfile("minor")
                }

                Text {
                    anchors.centerIn: parent
                    text: qsTr("辅流")
                    color: parent.isSelected ? "#ffffff" : "#c5d7e5"
                    font.pixelSize: cameraPopupRoot.fs(11)
                    font.bold: true
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: appController.onvifCurrentProfile === "minor" ? qsTr("低负载模式") : qsTr("高清画质")
                color: "#85d8ff"
                font.pixelSize: cameraPopupRoot.fs(11)
            }
        }

        // 摄像头预览卡片网格
        GridView {
            id: previewGrid

            property var popup: cameraPopupRoot

            Layout.fillWidth: true
            Layout.fillHeight: true
            cellWidth: cameraPopupRoot.dp(196)
            cellHeight: cameraPopupRoot.dp(130)
            model: appController.cameraPreviewModels
            clip: true

            delegate: MouseArea {
                id: cameraDelegate

                property var popupRef: GridView.view ? GridView.view.popup : null
                property url previewSource: modelData.source || ""
                property bool useSharedPreview: modelData.shared || false
                property string sharedPreviewKey: modelData.sharedKey || ""
                property bool popupIsOpen: popupRef ? popupRef.popupOpened : false
                property bool previewConfigured: useSharedPreview
                                               ? (sharedPreviewKey.length > 0)
                                               : (previewSource.toString().length > 0)

                function dp(value) { return popupRef ? popupRef.dp(value) : value }
                function fs(value) { return popupRef ? popupRef.fs(value) : value }

                width: dp(186)
                height: dp(120)

                onClicked: {
                    appController.selectCamera(index)
                    if (popupRef) {
                        popupRef.close()
                    }
                }

                // 磨砂玻璃卡片背景
                Rectangle {
                    anchors.fill: parent
                    radius: cameraDelegate.popupRef ? cameraDelegate.popupRef.cardRadius : 14
                    color: index === appController.currentCameraIndex
                           ? Qt.rgba(0.0, 0.48, 1.0, 0.22)
                           : (cameraDelegate.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : Qt.rgba(1.0, 1.0, 1.0, 0.05))
                    border.color: index === appController.currentCameraIndex
                                  ? Qt.rgba(0.40, 0.78, 1.0, 0.70)
                                  : Qt.rgba(1.0, 1.0, 1.0, 0.09)
                    border.width: 1
                    scale: cameraDelegate.pressed ? 0.96 : 1.0
                    clip: true

                    Behavior on scale { NumberAnimation { duration: 90 } }
                    Behavior on color { ColorAnimation { duration: 100 } }

                    Rectangle {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: cameraDelegate.popupRef ? cameraDelegate.popupRef.cardRadius : 14
                        anchors.rightMargin: cameraDelegate.popupRef ? cameraDelegate.popupRef.cardRadius : 14
                        height: 1
                        color: "#ffffff"
                        opacity: index === appController.currentCameraIndex ? 0.35 : 0.10
                    }
                }

                // 监控视频渲染区
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: cameraDelegate.dp(8)
                    height: cameraDelegate.dp(76)
                    radius: cameraDelegate.popupRef ? cameraDelegate.popupRef.chipRadius : 10
                    color: Qt.rgba(0.02, 0.05, 0.08, 0.80)
                    border.color: Qt.rgba(1.0, 1.0, 1.0, 0.06)
                    border.width: 0.5
                    clip: true

                    VideoItem {
                        id: previewVideo
                        anchors.fill: parent
                        source: useSharedPreview ? "" : previewSource
                        backend: modelData.backend || "libav"
                        decoder: modelData.decoder || "auto"
                        sharedFrameKey: sharedPreviewKey
                        useSharedFrame: cameraDelegate.popupIsOpen && useSharedPreview && sharedPreviewKey.length > 0
                        autoPlay: cameraDelegate.popupIsOpen && !useSharedPreview && previewSource.toString().length > 0
                        networkOnline: appController.networkOnline
                        visible: useSharedPreview ? useSharedFrame : framePresented
                    }

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - cameraDelegate.dp(16)
                        text: !cameraDelegate.previewConfigured
                              ? qsTr("画面未配置")
                              : (!cameraDelegate.useSharedPreview && !previewVideo.framePresented
                                 ? qsTr("正在加载预览...")
                                 : "")
                        color: "#8fa9b8"
                        font.pixelSize: cameraDelegate.fs(10)
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }

                // 底部名称与状态指示灯
                Row {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: cameraDelegate.dp(10)
                    anchors.rightMargin: cameraDelegate.dp(10)
                    anchors.bottomMargin: cameraDelegate.dp(8)
                    spacing: cameraDelegate.dp(6)

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: cameraDelegate.dp(7)
                        height: cameraDelegate.dp(7)
                        radius: width / 2
                        color: index === appController.currentCameraIndex ? "#57c7ff" : Qt.rgba(1.0, 1.0, 1.0, 0.25)
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - cameraDelegate.dp(64)
                        text: modelData.name
                        color: "#eef7fb"
                        font.pixelSize: cameraDelegate.fs(11)
                        font.bold: index === appController.currentCameraIndex
                        elide: Text.ElideRight
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: index === appController.currentCameraIndex ? qsTr("正在直播") : qsTr("点击切换")
                        color: index === appController.currentCameraIndex ? "#79d2ff" : "#89a2b0"
                        font.pixelSize: cameraDelegate.fs(9)
                        font.bold: true
                    }
                }
            }
        }
    }
}
