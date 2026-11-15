import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0

GlassPopup {
    id: root
    title: qsTr("小爱音箱对话记录")

    property real scaleUnit: Theme.scaleUnit
    property int cardRadius: Theme.radiusCard
    property int chipRadius: Theme.radiusChip
    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height

    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.94, Theme.dp(isPortrait ? 420 : 580))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.92, Theme.dp(isPortrait ? 600 : 440))

    function dp(val) { return Theme.dp(val) }
    function fs(val) { return Theme.fs(val) }

    property string currentDeviceId: ""
    property string currentDeviceModel: ""
    property string currentDeviceName: ""

    function openWithSpeaker(deviceId, deviceModel, deviceName) {
        currentDeviceId = deviceId || ""
        currentDeviceModel = deviceModel || ""
        currentDeviceName = deviceName || ""
        open()
        refresh()
    }

    function refresh() {
        if (!currentDeviceId && appController && appController.voipSpeakers && appController.voipSpeakers.length > 0) {
            selectSpeaker(appController.voipSpeakers[0])
            return
        }
        if (typeof appController !== "undefined") {
            appController.fetchXiaoaiDialogue(currentDeviceId, currentDeviceModel, 0, false)
        }
    }

    function selectSpeaker(sp) {
        currentDeviceId = sp.deviceId || ""
        currentDeviceModel = sp.deviceModel || ""
        currentDeviceName = sp.deviceName || ""
        if (typeof appController !== "undefined") {
            appController.fetchXiaoaiDialogue(currentDeviceId, currentDeviceModel, 0, false)
        }
    }

    function loadMore() {
        if (typeof appController !== "undefined" && appController.xiaoaiDialogueNextTimestamp > 0) {
            appController.fetchXiaoaiDialogue(currentDeviceId, currentDeviceModel, appController.xiaoaiDialogueNextTimestamp, true)
        }
    }

    onAboutToShow: {
        if (!currentDeviceId && appController && appController.voipSpeakers && appController.voipSpeakers.length > 0) {
            selectSpeaker(appController.voipSpeakers[0])
        } else {
            refresh()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.dp(14)
        spacing: root.dp(10)

        // 顶部工具条：音箱切换横条 + 刷新按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: root.dp(8)

            // 音箱选择水平滑动条
            Flickable {
                id: speakerFlickable
                Layout.fillWidth: true
                implicitHeight: root.dp(34)
                contentWidth: speakerRow.implicitWidth
                contentHeight: height
                clip: true
                boundsBehavior: Flickable.DragAndOvershootBounds

                RowLayout {
                    id: speakerRow
                    spacing: root.dp(6)

                    Repeater {
                        model: (typeof appController !== "undefined" && appController.voipSpeakers)
                               ? appController.voipSpeakers
                               : []

                        delegate: Rectangle {
                            id: chipRect
                            readonly property bool isSelected: (modelData.deviceId === root.currentDeviceId)
                            implicitWidth: chipRow.implicitWidth + root.dp(16)
                            implicitHeight: root.dp(32)
                            radius: root.chipRadius
                            color: isSelected ? Qt.rgba(0.20, 0.60, 1.0, 0.35)
                                              : (chipMouse.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                                                         : Qt.rgba(1.0, 1.0, 1.0, 0.06))
                            border.color: isSelected ? Qt.rgba(0.40, 0.80, 1.0, 0.70)
                                                    : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                            border.width: 1

                            RowLayout {
                                id: chipRow
                                anchors.centerIn: parent
                                spacing: root.dp(6)

                                Rectangle {
                                    width: root.dp(6)
                                    height: root.dp(6)
                                    radius: width / 2
                                    color: modelData.isOnline ? "#22c55e" : "#94a3b8"
                                }

                                Text {
                                    text: modelData.deviceName || modelData.deviceModel || qsTr("未知音箱")
                                    color: chipRect.isSelected ? "#ffffff" : "#cbd5e1"
                                    font.pixelSize: root.fs(12)
                                    font.bold: chipRect.isSelected
                                }
                            }

                            MouseArea {
                                id: chipMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectSpeaker(modelData)
                            }
                        }
                    }
                }
            }

            // 刷新按钮
            GlassButton {
                scaleUnit: root.scaleUnit
                styleType: "neutral"
                implicitWidth: root.dp(64)
                implicitHeight: root.dp(32)
                text: qsTr("刷新")
                textPixelSize: root.fs(11)
                disabled: (typeof appController !== "undefined" && appController.xiaoaiDialogueLoading)
                onClicked: root.refresh()
            }
        }

        // 主内容区域：对话记录列表
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: root.cardRadius
            color: Qt.rgba(0.06, 0.10, 0.18, 0.50)
            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
            border.width: 1
            clip: true

            ListView {
                id: dialogueListView
                anchors.fill: parent
                anchors.margins: root.dp(10)
                spacing: root.dp(14)
                clip: true
                boundsBehavior: Flickable.DragAndOvershootBounds

                model: (typeof appController !== "undefined" && appController.xiaoaiDialogueRecords)
                       ? appController.xiaoaiDialogueRecords
                       : []

                delegate: ColumnLayout {
                    id: delegateItem
                    width: dialogueListView.width
                    spacing: root.dp(6)

                    readonly property bool hasXiaoaiReply: {
                        if (modelData.replySummary && modelData.replySummary.trim().length > 0) {
                            return true;
                        }
                        if (modelData.answers && modelData.answers.length > 0) {
                            for (var i = 0; i < modelData.answers.length; ++i) {
                                var a = modelData.answers[i];
                                if ((a.text && a.text.trim().length > 0) ||
                                    (a.title && a.title.trim().length > 0) ||
                                    (a.datetime && a.datetime.trim().length > 0)) {
                                    return true;
                                }
                            }
                        }
                        return false;
                    }

                    // 时间戳小徽章 (居中)
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: timeText.implicitWidth + root.dp(14)
                        implicitHeight: root.dp(18)
                        radius: root.dp(9)
                        color: Qt.rgba(0.0, 0.0, 0.0, 0.35)
                        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                        border.width: 1

                        Text {
                            id: timeText
                            anchors.centerIn: parent
                            text: modelData.timeStr || ""
                            color: "#94a3b8"
                            font.pixelSize: root.fs(10)
                        }
                    }

                    // 1. 用户说话气泡 (右侧对齐)
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignRight
                        spacing: root.dp(8)

                        Item { Layout.fillWidth: true } // 占位将气泡推向右侧

                        Rectangle {
                            Layout.maximumWidth: dialogueListView.width * 0.78
                            implicitWidth: userQueryText.implicitWidth + root.dp(20)
                            implicitHeight: userQueryText.implicitHeight + root.dp(16)
                            radius: root.chipRadius
                            color: Qt.rgba(0.12, 0.45, 0.88, 0.75)
                            border.color: Qt.rgba(0.35, 0.70, 1.0, 0.45)
                            border.width: 1

                            Text {
                                id: userQueryText
                                anchors.fill: parent
                                anchors.margins: root.dp(10)
                                text: modelData.query || ""
                                color: "#ffffff"
                                font.pixelSize: root.fs(13)
                                font.bold: true
                                wrapMode: Text.Wrap
                            }
                        }

                        // 用户头像图标
                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            width: root.dp(30)
                            height: root.dp(30)
                            radius: width / 2
                            color: Qt.rgba(0.20, 0.60, 1.0, 0.30)
                            border.color: Qt.rgba(0.40, 0.80, 1.0, 0.50)
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "🗣️"
                                font.pixelSize: root.fs(14)
                            }
                        }
                    }

                    // 2. 小爱音箱回答气泡 (左侧对齐，若无回复则不显示，不占位)
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft
                        spacing: root.dp(8)
                        visible: delegateItem.hasXiaoaiReply

                        // 小爱头像图标
                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            width: root.dp(30)
                            height: root.dp(30)
                            radius: width / 2
                            color: Qt.rgba(0.15, 0.70, 0.50, 0.30)
                            border.color: Qt.rgba(0.30, 0.90, 0.65, 0.50)
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "🤖"
                                font.pixelSize: root.fs(14)
                            }
                        }

                        Rectangle {
                            Layout.maximumWidth: dialogueListView.width * 0.82
                            implicitWidth: xiaoaiCol.implicitWidth + root.dp(22)
                            implicitHeight: xiaoaiCol.implicitHeight + root.dp(16)
                            radius: root.chipRadius
                            color: Qt.rgba(0.14, 0.20, 0.32, 0.80)
                            border.color: Qt.rgba(0.35, 0.55, 0.85, 0.25)
                            border.width: 1

                            ColumnLayout {
                                id: xiaoaiCol
                                anchors.fill: parent
                                anchors.margins: root.dp(10)
                                spacing: root.dp(6)

                                Text {
                                    Layout.fillWidth: true
                                    visible: (modelData.replySummary && modelData.replySummary.trim().length > 0)
                                    text: modelData.replySummary || ""
                                    color: "#f1f5f9"
                                    font.pixelSize: root.fs(13)
                                    wrapMode: Text.Wrap
                                }

                                // 如果有多条详细 directive / audio / alert 动作
                                Repeater {
                                    model: modelData.answers || []
                                    delegate: RowLayout {
                                        Layout.fillWidth: true
                                        spacing: root.dp(6)
                                        visible: modelData.type === "AUDIO" || modelData.type === "ALERT" || Boolean(modelData.title)

                                        Rectangle {
                                            implicitWidth: tagText.implicitWidth + root.dp(10)
                                            implicitHeight: root.dp(20)
                                            radius: root.dp(4)
                                            color: modelData.type === "AUDIO" ? Qt.rgba(0.85, 0.35, 0.20, 0.35)
                                                                              : Qt.rgba(0.30, 0.65, 1.0, 0.30)

                                            Text {
                                                id: tagText
                                                anchors.centerIn: parent
                                                text: modelData.type === "AUDIO" ? qsTr("🎵 音乐") : qsTr("⏰ 闹钟")
                                                color: "#ffffff"
                                                font.pixelSize: root.fs(10)
                                                font.bold: true
                                            }
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData.title ? (modelData.title + (modelData.artist ? " - " + modelData.artist : ""))
                                                                  : (modelData.datetime || "")
                                            color: "#cbd5e1"
                                            font.pixelSize: root.fs(11)
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }

                        Item { Layout.fillWidth: true } // 占位将气泡推向左侧
                    }
                }

                // 底部加载更多按钮
                footer: Item {
                    width: dialogueListView.width
                    implicitHeight: (typeof appController !== "undefined" && appController.xiaoaiDialogueNextTimestamp > 0)
                                    ? root.dp(48) : 0
                    visible: implicitHeight > 0

                    GlassButton {
                        anchors.centerIn: parent
                        scaleUnit: root.scaleUnit
                        styleType: "neutral"
                        implicitWidth: root.dp(140)
                        implicitHeight: root.dp(30)
                        text: (typeof appController !== "undefined" && appController.xiaoaiDialogueLoading)
                              ? qsTr("加载中...") : qsTr("加载更早对话...")
                        textPixelSize: root.fs(11)
                        disabled: (typeof appController !== "undefined" && appController.xiaoaiDialogueLoading)
                        onClicked: root.loadMore()
                    }
                }
            }

            // 加载中状态指示
            Rectangle {
                anchors.centerIn: parent
                width: root.dp(160)
                height: root.dp(56)
                radius: root.chipRadius
                color: Qt.rgba(0.08, 0.12, 0.20, 0.88)
                border.color: Qt.rgba(0.30, 0.70, 1.0, 0.30)
                border.width: 1
                visible: (typeof appController !== "undefined" && appController.xiaoaiDialogueLoading &&
                          (!appController.xiaoaiDialogueRecords || appController.xiaoaiDialogueRecords.length === 0))

                RowLayout {
                    anchors.centerIn: parent
                    spacing: root.dp(10)

                    BusyIndicator {
                        running: true
                        implicitWidth: root.dp(24)
                        implicitHeight: root.dp(24)
                    }

                    Text {
                        text: qsTr("正在加载对话...")
                        color: "#e2e8f0"
                        font.pixelSize: root.fs(12)
                    }
                }
            }

            // 空数据或错误提示
            ColumnLayout {
                anchors.centerIn: parent
                spacing: root.dp(8)
                visible: (typeof appController !== "undefined" && !appController.xiaoaiDialogueLoading &&
                          (!appController.xiaoaiDialogueRecords || appController.xiaoaiDialogueRecords.length === 0))

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: (appController && appController.xiaoaiDialogueError)
                          ? "⚠️ " + appController.xiaoaiDialogueError
                          : qsTr("📜 暂无对话记录")
                    color: (appController && appController.xiaoaiDialogueError) ? "#f87171" : "#94a3b8"
                    font.pixelSize: root.fs(13)
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("尝试向小爱音箱唤醒对话后，点击右上角刷新。")
                    color: "#64748b"
                    font.pixelSize: root.fs(11)
                }
            }
        }
    }
}
