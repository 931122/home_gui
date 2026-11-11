import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0

Item {
    id: root

    property real scaleUnit: Theme.scaleUnit
    function dp(val) { return Theme.dp(val) }
    function fs(val) { return Theme.fs(val) }

    readonly property bool isRecording: typeof appController !== "undefined" && appController.intercomRecording
    readonly property real audioLevel: (typeof appController !== "undefined" && appController.intercomAudioLevel) ? appController.intercomAudioLevel : 0.0
    readonly property int recordDuration: (typeof appController !== "undefined" && appController.intercomRecordDuration) ? appController.intercomRecordDuration : 0
    readonly property bool hasVoip: typeof appController !== "undefined" && appController.voipAvailable

    signal roomSelectRequested(real durationMs)
    signal speakerSelectRequested()

    width: isRecording ? root.dp(168) : (hasVoip ? root.dp(112) : root.dp(56))
    height: root.dp(56)

    Behavior on width {
        NumberAnimation { duration: 220; easing.type: Easing.OutBack }
    }

    // 录音外圈音频电平呼吸光晕 (动态跟随麦克风输入响度)
    Rectangle {
        anchors.centerIn: parent
        width: parent.width + root.dp(16) + (root.audioLevel * root.dp(24))
        height: parent.height + root.dp(16) + (root.audioLevel * root.dp(24))
        radius: height / 2
        color: Qt.rgba(0.22, 0.65, 1.0, 0.22 + root.audioLevel * 0.35)
        visible: root.isRecording
        z: -1

        Behavior on width { NumberAnimation { duration: 80 } }
        Behavior on height { NumberAnimation { duration: 80 } }
        Behavior on color { ColorAnimation { duration: 80 } }
    }

    // 液态微晶毛玻璃按钮底座
    LiquidGlassSurface {
        id: surface
        anchors.fill: parent
        cornerRadius: height / 2
        materialVariant: LiquidGlassSurface.MaterialVariant.Clear
        tintColor: root.isRecording ? Qt.rgba(0.15, 0.50, 1.0, 0.55) : Qt.rgba(0.12, 0.18, 0.28, 0.65)
        tintStrength: root.isRecording ? 0.6 : 0.3
        highlightIntensity: root.isRecording ? 1.6 : 1.2
        edgeHighlightEnabled: true
        edgeHighlightColor: root.isRecording ? Qt.rgba(0.40, 0.75, 1.0, 0.8) : Qt.rgba(1.0, 1.0, 1.0, 0.35)
        z: 0
    }

    // 边缘微晶描边
    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: "transparent"
        border.width: 1
        border.color: root.isRecording ? Qt.rgba(0.40, 0.75, 1.0, 0.60) : Qt.rgba(1.0, 1.0, 1.0, 0.20)
        z: 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.dp(12)
        anchors.rightMargin: root.dp(12)
        spacing: root.dp(8)
        z: 2

        // 麦克风区域 (录音/对讲)
        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: root.isRecording ? (parent.width - root.dp(24)) : (root.hasVoip ? root.dp(40) : root.dp(32))

            RowLayout {
                anchors.fill: parent
                spacing: root.dp(8)

                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    width: root.dp(30)
                    height: root.dp(30)
                    radius: width / 2
                    color: root.isRecording ? Qt.rgba(1.0, 1.0, 1.0, 0.30) : Qt.rgba(0.20, 0.60, 1.0, 0.25)

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(16)
                        height: root.dp(16)
                        source: "qrc:/icons/mic.svg"
                        sourceSize: Qt.size(width, height)
                    }

                    // 录音闪烁红点
                    Rectangle {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        width: root.dp(8)
                        height: root.dp(8)
                        radius: width / 2
                        color: "#ef4444"
                        visible: root.isRecording

                        SequentialAnimation on opacity {
                            running: root.isRecording
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.2; duration: 600 }
                            NumberAnimation { from: 0.2; to: 1.0; duration: 600 }
                        }
                    }
                }

                // 录音状态文字与时长
                Column {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.isRecording
                    spacing: root.dp(2)

                    Text {
                        text: qsTr("录音中 %1s").arg(root.recordDuration)
                        color: "#ffffff"
                        font.pixelSize: root.fs(12)
                        font.bold: true
                    }

                    Text {
                        text: qsTr("点击发送对讲")
                        color: "#bae6fd"
                        font.pixelSize: root.fs(10)
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (typeof appController === "undefined") return
                    if (!root.isRecording) {
                        appController.startIntercomRecord()
                    } else {
                        appController.stopIntercomRecord()
                    }
                }
            }
        }

        // 分割线 (仅在未录音且有 VoIP 时显示)
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 1
            Layout.preferredHeight: root.dp(20)
            color: Qt.rgba(1.0, 1.0, 1.0, 0.20)
            visible: !root.isRecording && root.hasVoip
        }

        // 电话通话图标 (仅在未录音且有 VoIP 时显示)
        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: root.dp(40)
            visible: !root.isRecording && root.hasVoip

            Rectangle {
                anchors.centerIn: parent
                width: root.dp(30)
                height: root.dp(30)
                radius: width / 2
                color: Qt.rgba(0.15, 0.70, 0.40, 0.25)

                Text {
                    anchors.centerIn: parent
                    text: "📞"
                    font.pixelSize: root.fs(14)
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    root.speakerSelectRequested()
                }
            }
        }
    }
}
