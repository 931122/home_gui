import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0

GlassPopup {
    id: root
    title: qsTr("发送语音对讲")

    property real audioDurationSeconds: 0
    property real scaleUnit: Theme.scaleUnit
    property int cardRadius: Theme.radiusCard
    property int chipRadius: Theme.radiusChip
    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height

    signal speakerSelectRequested()

    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.92, Theme.dp(isPortrait ? 400 : 460))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.90, Theme.dp(isPortrait ? 460 : 380))

    function dp(val) { return Theme.dp(val) }
    function fs(val) { return Theme.fs(val) }

    function openWithDuration(ms) {
        audioDurationSeconds = Math.max(0.5, ms / 1000.0)
        open()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.dp(16)
        spacing: root.dp(12)

        // 顶部录音概要卡片
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: root.dp(56)
            radius: root.cardRadius
            color: Qt.rgba(0.20, 0.60, 1.0, 0.12)
            border.color: Qt.rgba(0.40, 0.75, 1.0, 0.28)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.dp(14)
                anchors.rightMargin: root.dp(14)
                spacing: root.dp(12)

                Rectangle {
                    width: root.dp(36)
                    height: root.dp(36)
                    radius: width / 2
                    color: Qt.rgba(0.22, 0.65, 1.0, 0.25)

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(18)
                        height: root.dp(18)
                        source: "qrc:/icons/mic.svg"
                        sourceSize: Qt.size(width, height)
                    }
                }

                Column {
                    Layout.fillWidth: true
                    spacing: root.dp(2)

                    Text {
                        text: qsTr("录音完成 · 时长: %1 秒").arg(root.audioDurationSeconds.toFixed(1))
                        color: "#f0f6fa"
                        font.pixelSize: root.fs(14)
                        font.bold: true
                    }

                    Text {
                        text: qsTr("请选择接收广播或对讲的目标房间：")
                        color: "#94a3b8"
                        font.pixelSize: root.fs(11)
                    }
                }
            }
        }

        // 房间选择列表 (平滑滚动)
        Flickable {
            id: roomsFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: roomsFlow.implicitHeight
            clip: true
            boundsBehavior: Flickable.DragAndOvershootBounds

            Flow {
                id: roomsFlow
                width: roomsFlickable.width
                spacing: root.dp(10)

                Repeater {
                    model: (typeof appController !== "undefined" && appController.intercomRooms)
                           ? appController.intercomRooms
                           : []

                    delegate: GlassButton {
                        scaleUnit: root.scaleUnit
                        styleType: modelData.isWholeHouse ? "primary" : "neutral"
                        implicitHeight: root.dp(44)
                        implicitWidth: modelData.isWholeHouse
                                       ? roomsFlow.width
                                       : Math.max(root.dp(100), (roomsFlow.width - root.dp(10)) / 2)
                        text: (modelData.icon ? (modelData.icon + " ") : "") + modelData.roomName
                        textPixelSize: root.fs(13)

                        onClicked: {
                            if (typeof appController !== "undefined") {
                                appController.sendIntercomAudio(modelData.roomId, modelData.roomName)
                            }
                            root.close()
                        }
                    }
                }
            }
        }

        // 底部操作按钮
        RowLayout {
            Layout.fillWidth: true

            GlassButton {
                scaleUnit: root.scaleUnit
                styleType: "accent"
                implicitWidth: root.dp(120)
                implicitHeight: root.dp(34)
                text: qsTr("📞 改为音箱通话")
                textPixelSize: root.fs(12)
                visible: typeof appController !== "undefined" && appController.voipAvailable
                onClicked: {
                    if (typeof appController !== "undefined") {
                        appController.cancelIntercomRecord()
                    }
                    root.close()
                    root.speakerSelectRequested()
                }
            }

            Item { Layout.fillWidth: true }

            GlassButton {
                scaleUnit: root.scaleUnit
                styleType: "neutral"
                implicitWidth: root.dp(90)
                implicitHeight: root.dp(34)
                text: qsTr("放弃发送")
                textPixelSize: root.fs(12)
                onClicked: {
                    if (typeof appController !== "undefined") {
                        appController.cancelIntercomRecord()
                    }
                    root.close()
                }
            }
        }
    }
}
