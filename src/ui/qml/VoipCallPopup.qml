import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0

GlassPopup {
    id: root
    title: qsTr("音箱通话")

    property real scaleUnit: Theme.scaleUnit
    property int cardRadius: Theme.radiusCard
    property int chipRadius: Theme.radiusChip
    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height

    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.90, Theme.dp(isPortrait ? 380 : 420))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.90, Theme.dp(isPortrait ? 440 : 380))

    function dp(val) { return Theme.dp(val) }
    function fs(val) { return Theme.fs(val) }

    closePolicy: Popup.NoAutoClose

    // 监听通话状态，若挂断延迟 1.5 秒自动关闭弹窗
    Connections {
        target: typeof appController !== "undefined" ? appController : null
        function onVoipStateChanged() {
            if (typeof appController === "undefined") return
            if (appController.voipActive && !root.visible) {
                root.open()
            } else if (appController.voipState === 4 /* HANGUP */ || appController.voipState === 5 /* ERROR */) {
                autoCloseTimer.restart()
            }
        }
    }

    Timer {
        id: autoCloseTimer
        interval: 1500
        repeat: false
        onTriggered: {
            if (typeof appController !== "undefined" && !appController.voipActive) {
                root.close()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.dp(16)
        spacing: root.dp(14)

        Item { Layout.fillHeight: true }

        // 头像与呼叫声波扩散光晕
        Item {
            Layout.alignment: Qt.AlignHCenter
            width: root.dp(110)
            height: root.dp(110)

            // 扩散光波 1
            Rectangle {
                id: wave1
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                radius: width / 2
                color: Qt.rgba(0.22, 0.65, 1.0, 0.20)
                visible: typeof appController !== "undefined" && appController.voipActive && (appController.voipState === 1 || appController.voipState === 2 || appController.voipState === 3)

                SequentialAnimation on scale {
                    running: wave1.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 1.45; duration: 1600; easing.type: Easing.OutQuad }
                    NumberAnimation { from: 1.45; to: 1.0; duration: 0 }
                }
                SequentialAnimation on opacity {
                    running: wave1.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.6; to: 0.0; duration: 1600; easing.type: Easing.OutQuad }
                    NumberAnimation { from: 0.0; to: 0.6; duration: 0 }
                }
            }

            // 扩散光波 2
            Rectangle {
                id: wave2
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                radius: width / 2
                color: Qt.rgba(0.22, 0.65, 1.0, 0.15)
                visible: wave1.visible

                SequentialAnimation on scale {
                    running: wave2.visible
                    loops: Animation.Infinite
                    PauseAnimation { duration: 800 }
                    NumberAnimation { from: 1.0; to: 1.45; duration: 1600; easing.type: Easing.OutQuad }
                    NumberAnimation { from: 1.45; to: 1.0; duration: 0 }
                }
                SequentialAnimation on opacity {
                    running: wave2.visible
                    loops: Animation.Infinite
                    PauseAnimation { duration: 800 }
                    NumberAnimation { from: 0.5; to: 0.0; duration: 1600; easing.type: Easing.OutQuad }
                    NumberAnimation { from: 0.0; to: 0.5; duration: 0 }
                }
            }

            // 头像微晶底座
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: Qt.rgba(0.12, 0.20, 0.32, 0.75)
                border.color: (typeof appController !== "undefined" && appController.voipState === 3)
                              ? Qt.rgba(0.22, 0.75, 1.0, 0.8)
                              : Qt.rgba(1.0, 1.0, 1.0, 0.25)
                border.width: 2

                Image {
                    id: avatarImg
                    anchors.centerIn: parent
                    width: parent.width * 0.72
                    height: parent.height * 0.72
                    fillMode: Image.PreserveAspectFit
                    source: (typeof appController !== "undefined" && appController.voipTargetAvatar && appController.voipTargetAvatar.length > 0)
                            ? appController.voipTargetAvatar
                            : "qrc:/icons/mic.svg"
                    mipmap: true
                }
            }
        }

        // 设备名称与在线状态
        Column {
            Layout.alignment: Qt.AlignHCenter
            spacing: root.dp(4)

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: (typeof appController !== "undefined" && appController.voipTargetName && appController.voipTargetName.length > 0)
                      ? appController.voipTargetName
                      : qsTr("小爱音箱")
                color: "#ffffff"
                font.pixelSize: root.fs(18)
                font.bold: true
            }

            // 通话状态或计时器
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: {
                    if (typeof appController === "undefined") return ""
                    if (appController.voipState === 3 /* CONNECTED */) {
                        return appController.voipDurationText
                    }
                    return appController.voipStatusText || qsTr("连接中...")
                }
                color: (typeof appController !== "undefined" && appController.voipState === 3) ? "#38bdf8" : "#94a3b8"
                font.pixelSize: (typeof appController !== "undefined" && appController.voipState === 3) ? root.fs(22) : root.fs(13)
                font.bold: (typeof appController !== "undefined" && appController.voipState === 3)
                font.family: "Monospace"
            }
        }

        Item { Layout.fillHeight: true }

        // 通话控制功能按钮栏
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: root.dp(20)

            // 静音按键
            Column {
                spacing: root.dp(6)

                GlassButton {
                    scaleUnit: root.scaleUnit
                    styleType: (typeof appController !== "undefined" && appController.voipMuted) ? "accent" : "neutral"
                    implicitWidth: root.dp(56)
                    implicitHeight: root.dp(56)
                    cornerRadius: width / 2
                    isCapsule: false
                    text: (typeof appController !== "undefined" && appController.voipMuted) ? "🔇" : "🎙️"
                    textPixelSize: root.fs(20)

                    onClicked: {
                        if (typeof appController !== "undefined") {
                            appController.setVoipMute(!appController.voipMuted)
                        }
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: (typeof appController !== "undefined" && appController.voipMuted) ? qsTr("已静音") : qsTr("静音")
                    color: (typeof appController !== "undefined" && appController.voipMuted) ? "#38bdf8" : "#94a3b8"
                    font.pixelSize: root.fs(11)
                }
            }

            // 挂断按键 (红色醒目药丸)
            Column {
                spacing: root.dp(6)

                GlassButton {
                    scaleUnit: root.scaleUnit
                    styleType: "danger"
                    implicitWidth: root.dp(68)
                    implicitHeight: root.dp(68)
                    cornerRadius: width / 2
                    isCapsule: false
                    text: "📞"
                    textPixelSize: root.fs(24)

                    onClicked: {
                        if (typeof appController !== "undefined") {
                            appController.hangupVoipCall()
                        }
                        root.close()
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("挂断")
                    color: "#f87171"
                    font.pixelSize: root.fs(11)
                }
            }

            // 免提按键
            Column {
                spacing: root.dp(6)

                GlassButton {
                    scaleUnit: root.scaleUnit
                    styleType: (typeof appController !== "undefined" && appController.voipSpeakerphone) ? "primary" : "neutral"
                    implicitWidth: root.dp(56)
                    implicitHeight: root.dp(56)
                    cornerRadius: width / 2
                    isCapsule: false
                    text: (typeof appController !== "undefined" && appController.voipSpeakerphone) ? "🔊" : "🔈"
                    textPixelSize: root.fs(20)

                    onClicked: {
                        if (typeof appController !== "undefined") {
                            appController.setVoipSpeakerphone(!appController.voipSpeakerphone)
                        }
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: (typeof appController !== "undefined" && appController.voipSpeakerphone) ? qsTr("免提") : qsTr("听筒")
                    color: (typeof appController !== "undefined" && appController.voipSpeakerphone) ? "#38bdf8" : "#94a3b8"
                    font.pixelSize: root.fs(11)
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
