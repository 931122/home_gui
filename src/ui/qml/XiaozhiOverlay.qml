import QtQuick
import QtQuick.Layouts
import HomeGui 1.0

Item {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(22)
    property bool overlayVisible: globalState.xiaozhiOverlayVisible
    property string stateText: globalState.xiaozhiState
    property string messageText: globalState.xiaozhiText
    property string statusText: globalState.xiaozhiStatus

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    visible: globalState.xiaozhiAvailable || overlayVisible
    z: 90

    // 全屏半透明背景遮罩
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.02, 0.04, 0.07, 0.60)
        opacity: overlayVisible ? 1.0 : 0.0
        visible: overlayVisible || opacity > 0.01

        Behavior on opacity {
            NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: appController.hideXiaozhiOverlay()
        }
    }

    // 苹果 Siri 风格悬浮对话卡片
    Rectangle {
        id: panel
        width: Math.min(parent.width - root.dp(48), root.dp(440))
        height: root.dp(186)
        radius: root.panelRadius
        color: Qt.rgba(0.08, 0.13, 0.19, 0.92)
        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.18)
        border.width: 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: root.dp(84)
        opacity: overlayVisible ? 1.0 : 0.0
        visible: overlayVisible || opacity > 0.01
        clip: true

        Behavior on opacity {
            NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
        }

        // 次表面呼吸光晕（回答时翠绿，倾听时冰蓝）
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: -root.dp(40)
            width: root.dp(180)
            height: root.dp(180)
            radius: width / 2
            color: stateText === "回答" ? Qt.rgba(0.25, 0.85, 0.40, 0.16) : Qt.rgba(0.20, 0.58, 0.95, 0.16)

            Behavior on color { ColorAnimation { duration: 250 } }
        }


        // 左侧指示呼吸竖条
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.dp(5)
            color: stateText === "回答" ? "#5ce87e" : "#5eccff"

            Behavior on color { ColorAnimation { duration: 200 } }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.dp(18)
            spacing: root.dp(10)

            // 对话头部
            RowLayout {
                Layout.fillWidth: true
                spacing: root.dp(12)

                // AI 磨砂光球头像
                Rectangle {
                    Layout.preferredWidth: root.dp(46)
                    Layout.preferredHeight: root.dp(46)
                    radius: width / 2
                    color: stateText === "回答" ? Qt.rgba(0.25, 0.85, 0.40, 0.22) : Qt.rgba(0.20, 0.58, 0.95, 0.22)
                    border.color: stateText === "回答" ? Qt.rgba(0.35, 0.90, 0.50, 0.60) : Qt.rgba(0.40, 0.75, 1.0, 0.60)
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 200 } }

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(22)
                        height: root.dp(22)
                        sourceSize.width: root.dp(22)
                        sourceSize.height: root.dp(22)
                        source: stateText === "回答" ? "qrc:/icons/soundwave.svg" : "qrc:/icons/mic.svg"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: root.dp(2)

                    Text {
                        Layout.fillWidth: true
                        text: stateText.length > 0 ? stateText : qsTr("小智")
                        color: "#f4f8fb"
                        font.pixelSize: root.fs(20)
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: statusText
                        color: "#8aa5b7"
                        font.pixelSize: root.fs(11)
                        elide: Text.ElideRight
                    }
                }

                CloseButton {
                    Layout.preferredWidth: root.dp(32)
                    Layout.preferredHeight: root.dp(32)
                    iconSize: root.dp(14)
                    onClicked: appController.hideXiaozhiOverlay()
                }
            }

            // 对话内容展示区
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: root.dp(12)
                color: Qt.rgba(1.0, 1.0, 1.0, 0.05)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                border.width: 1
                clip: true


                Text {
                    anchors.fill: parent
                    anchors.margins: root.dp(12)
                    text: messageText.length > 0 ? messageText : qsTr("按下唤醒键后开始对话")
                    color: "#e8f2f8"
                    font.pixelSize: root.fs(17)
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // 右下角苹果毛玻璃 AI 唤醒悬浮球
    Rectangle {
        id: fabWake
        width: root.dp(60)
        height: root.dp(60)
        radius: width / 2
        color: fabArea.pressed ? Qt.rgba(0.0, 0.48, 1.0, 0.85) : Qt.rgba(0.08, 0.16, 0.25, 0.85)
        border.color: Qt.rgba(0.40, 0.78, 1.0, 0.60)
        border.width: 1
        anchors.right: parent.right
        anchors.rightMargin: root.dp(18)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.dp(18)
        visible: globalState.xiaozhiAvailable && !overlayVisible
        opacity: visible ? 1.0 : 0.0
        scale: fabArea.pressed ? 0.92 : 1.0
        clip: true

        Behavior on scale { NumberAnimation { duration: 90 } }
        Behavior on opacity { NumberAnimation { duration: 150 } }
        Behavior on color { ColorAnimation { duration: 120 } }


        Image {
            anchors.centerIn: parent
            width: root.dp(24)
            height: root.dp(24)
            sourceSize.width: root.dp(24)
            sourceSize.height: root.dp(24)
            source: "qrc:/icons/robot.svg"
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        MouseArea {
            id: fabArea
            anchors.fill: parent
            onClicked: appController.wakeXiaozhi()
        }
    }
}
