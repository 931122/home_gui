import QtQuick 2.12
import QtQuick.Layouts 1.12
import HomeGui 1.0

Rectangle {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int chipRadius: dp(10)

    signal cameraClicked()
    signal settingsClicked()
    signal weatherClicked()
    signal calendarClicked()

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    readonly property bool isCompact: width < dp(520)

    radius: panelRadius
    color: Qt.rgba(0.12, 0.18, 0.26, 0.52)
    border.color: Qt.rgba(1, 1, 1, 0.15)
    border.width: 1
    clip: true

    // 顶部微白玻璃边缘折射高光
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.panelRadius
        anchors.rightMargin: root.panelRadius
        height: 1
        color: "#ffffff"
        opacity: 0.30
    }

    function getTopBarWeatherIcon() {
        var cur = appController.weatherCurrent || ({})
        if (cur.icon && cur.icon.length > 0) return cur.icon
        return "qrc:/icons/weather-cloudy.svg"
    }

    function getTopBarWeatherText() {
        var cur = appController.weatherCurrent || ({})
        if (cur.weather && cur.temp) {
            var t = String(cur.temp).replace(/[\u2103°C\s]/g, "").trim()
            return cur.weather + " " + t + "°C"
        }
        var s = String(appController.weatherSummary || "").trim()
        s = s.replace(/[\uD800-\uDBFF][\uDC00-\uDFFF]|[\u2600-\u27BF]|[\uFE00-\uFE0F]/g, "").replace(/\u2103/g, "°C").trim()
        return s || "多云 25°C"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.dp(16)
        anchors.rightMargin: root.dp(16)
        spacing: root.isCompact ? root.dp(6) : root.dp(10)

        HeaderActionButton {
            scaleUnit: root.scaleUnit
            cornerRadius: root.chipRadius
            implicitWidth: root.isCompact ? root.dp(76) : root.dp(92)
            iconSource: "qrc:/icons/camera.svg"
            label: qsTr("Camera")
            boldIcon: true
            onClicked: root.cameraClicked()
        }

        HeaderActionButton {
            visible: true
            scaleUnit: root.scaleUnit
            cornerRadius: root.chipRadius
            implicitWidth: root.isCompact ? root.dp(76) : root.dp(92)
            iconSource: "qrc:/icons/settings.svg"
            label: qsTr("Settings")
            boldIcon: true
            onClicked: root.settingsClicked()
        }

        Item { Layout.fillWidth: true }

        MouseArea {
            id: weatherArea
            Layout.preferredWidth: root.isCompact ? root.dp(108) : root.dp(148)
            Layout.preferredHeight: root.dp(36)
            hoverEnabled: true
            onClicked: root.weatherClicked()

            scale: weatherArea.pressed ? 0.952 : 1.0
            Behavior on scale {
                NumberAnimation {
                    duration: weatherArea.pressed ? 75 : 180
                    easing.type: weatherArea.pressed ? Easing.OutQuad : Easing.OutBack
                    easing.overshoot: 1.12
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: root.chipRadius
                clip: true
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: weatherArea.pressed ? Qt.rgba(1, 1, 1, 0.22) : (weatherArea.containsMouse ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, 0.09))
                    }
                    GradientStop {
                        position: 1.0
                        color: weatherArea.pressed ? Qt.rgba(1, 1, 1, 0.11) : (weatherArea.containsMouse ? Qt.rgba(1, 1, 1, 0.07) : Qt.rgba(1, 1, 1, 0.03))
                    }
                }
                border.color: weatherArea.pressed ? Qt.rgba(1, 1, 1, 0.42) : (weatherArea.containsMouse ? Qt.rgba(1, 1, 1, 0.28) : Qt.rgba(1, 1, 1, 0.18))
                border.width: 1

                // 穹顶透镜曲面高光
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: 1
                    anchors.leftMargin: Math.round(root.chipRadius * 0.4)
                    anchors.rightMargin: Math.round(root.chipRadius * 0.4)
                    height: Math.round(parent.height * 0.48)
                    radius: root.chipRadius
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, weatherArea.pressed ? 0.50 : (weatherArea.containsMouse ? 0.38 : 0.24)) }
                        GradientStop { position: 0.85; color: Qt.rgba(1.0, 1.0, 1.0, 0.02) }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                // 触压波纹提亮
                Rectangle {
                    anchors.fill: parent
                    radius: root.chipRadius
                    color: "#ffffff"
                    opacity: weatherArea.pressed ? 0.12 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 80 } }
                }
            }

            Row {
                anchors.centerIn: parent
                spacing: root.dp(6)

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: root.getTopBarWeatherIcon()
                    sourceSize.width: root.dp(20)
                    sourceSize.height: root.dp(20)
                    width: root.dp(20)
                    height: root.dp(20)
                    fillMode: Image.PreserveAspectFit
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 0

                    Text {
                        text: root.getTopBarWeatherText()
                        color: "#98dfff"
                        font.pixelSize: root.isCompact ? root.fs(10) : root.fs(11)
                        font.bold: true
                    }

                    Text {
                        text: appController.weatherLocation.length > 0 ? appController.weatherLocation : qsTr("Tap to view forecast")
                        color: "#7f9aac"
                        font.pixelSize: root.isCompact ? root.fs(8) : root.fs(9)
                    }
                }
            }
        }

        MouseArea {
            id: calendarArea
            Layout.preferredWidth: root.isCompact ? root.dp(110) : root.dp(160)
            Layout.preferredHeight: root.dp(36)
            Layout.alignment: Qt.AlignVCenter
            onClicked: root.calendarClicked()

            Column {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0

                Text {
                    anchors.right: parent.right
                    text: appController.currentDateText
                    color: "#c3d5e2"
                    font.pixelSize: root.isCompact ? root.fs(9) : root.fs(10)
                    font.bold: true
                    horizontalAlignment: Text.AlignRight
                    renderType: Text.NativeRendering
                }

                Text {
                    anchors.right: parent.right
                    text: appController.currentClockText
                    color: "#f0f6fb"
                    font.pixelSize: root.isCompact ? root.fs(16) : root.fs(18)
                    font.bold: true
                    horizontalAlignment: Text.AlignRight
                    renderType: Text.NativeRendering
                }
            }
        }
    }
}
