import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

Rectangle {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(16)
    property int cardRadius: dp(12)

    signal fullscreenRequested()

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    // 当前模式：优先联动 appController.videoBottomCardMode
    readonly property string currentMode: {
        var m = appController.videoBottomCardMode
        if (m === "camera" || m === "scene" || m === "security") {
            return m
        }
        return "camera"
    }

    radius: cardRadius
    color: Qt.rgba(0.06, 0.11, 0.16, 0.82)
    border.color: Qt.rgba(1, 1, 1, 0.13)
    border.width: 1
    clip: true

    // 顶部微晶玻璃月白折射高光
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.cardRadius
        anchors.rightMargin: root.cardRadius
        height: 1
        color: "#ffffff"
        opacity: 0.20
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.dp(10)
        spacing: root.dp(6)

        // 顶栏：3 等分分段切换栏（直接使用明确名称，点击即切换）
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.dp(26)
            radius: root.dp(6)
            color: Qt.rgba(0, 0, 0, 0.28)
            border.color: Qt.rgba(1, 1, 1, 0.08)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 2
                spacing: root.dp(2)

                Repeater {
                    model: [
                        { key: "camera", icon: "qrc:/icons/camera.svg", name: qsTr("多路监控") },
                        { key: "scene", icon: "qrc:/icons/scene.svg", name: qsTr("快捷场景") },
                        { key: "security", icon: "qrc:/icons/dock-shield.svg", name: qsTr("安防速览") }
                    ]

                    Rectangle {
                        id: tabItem
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        readonly property bool isSelected: root.currentMode === modelData.key
                        radius: root.dp(4)
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: tabItem.isSelected ? Qt.rgba(0.20, 0.55, 0.95, 0.50) : (tabMouse.pressed ? Qt.rgba(1, 1, 1, 0.12) : (tabMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.06) : "transparent"))
                            }
                            GradientStop {
                                position: 1.0
                                color: tabItem.isSelected ? Qt.rgba(0.08, 0.32, 0.70, 0.50) : (tabMouse.pressed ? Qt.rgba(1, 1, 1, 0.06) : "transparent")
                            }
                        }
                        border.color: tabItem.isSelected ? Qt.rgba(0.45, 0.78, 1.0, 0.80) : "transparent"
                        border.width: 1

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: root.dp(4)

                            Image {
                                source: modelData.icon
                                width: root.dp(12)
                                height: root.dp(12)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                text: modelData.name
                                color: tabItem.isSelected ? "#ffffff" : "#8ea5b8"
                                font.pixelSize: root.fs(10)
                                font.bold: tabItem.isSelected
                            }
                        }

                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: appController.setVideoBottomCardMode(modelData.key)
                        }
                    }
                }
            }
        }

        // 内容区域：根据当前模式动态呈现 A/B/C/D
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ================= 模式 A: 监控通道切换与操作工具栏 =================
            RowLayout {
                id: modeAView
                anchors.fill: parent
                visible: root.currentMode === "camera"
                spacing: root.dp(8)

                // 左侧：多摄像头通道切换胶囊滚动流
                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: cameraRow.implicitWidth
                    contentHeight: height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    RowLayout {
                        id: cameraRow
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: root.dp(6)

                        Repeater {
                            model: appController.cameraPreviewModels.length > 0 ? appController.cameraPreviewModels : [
                                { "cameraName": "主摄像头", "index": 0 }
                            ]

                            Rectangle {
                                id: camBtn
                                readonly property bool isCurrent: appController.currentCameraIndex === (modelData.index !== undefined ? modelData.index : index)
                                implicitWidth: Math.max(root.dp(88), camTitle.implicitWidth + root.dp(24))
                                implicitHeight: parent.height > 0 ? Math.min(parent.height, root.dp(38)) : root.dp(34)
                                radius: root.dp(8)
                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: camBtn.isCurrent ? Qt.rgba(0.18, 0.48, 0.88, 0.45) : (camMouse.pressed ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.06))
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: camBtn.isCurrent ? Qt.rgba(0.08, 0.28, 0.58, 0.45) : (camMouse.pressed ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(1, 1, 1, 0.02))
                                    }
                                }
                                border.color: camBtn.isCurrent ? Qt.rgba(0.40, 0.75, 1.0, 0.80) : Qt.rgba(1, 1, 1, 0.12)
                                border.width: 1

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: root.dp(5)

                                    Rectangle {
                                        width: root.dp(6)
                                        height: root.dp(6)
                                        radius: 3
                                        color: camBtn.isCurrent ? "#2ecc71" : "#526573"
                                    }

                                    Text {
                                        id: camTitle
                                        text: (modelData && modelData.cameraName) ? modelData.cameraName : (qsTr("通道 ") + (index + 1))
                                        color: camBtn.isCurrent ? "#ffffff" : "#9eb3c7"
                                        font.pixelSize: root.fs(11)
                                        font.bold: camBtn.isCurrent
                                    }
                                }

                                MouseArea {
                                    id: camMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        var targetIdx = (modelData.index !== undefined) ? modelData.index : index
                                        appController.selectCamera(targetIdx)
                                    }
                                }
                            }
                        }
                    }
                }

                // 竖向细微分割线
                Rectangle {
                    Layout.fillHeight: true
                    Layout.preferredHeight: root.dp(28)
                    width: 1
                    color: Qt.rgba(1, 1, 1, 0.08)
                }

                // 右侧：快捷监控操作纽扣（全屏、抓拍、画质）
                RowLayout {
                    Layout.preferredHeight: parent.height
                    spacing: root.dp(6)

                    // 1. 全屏视窗
                    Rectangle {
                        id: fullBtn
                        implicitWidth: root.dp(52)
                        implicitHeight: parent.height > 0 ? Math.min(parent.height, root.dp(38)) : root.dp(34)
                        radius: root.dp(8)
                        color: fullMouse.pressed ? Qt.rgba(0.25, 0.60, 1.0, 0.35) : Qt.rgba(1, 1, 1, 0.07)
                        border.color: Qt.rgba(1, 1, 1, 0.16)
                        border.width: 1

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: root.dp(2)
                            Image {
                                Layout.alignment: Qt.AlignHCenter
                                source: "qrc:/icons/dock-fullscreen.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("全屏")
                                color: "#ffffff"
                                font.pixelSize: root.fs(9)
                            }
                        }

                        MouseArea {
                            id: fullMouse
                            anchors.fill: parent
                            onClicked: root.fullscreenRequested()
                        }
                    }

                    // 2. 画面抓拍
                    Rectangle {
                        id: snapBtn
                        implicitWidth: root.dp(52)
                        implicitHeight: parent.height > 0 ? Math.min(parent.height, root.dp(38)) : root.dp(34)
                        radius: root.dp(8)
                        color: snapMouse.pressed ? Qt.rgba(0.20, 0.85, 0.55, 0.35) : Qt.rgba(1, 1, 1, 0.07)
                        border.color: Qt.rgba(1, 1, 1, 0.16)
                        border.width: 1

                        property bool snapped: false
                        Timer {
                            id: snapResetTimer
                            interval: 1200
                            onTriggered: snapBtn.snapped = false
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: root.dp(2)
                            Image {
                                Layout.alignment: Qt.AlignHCenter
                                source: snapBtn.snapped ? "qrc:/icons/check.svg" : "qrc:/icons/dock-snapshot.svg"
                                width: root.dp(14)
                                height: root.dp(14)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: snapBtn.snapped ? qsTr("已存") : qsTr("抓拍")
                                color: "#ffffff"
                                font.pixelSize: root.fs(9)
                            }
                        }

                        MouseArea {
                            id: snapMouse
                            anchors.fill: parent
                            onClicked: {
                                snapBtn.snapped = true
                                snapResetTimer.restart()
                            }
                        }
                    }

                    // 3. 实时高清状态标签
                    Rectangle {
                        implicitWidth: root.dp(48)
                        implicitHeight: parent.height > 0 ? Math.min(parent.height, root.dp(38)) : root.dp(34)
                        radius: root.dp(8)
                        color: Qt.rgba(1, 1, 1, 0.04)
                        border.color: Qt.rgba(1, 1, 1, 0.10)
                        border.width: 1

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 1
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "1080P"
                                color: "#00f2fe"
                                font.pixelSize: root.fs(10)
                                font.bold: true
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: appController.videoDecoder.length > 0 ? appController.videoDecoder : "HD"
                                color: "#7a8f9f"
                                font.pixelSize: root.fs(8)
                            }
                        }
                    }
                }
            }

            // ================= 模式 B: 全屋安防与快捷场景 =================
            RowLayout {
                id: modeBView
                anchors.fill: parent
                visible: root.currentMode === "scene"
                spacing: root.dp(8)

                Repeater {
                    model: [
                        { name: qsTr("回家模式"), icon: "qrc:/icons/dock-home.svg", hint: qsTr("迎宾开灯·撤防"), color: "#00c6ff", action: "switch.zimi_cn_1000000001_zncz01_on_p_2_1" },
                        { name: qsTr("离家布防"), icon: "qrc:/icons/dock-leave.svg", hint: qsTr("全屋关灯·警戒"), color: "#ff7675", action: "switch.zimi_cn_1000000001_zncz01_on_p_2_1" },
                        { name: qsTr("睡眠警戒"), icon: "qrc:/icons/dock-sleep.svg", hint: qsTr("伴睡夜灯·静音"), color: "#a29bfe", action: "switch.lumi_cn_1000000004_b1nc01_on_p_2_1" },
                        { name: qsTr("会客影音"), icon: "qrc:/icons/dock-movie.svg", hint: qsTr("氛围灯光·影音"), color: "#fdcb6e", action: "switch.lumi_cn_1000000003_b2nc01_on_p_2_1" }
                    ]

                    Rectangle {
                        id: sceneCard
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: root.dp(8)
                        color: sceneMouse.pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, 0.05)
                        border.color: sceneMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(1, 1, 1, 0.10)
                        border.width: 1

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: root.dp(2)

                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: root.dp(4)
                                Image {
                                    source: modelData.icon
                                    width: root.dp(14)
                                    height: root.dp(14)
                                    sourceSize: Qt.size(width, height)
                                    smooth: true
                                }
                                Text {
                                    text: modelData.name
                                    color: "#ffffff"
                                    font.pixelSize: root.fs(11)
                                    font.bold: true
                                }
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: modelData.hint
                                color: "#8a9ba8"
                                font.pixelSize: root.fs(9)
                            }
                        }

                        MouseArea {
                            id: sceneMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (modelData.action) {
                                    appController.triggerHaAction(modelData.action)
                                }
                            }
                        }
                    }
                }
            }

            // ================= 模式 C: 全屋安防状态速览 =================
            RowLayout {
                id: modeCView
                anchors.fill: parent
                visible: root.currentMode === "security"
                spacing: root.dp(8)

                Repeater {
                    model: [
                        { title: qsTr("智能门锁"), status: qsTr("已安全反锁"), icon: "qrc:/icons/lock-locked.svg", ok: true, color: "#2ecc71" },
                        { title: qsTr("周界门窗"), status: qsTr("6处均已闭合"), icon: "qrc:/icons/sensor-door.svg", ok: true, color: "#2ecc71" },
                        { title: qsTr("在开照明"), status: qsTr("2 盏灯点亮"), icon: "qrc:/icons/light.svg", ok: true, color: "#ffd200" },
                        { title: qsTr("实时功率"), status: qsTr("320W 正常"), icon: "qrc:/icons/dock-bolt.svg", ok: true, color: "#00f2fe" }
                    ]

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: root.dp(8)
                        color: Qt.rgba(1, 1, 1, 0.05)
                        border.color: Qt.rgba(1, 1, 1, 0.10)
                        border.width: 1

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: root.dp(2)

                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: root.dp(4)
                                Image {
                                    source: modelData.icon
                                    width: root.dp(13)
                                    height: root.dp(13)
                                    sourceSize: Qt.size(width, height)
                                    smooth: true
                                }
                                Text {
                                    text: modelData.title
                                    color: "#a4b7c6"
                                    font.pixelSize: root.fs(10)
                                }
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: modelData.status
                                color: modelData.color
                                font.pixelSize: root.fs(10)
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
    }
}
