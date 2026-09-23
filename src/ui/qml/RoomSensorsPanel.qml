import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

Rectangle {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)
    property string activeRoom: "客厅"

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    radius: panelRadius
    color: Qt.rgba(0.06, 0.11, 0.16, 0.78)
    border.color: Qt.rgba(1, 1, 1, 0.12)
    border.width: 1
    clip: true

    // 顶部月白玻璃边缘折射微光高光
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.panelRadius
        anchors.rightMargin: root.panelRadius
        height: 1
        color: "#ffffff"
        opacity: 0.18
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.dp(12)
        spacing: root.dp(8)

        // 顶栏：标题与房间切换状态胶囊
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.dp(30)
            spacing: root.dp(8)

            Rectangle {
                width: root.dp(22)
                height: root.dp(22)
                radius: width / 2
                color: Qt.rgba(0.20, 0.52, 0.85, 0.20)
                border.color: Qt.rgba(0.20, 0.52, 0.85, 0.40)
                border.width: 1

                Canvas {
                    anchors.centerIn: parent
                    width: root.dp(12)
                    height: root.dp(12)
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.strokeStyle = "#5dade2"
                        ctx.lineWidth = 1.6
                        ctx.beginPath()
                        // 绘制环境波纹雷达/传感器微图标
                        ctx.arc(width/2, height/2, width*0.4, 0, Math.PI * 2)
                        ctx.stroke()
                        ctx.beginPath()
                        ctx.arc(width/2, height/2, width*0.18, 0, Math.PI * 2)
                        ctx.fillStyle = "#5dade2"
                        ctx.fill()
                    }
                }
            }

            Text {
                text: qsTr("房间环境")
                color: "#e6f1f8"
                font.pixelSize: root.fs(13)
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            // 房间标识胶囊（带在线呼吸指示灯）
            Rectangle {
                Layout.preferredHeight: root.dp(22)
                implicitWidth: roomRow.implicitWidth + root.dp(14)
                radius: height / 2
                color: Qt.rgba(1, 1, 1, 0.06)
                border.color: Qt.rgba(1, 1, 1, 0.10)
                border.width: 1

                Row {
                    id: roomRow
                    anchors.centerIn: parent
                    spacing: root.dp(5)

                    Rectangle {
                        width: root.dp(6)
                        height: root.dp(6)
                        radius: 3
                        color: "#2ecc71"
                        anchors.verticalCenter: parent.verticalCenter
                        
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            PropertyAnimation { to: 0.35; duration: 1100; easing.type: Easing.InOutQuad }
                            PropertyAnimation { to: 1.0; duration: 1100; easing.type: Easing.InOutQuad }
                        }
                    }

                    Text {
                        text: root.activeRoom
                        color: "#9db0c2"
                        font.pixelSize: root.fs(11)
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        // 传感器卡片流动展示区
        Flickable {
            id: flickArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: sensorFlow.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: flickArea.contentHeight > flickArea.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                width: root.dp(4)
                contentItem: Rectangle {
                    implicitWidth: root.dp(4)
                    radius: root.dp(2)
                    color: Qt.rgba(1, 1, 1, 0.22)
                }
            }

            Flow {
                id: sensorFlow
                width: parent.width
                spacing: root.dp(8)

                readonly property bool useTwoColumns: width >= root.dp(250)
                readonly property real itemWidth: useTwoColumns ? Math.floor((width - spacing) / 2) : width

                // 1. 室内温度
                Rectangle {
                    width: sensorFlow.itemWidth
                    height: root.dp(80)
                    radius: root.cardRadius
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: Qt.rgba(1, 1, 1, 0.08)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(9)
                        spacing: root.dp(2)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Image {
                                source: "qrc:/icons/sensor-temp.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: qsTr("室内温度")
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredHeight: root.dp(16)
                                Layout.preferredWidth: root.dp(38)
                                radius: height / 2
                                color: Qt.rgba(0.95, 0.58, 0.16, 0.16)
                                border.color: Qt.rgba(0.95, 0.58, 0.16, 0.35)
                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("舒适")
                                    color: "#f39c12"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "24.5"
                                color: "#f8fafc"
                                font.pixelSize: root.fs(21)
                                font.bold: true
                                font.family: "Menlo, Monaco, Consolas, monospace"
                            }
                            Text {
                                text: "°C"
                                color: "#8da5b8"
                                font.pixelSize: root.fs(12)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(2)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("标称 22~26°C")
                                color: "#627889"
                                font.pixelSize: root.fs(9)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(3)
                            }
                        }
                    }
                }

                // 2. 室内湿度
                Rectangle {
                    width: sensorFlow.itemWidth
                    height: root.dp(80)
                    radius: root.cardRadius
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: Qt.rgba(1, 1, 1, 0.08)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(9)
                        spacing: root.dp(2)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Image {
                                source: "qrc:/icons/sensor-humidity.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: qsTr("室内湿度")
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredHeight: root.dp(16)
                                Layout.preferredWidth: root.dp(38)
                                radius: height / 2
                                color: Qt.rgba(0.20, 0.65, 0.95, 0.16)
                                border.color: Qt.rgba(0.20, 0.65, 0.95, 0.35)
                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("清爽")
                                    color: "#3498db"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "55"
                                color: "#f8fafc"
                                font.pixelSize: root.fs(21)
                                font.bold: true
                                font.family: "Menlo, Monaco, Consolas, monospace"
                            }
                            Text {
                                text: "%"
                                color: "#8da5b8"
                                font.pixelSize: root.fs(12)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(2)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("适宜 40~60%")
                                color: "#627889"
                                font.pixelSize: root.fs(9)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(3)
                            }
                        }
                    }
                }

                // 3. 空气质量 PM2.5
                Rectangle {
                    width: sensorFlow.itemWidth
                    height: root.dp(80)
                    radius: root.cardRadius
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: Qt.rgba(1, 1, 1, 0.08)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(9)
                        spacing: root.dp(2)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Image {
                                source: "qrc:/icons/sensor-air.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: qsTr("空气质量")
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredHeight: root.dp(16)
                                Layout.preferredWidth: root.dp(32)
                                radius: height / 2
                                color: Qt.rgba(0.18, 0.80, 0.44, 0.16)
                                border.color: Qt.rgba(0.18, 0.80, 0.44, 0.35)
                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("优")
                                    color: "#2ecc71"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "15"
                                color: "#f8fafc"
                                font.pixelSize: root.fs(21)
                                font.bold: true
                                font.family: "Menlo, Monaco, Consolas, monospace"
                            }
                            Text {
                                text: "μg/m³"
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(2)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("PM2.5 极佳")
                                color: "#627889"
                                font.pixelSize: root.fs(9)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(3)
                            }
                        }
                    }
                }

                // 4. 人体存在 / 活动联动
                Rectangle {
                    width: sensorFlow.itemWidth
                    height: root.dp(80)
                    radius: root.cardRadius
                    readonly property bool detected: (typeof appController !== "undefined") && appController.humanPresenceDetected
                    color: detected ? Qt.rgba(0.35, 0.20, 0.70, 0.18) : Qt.rgba(1, 1, 1, 0.05)
                    border.color: detected ? Qt.rgba(0.55, 0.35, 0.95, 0.40) : Qt.rgba(1, 1, 1, 0.08)
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 250 } }
                    Behavior on border.color { ColorAnimation { duration: 250 } }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(9)
                        spacing: root.dp(2)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Image {
                                source: "qrc:/icons/sensor-presence.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: qsTr("人体感应")
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredHeight: root.dp(16)
                                implicitWidth: presenceText.implicitWidth + root.dp(10)
                                radius: height / 2
                                color: parent.parent.parent.detected ? Qt.rgba(0.55, 0.35, 0.95, 0.25) : Qt.rgba(1, 1, 1, 0.08)
                                border.color: parent.parent.parent.detected ? Qt.rgba(0.65, 0.45, 1.0, 0.50) : Qt.rgba(1, 1, 1, 0.12)
                                Text {
                                    id: presenceText
                                    anchors.centerIn: parent
                                    text: parent.parent.parent.parent.detected ? qsTr("活跃") : qsTr("静止")
                                    color: parent.parent.parent.parent.detected ? "#b388ff" : "#94a3b8"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: parent.parent.detected ? qsTr("有人在场") : qsTr("无人活动")
                                color: parent.parent.detected ? "#e0d4fc" : "#cbd5e1"
                                font.pixelSize: root.fs(15)
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: parent.parent.detected ? qsTr("实时感知中") : qsTr("保持守候")
                                color: "#627889"
                                font.pixelSize: root.fs(9)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(2)
                            }
                        }
                    }
                }

                // 5. 光照强度
                Rectangle {
                    width: sensorFlow.itemWidth
                    height: root.dp(80)
                    radius: root.cardRadius
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: Qt.rgba(1, 1, 1, 0.08)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(9)
                        spacing: root.dp(2)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Image {
                                source: "qrc:/icons/weather-sunny.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: qsTr("环境光照")
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredHeight: root.dp(16)
                                Layout.preferredWidth: root.dp(38)
                                radius: height / 2
                                color: Qt.rgba(0.95, 0.77, 0.16, 0.16)
                                border.color: Qt.rgba(0.95, 0.77, 0.16, 0.35)
                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("明亮")
                                    color: "#f1c40f"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "320"
                                color: "#f8fafc"
                                font.pixelSize: root.fs(21)
                                font.bold: true
                                font.family: "Menlo, Monaco, Consolas, monospace"
                            }
                            Text {
                                text: "Lux"
                                color: "#8da5b8"
                                font.pixelSize: root.fs(12)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(2)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("室内自然光")
                                color: "#627889"
                                font.pixelSize: root.fs(9)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(3)
                            }
                        }
                    }
                }

                // 6. 门窗/安防微卡片
                Rectangle {
                    width: sensorFlow.itemWidth
                    height: root.dp(80)
                    radius: root.cardRadius
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: Qt.rgba(1, 1, 1, 0.08)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(9)
                        spacing: root.dp(2)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Image {
                                source: "qrc:/icons/sensor-door.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: qsTr("门窗状态")
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredHeight: root.dp(16)
                                Layout.preferredWidth: root.dp(38)
                                radius: height / 2
                                color: Qt.rgba(0.20, 0.80, 0.60, 0.16)
                                border.color: Qt.rgba(0.20, 0.80, 0.60, 0.35)
                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("安全")
                                    color: "#1abc9c"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: qsTr("全部已闭合")
                                color: "#f8fafc"
                                font.pixelSize: root.fs(15)
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("安防设防中")
                                color: "#627889"
                                font.pixelSize: root.fs(9)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(2)
                            }
                        }
                    }
                }

                // 7. 环境噪音 / 分贝
                Rectangle {
                    width: sensorFlow.itemWidth
                    height: root.dp(80)
                    radius: root.cardRadius
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: Qt.rgba(1, 1, 1, 0.08)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(9)
                        spacing: root.dp(2)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Image {
                                source: "qrc:/icons/sensor-noise.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: qsTr("环境噪音")
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredHeight: root.dp(16)
                                Layout.preferredWidth: root.dp(38)
                                radius: height / 2
                                color: Qt.rgba(0.20, 0.80, 0.44, 0.16)
                                border.color: Qt.rgba(0.20, 0.80, 0.44, 0.35)
                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("静谧")
                                    color: "#2ecc71"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "36"
                                color: "#f8fafc"
                                font.pixelSize: root.fs(21)
                                font.bold: true
                                font.family: "Menlo, Monaco, Consolas, monospace"
                            }
                            Text {
                                text: "dB"
                                color: "#8da5b8"
                                font.pixelSize: root.fs(12)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(2)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("适宜入眠 <45dB")
                                color: "#627889"
                                font.pixelSize: root.fs(9)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(3)
                            }
                        }
                    }
                }

                // 8. 二氧化碳 / 新风监测
                Rectangle {
                    width: sensorFlow.itemWidth
                    height: root.dp(80)
                    radius: root.cardRadius
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: Qt.rgba(1, 1, 1, 0.08)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(9)
                        spacing: root.dp(2)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: root.dp(6)

                            Image {
                                source: "qrc:/icons/sensor-co2.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: qsTr("CO2 浓度")
                                color: "#8da5b8"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredHeight: root.dp(16)
                                Layout.preferredWidth: root.dp(38)
                                radius: height / 2
                                color: Qt.rgba(0.20, 0.70, 0.95, 0.16)
                                border.color: Qt.rgba(0.20, 0.70, 0.95, 0.35)
                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("优良")
                                    color: "#3498db"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "420"
                                color: "#f8fafc"
                                font.pixelSize: root.fs(21)
                                font.bold: true
                                font.family: "Menlo, Monaco, Consolas, monospace"
                            }
                            Text {
                                text: "ppm"
                                color: "#8da5b8"
                                font.pixelSize: root.fs(12)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(2)
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("空气清新无异味")
                                color: "#627889"
                                font.pixelSize: root.fs(9)
                                Layout.alignment: Qt.AlignBottom
                                Layout.bottomMargin: root.dp(3)
                            }
                        }
                    }
                }
            }
        }

        // 底部固定：全屋微气候舒适健康综合卡片（将中间底部剩余空间 100% 充分利用）
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.dp(52)
            radius: root.cardRadius
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(0.10, 0.24, 0.38, 0.40)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.rgba(0.05, 0.14, 0.22, 0.40)
                }
            }
            border.color: Qt.rgba(0.30, 0.65, 0.95, 0.30)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: root.dp(8)
                spacing: root.dp(8)

                // 舒适度健康分胶囊
                Rectangle {
                    width: root.dp(36)
                    height: root.dp(36)
                    radius: width / 2
                    color: Qt.rgba(0.18, 0.80, 0.44, 0.18)
                    border.color: Qt.rgba(0.18, 0.80, 0.44, 0.45)
                    border.width: 1

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 0
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "98"
                            color: "#2ecc71"
                            font.pixelSize: root.fs(12)
                            font.bold: true
                            font.family: "Menlo, Monaco, Consolas, monospace"
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("分")
                            color: "#2ecc71"
                            font.pixelSize: root.fs(8)
                        }
                    }
                }

                // 中间描述
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    RowLayout {
                        spacing: root.dp(5)
                        Text {
                            text: qsTr("全屋微气候 · 舒适宜居")
                            color: "#ffffff"
                            font.pixelSize: root.fs(11)
                            font.bold: true
                        }
                        Rectangle {
                            width: root.dp(6)
                            height: root.dp(6)
                            radius: 3
                            color: "#2ecc71"
                        }
                    }

                    Text {
                        text: qsTr("温湿清爽 · 空气极佳 · 静谧守候")
                        color: "#8395a7"
                        font.pixelSize: root.fs(9)
                        elide: Text.ElideRight
                    }
                }

                // 右侧：一键净化/新风按钮
                Rectangle {
                    id: airPurifierBtn
                    property bool active: false
                    width: root.dp(68)
                    height: root.dp(28)
                    radius: root.dp(6)
                    color: active ? Qt.rgba(0.20, 0.85, 0.55, 0.30) : Qt.rgba(1, 1, 1, 0.08)
                    border.color: active ? "#2ecc71" : Qt.rgba(1, 1, 1, 0.15)
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: root.dp(4)
                        Image {
                            source: "qrc:/icons/sensor-air.svg"
                            width: root.dp(12)
                            height: root.dp(12)
                            sourceSize: Qt.size(width, height)
                            smooth: true
                        }
                        Text {
                            text: airPurifierBtn.active ? qsTr("净化中") : qsTr("一键新风")
                            color: airPurifierBtn.active ? "#2ecc71" : "#ffffff"
                            font.pixelSize: root.fs(9)
                            font.bold: true
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            airPurifierBtn.active = !airPurifierBtn.active
                            if (airPurifierBtn.active) {
                                appController.triggerHaAction("switch.zimi_cn_1000000001_zncz01_on_p_2_1")
                            }
                        }
                    }
                }
            }
        }
    }
}
