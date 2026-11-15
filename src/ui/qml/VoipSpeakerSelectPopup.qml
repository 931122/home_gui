import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0

GlassPopup {
    id: root
    title: qsTr("选择通话音箱")

    property real scaleUnit: Theme.scaleUnit
    property int cardRadius: Theme.radiusCard
    property int chipRadius: Theme.radiusChip
    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height

    signal dialogueRequested(string deviceId, string deviceModel, string deviceName)

    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.92, Theme.dp(isPortrait ? 400 : 460))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.90, Theme.dp(isPortrait ? 500 : 420))

    function dp(val) { return Theme.dp(val) }
    function fs(val) { return Theme.fs(val) }

    onAboutToShow: {
        if (typeof appController !== "undefined") {
            appController.refreshVoipContacts()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.dp(16)
        spacing: root.dp(12)

        // 顶部说明
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: root.dp(48)
            radius: root.cardRadius
            color: Qt.rgba(0.20, 0.60, 1.0, 0.12)
            border.color: Qt.rgba(0.40, 0.75, 1.0, 0.28)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.dp(12)
                anchors.rightMargin: root.dp(12)
                spacing: root.dp(10)

                Text {
                    text: "📞"
                    font.pixelSize: root.fs(18)
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("点击音箱即可发起双向实时语音通话：")
                    color: "#94a3b8"
                    font.pixelSize: root.fs(12)
                }

                GlassButton {
                    scaleUnit: root.scaleUnit
                    styleType: "neutral"
                    implicitWidth: root.dp(56)
                    implicitHeight: root.dp(28)
                    text: qsTr("刷新")
                    textPixelSize: root.fs(11)
                    onClicked: {
                        if (typeof appController !== "undefined") {
                            appController.refreshVoipContacts()
                        }
                    }
                }

                GlassButton {
                    scaleUnit: root.scaleUnit
                    styleType: "primary"
                    implicitWidth: root.dp(84)
                    implicitHeight: root.dp(28)
                    text: qsTr("📜 对话记录")
                    textPixelSize: root.fs(11)
                    onClicked: {
                        root.dialogueRequested("", "", "")
                        root.close()
                    }
                }
            }
        }

        // 音箱列表
        Flickable {
            id: speakersFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: speakersColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.DragAndOvershootBounds

            ColumnLayout {
                id: speakersColumn
                width: speakersFlickable.width
                spacing: root.dp(10)

                Repeater {
                    model: (typeof appController !== "undefined" && appController.voipSpeakers)
                           ? appController.voipSpeakers
                           : []

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: root.dp(64)
                        radius: root.cardRadius
                        color: Qt.rgba(0.12, 0.18, 0.28, 0.65)
                        border.color: modelData.isOnline ? Qt.rgba(0.30, 0.70, 1.0, 0.35) : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: root.dp(10)
                            spacing: root.dp(12)

                            // 头像
                            Rectangle {
                                width: root.dp(44)
                                height: root.dp(44)
                                radius: width / 2
                                color: Qt.rgba(0.15, 0.25, 0.38, 0.8)
                                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.15)
                                border.width: 1

                                Image {
                                    anchors.centerIn: parent
                                    width: parent.width * 0.75
                                    height: parent.height * 0.75
                                    fillMode: Image.PreserveAspectFit
                                    source: (modelData.avatar && modelData.avatar.length > 0)
                                            ? modelData.avatar
                                            : "qrc:/icons/mic.svg"
                                    mipmap: true
                                }
                            }

                            // 标题与型号状态
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                spacing: root.dp(2)

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: modelData.deviceName || qsTr("未知音箱")
                                    color: "#f8fafc"
                                    font.pixelSize: root.fs(14)
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: root.dp(6)

                                    Rectangle {
                                        width: root.dp(6)
                                        height: root.dp(6)
                                        radius: width / 2
                                        color: modelData.isOnline ? "#22c55e" : "#94a3b8"
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: (modelData.deviceModel || "") + (modelData.isOnline ? qsTr(" · 在线") : qsTr(" · 离线"))
                                        color: modelData.isOnline ? "#4ade80" : "#94a3b8"
                                        font.pixelSize: root.fs(11)
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            // 发起通话与对讲按钮组
                            RowLayout {
                                spacing: root.dp(6)

                                GlassButton {
                                    scaleUnit: root.scaleUnit
                                    styleType: modelData.isOnline ? "primary" : "neutral"
                                    implicitWidth: root.dp(72)
                                    implicitHeight: root.dp(36)
                                    text: qsTr("📢 对讲")
                                    textPixelSize: root.fs(12)
                                    disabled: !modelData.isOnline

                                    onClicked: {
                                        if (typeof appController !== "undefined") {
                                            appController.startDirectIntercom(modelData.deviceId,
                                                                             modelData.deviceName,
                                                                             modelData.avatar,
                                                                             modelData.uid || "",
                                                                             modelData.deviceModel || "",
                                                                             modelData.deviceType || 0,
                                                                             modelData.did || "")
                                        }
                                        root.close()
                                    }
                                }

                                GlassButton {
                                    scaleUnit: root.scaleUnit
                                    styleType: "neutral"
                                    implicitWidth: root.dp(64)
                                    implicitHeight: root.dp(36)
                                    text: qsTr("📞 电话")
                                    textPixelSize: root.fs(12)
                                    disabled: !modelData.isOnline

                                    onClicked: {
                                        if (typeof appController !== "undefined") {
                                            appController.startVoipCall(modelData.deviceId,
                                                                       modelData.deviceName,
                                                                       modelData.avatar,
                                                                       modelData.uid || "",
                                                                       modelData.deviceModel || "",
                                                                       modelData.deviceType || 0,
                                                                       modelData.did || "")
                                        }
                                        root.close()
                                    }
                                }

                                GlassButton {
                                    scaleUnit: root.scaleUnit
                                    styleType: "neutral"
                                    implicitWidth: root.dp(56)
                                    implicitHeight: root.dp(36)
                                    text: qsTr("📜 记录")
                                    textPixelSize: root.fs(11)

                                    onClicked: {
                                        root.dialogueRequested(modelData.deviceId || "",
                                                              modelData.deviceModel || "",
                                                              modelData.deviceName || "")
                                        root.close()
                                    }
                                }
                            }
                        }
                    }
                }

                // 空数据提示
                Item {
                    Layout.fillWidth: true
                    implicitHeight: root.dp(80)
                    visible: !appController || !appController.voipSpeakers || appController.voipSpeakers.length === 0

                    Column {
                        anchors.centerIn: parent
                        spacing: root.dp(6)

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("正在加载或未发现可通话的小爱音箱...")
                            color: "#94a3b8"
                            font.pixelSize: root.fs(12)
                        }
                    }
                }
            }
        }
    }
}
