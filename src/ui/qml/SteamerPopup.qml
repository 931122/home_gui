import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import "SteamerData.js" as SteamerData

Popup {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)
    property int chipRadius: dp(10)

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    property var actionModel: null
    property int selectedMinutes: 20
    property string selectedDishName: "自定义蒸煮"

    function openWithAction(actionData) {
        root.actionModel = actionData
        var presets = (typeof appController !== "undefined" && appController && appController.steamerPresetModels && appController.steamerPresetModels.length > 0)
                      ? appController.steamerPresetModels
                      : SteamerData.presetDishes
        if (actionData && actionData.name) {
            for (var i = 0; i < presets.length; ++i) {
                var d = presets[i]
                if (actionData.name.indexOf(d.name) !== -1 || (d.name.indexOf(actionData.name) !== -1)) {
                    root.selectedMinutes = d.time
                    root.selectedDishName = d.name
                    break
                }
            }
        } else if (presets.length > 0 && (!root.selectedDishName || root.selectedDishName === "自定义蒸煮")) {
            root.selectedMinutes = presets[0].time
            root.selectedDishName = presets[0].name
        }
        root.open()
    }

    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: Math.min((parent ? parent.width : 800) * 0.96, root.dp(parent && parent.width < parent.height ? 470 : 780))
    height: Math.min((parent ? parent.height : 480) * 0.95, root.dp(parent && parent.width < parent.height ? 760 : 460))
    modal: true
    focus: true
    clip: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property real dragOffsetY: 0
    property bool isDraggingDown: false

    Behavior on dragOffsetY {
        enabled: !root.isDraggingDown
        NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
    }

    Timer {
        id: autoCloseTimer
        interval: 180
        repeat: false
        onTriggered: {
            root.close()
            root.dragOffsetY = 0
            root.isDraggingDown = false
        }
    }

    onClosed: {
        root.dragOffsetY = 0
        root.isDraggingDown = false
    }

    Overlay.modal: Rectangle {
        color: Theme.colorOverlayModal
    }

    background: Rectangle {
        radius: root.panelRadius
        color: Qt.rgba(0.07, 0.11, 0.16, 0.98)
        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.18)
        border.width: 1
        clip: true
    }

    contentItem: Item {
        // 顶部居中下滑把手指示条 (支持向下滑动关闭)
        Item {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.dp(160)
            height: root.dp(20)
            z: 99

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: root.dp(5)
                width: root.dp(44)
                height: root.dp(4)
                radius: root.dp(2)
                color: "#ffffff"
                opacity: swipeDownArea.containsPress ? 0.70 : 0.28
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }

            MouseArea {
                id: swipeDownArea
                anchors.fill: parent
                anchors.margins: -root.dp(8)
                property real startY: 0
                onPressed: (mouse) => {
                    startY = mouse.y
                    root.isDraggingDown = true
                }
                onPositionChanged: (mouse) => {
                    var dy = mouse.y - startY
                    if (dy > 0) {
                        root.dragOffsetY = dy
                    } else {
                        root.dragOffsetY = dy * 0.2
                    }
                }
                onReleased: (mouse) => {
                    root.isDraggingDown = false
                    if (root.dragOffsetY > root.dp(55)) {
                        root.dragOffsetY = root.height
                        autoCloseTimer.start()
                    } else {
                        root.dragOffsetY = 0
                    }
                }
                onCanceled: {
                    root.isDraggingDown = false
                    root.dragOffsetY = 0
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.dp(16)
            spacing: root.dp(12)

            // ==================== 顶部导航栏 ====================
            RowLayout {
                Layout.fillWidth: true
                spacing: root.dp(10)

                Rectangle {
                    Layout.preferredWidth: root.dp(36)
                    Layout.preferredHeight: root.dp(36)
                    radius: root.dp(18)
                    color: Qt.rgba(0.95, 0.60, 0.15, 0.22)
                    border.color: Qt.rgba(0.95, 0.60, 0.15, 0.45)
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(20)
                        height: root.dp(20)
                        source: "qrc:/icons/cooker.svg"
                        sourceSize: Qt.size(width, height)
                        smooth: true
                    }
                }

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: qsTr("智能定时蒸煮台")
                        color: "#ffffff"
                        font.pixelSize: root.fs(16)
                        font.bold: true
                    }
                    Text {
                        text: qsTr("餐桌插座自动延时断电 · 安全防干烧")
                        color: "#7b91a3"
                        font.pixelSize: root.fs(10)
                    }
                }

                Item { Layout.fillWidth: true }

                // 餐桌插座快速通电胶囊
                // 插座状态药丸（苹果微光玻璃胶囊）
                Rectangle {
                    Layout.preferredHeight: root.dp(32)
                    implicitWidth: socketRow.implicitWidth + root.dp(22)
                    radius: root.dp(16)
                    visible: (typeof appController !== "undefined" && appController && appController.steamerSocketEntityId !== "")
                    readonly property bool isOn: appController.steamerSocketState
                    gradient: Gradient {
                        GradientStop {
                            position: 0.0
                            color: parent.isOn
                                   ? (socketArea.pressed ? Qt.rgba(0.24, 0.76, 0.46, 0.90) : Qt.rgba(0.18, 0.65, 0.38, 0.80))
                                   : (socketArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.08))
                        }
                        GradientStop {
                            position: 1.0
                            color: parent.isOn
                                   ? (socketArea.pressed ? Qt.rgba(0.14, 0.52, 0.30, 0.90) : Qt.rgba(0.10, 0.44, 0.24, 0.80))
                                   : (socketArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                        }
                    }
                    border.color: parent.isOn ? Qt.rgba(0.60, 0.95, 0.70, 0.80) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                    border.width: 1
                    scale: socketArea.pressed ? 0.94 : 1.0
                    clip: true

                    Behavior on scale { NumberAnimation { duration: 90 } }

                    RowLayout {
                        id: socketRow
                        anchors.centerIn: parent
                        spacing: root.dp(6)

                        Rectangle {
                            Layout.preferredWidth: root.dp(7)
                            Layout.preferredHeight: root.dp(7)
                            radius: root.dp(3.5)
                            color: appController.steamerSocketState ? "#4ade80" : "#64748b"
                        }

                        Text {
                            text: appController.steamerSocketState ? qsTr("插座电源: 通电中") : qsTr("插座电源: 已断电")
                            color: appController.steamerSocketState ? "#ffffff" : "#94a3b8"
                            font.pixelSize: root.fs(11)
                            font.bold: true
                        }
                    }

                    MouseArea {
                        id: socketArea
                        anchors.fill: parent
                        onClicked: appController.toggleSteamerSocket()
                    }
                }

                CloseButton {
                    implicitWidth: root.dp(36)
                    implicitHeight: root.dp(36)
                    iconSize: root.dp(16)
                    onClicked: root.close()
                }
            }

            // ==================== 主体工作区 (左侧看板 + 右侧食材) ====================
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.dp(12)

                // -------------------- 左侧：当前看板 & 控制台 --------------------
                Rectangle {
                Layout.preferredWidth: root.dp(260)
                Layout.fillHeight: true
                radius: root.cardRadius
                color: Qt.rgba(0.10, 0.15, 0.22, 0.85)
                border.color: appController.steamerRunning ? Qt.rgba(0.40, 0.85, 0.55, 0.40) : Qt.rgba(1, 1, 1, 0.12)
                border.width: appController.steamerRunning ? 1.5 : 1
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.dp(16)
                    spacing: root.dp(10)

                    // 运行中态
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: appController.steamerRunning
                        spacing: root.dp(10)

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: root.dp(6)
                            Rectangle {
                                Layout.preferredWidth: root.dp(8)
                                Layout.preferredHeight: root.dp(8)
                                radius: 4
                                color: "#4ade80"
                                SequentialAnimation on opacity {
                                    running: root.opened && appController.steamerRunning
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 0.3; duration: 600 }
                                    NumberAnimation { to: 1.0; duration: 600 }
                                }
                            }
                            Text {
                                text: appController.steamerMode && appController.steamerMode !== "待机" ? qsTr("模式: %1 (烹饪中)").arg(appController.steamerMode) : qsTr("正在定时烹饪中")
                                color: "#86efac"
                                font.pixelSize: root.fs(12)
                                font.bold: true
                            }
                        }

                        Item { Layout.fillHeight: true }

                        // 大字倒计时
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: SteamerData.formatRemainTime(appController.steamerRemainSeconds)
                            color: "#ffffff"
                            font.pixelSize: root.fs(44)
                            font.bold: true
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("食材: %1 (%2分钟)").arg(appController.steamerDishName).arg(appController.steamerTotalMinutes)
                            color: "#c2d6e8"
                            font.pixelSize: root.fs(13)
                        }

                        // 进度条
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: root.dp(6)
                            radius: 3
                            color: Qt.rgba(1, 1, 1, 0.10)

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                radius: 3
                                width: {
                                    var totalSec = appController.steamerTotalMinutes * 60
                                    if (totalSec <= 0) return 0
                                    var passSec = totalSec - appController.steamerRemainSeconds
                                    return Math.max(0, Math.min(parent.width, parent.width * (passSec / totalSec)))
                                }
                                color: "#4ade80"
                            }
                        }

                        Item { Layout.fillHeight: true }

                        // 停止按钮（苹果红宝石微光玻璃胶囊）
                        GlassButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: root.dp(44)
                            scaleUnit: root.scaleUnit
                            styleType: "danger"
                            text: qsTr("提前结束 / 立即断电")
                            textPixelSize: root.fs(13)
                            boldText: true
                            onClicked: appController.stopSteamer()
                        }
                    }

                    // 待机设置态
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !appController.steamerRunning
                        spacing: root.dp(10)

                        Text {
                            text: qsTr("设定烹饪时长")
                            color: "#ffffff"
                            font.pixelSize: root.fs(14)
                            font.bold: true
                        }

                        Item { Layout.fillHeight: true }

                        // 时长调节大字与步进按钮
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: root.dp(16)

                            // 减（苹果微光玻璃圆形纽扣）
                            Rectangle {
                                Layout.preferredWidth: root.dp(36)
                                Layout.preferredHeight: root.dp(36)
                                radius: root.dp(18)
                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: minusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.26) : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: minusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                                    }
                                }
                                border.color: minusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.40) : Qt.rgba(1.0, 1.0, 1.0, 0.18)
                                border.width: 1
                                scale: minusArea.pressed ? 0.90 : 1.0
                                clip: true

                                Behavior on scale { NumberAnimation { duration: 80 } }

                                Text {
                                    anchors.centerIn: parent
                                    text: "-"
                                    color: "#ffffff"
                                    font.pixelSize: root.fs(20)
                                    font.bold: true
                                }
                                MouseArea {
                                    id: minusArea
                                    anchors.fill: parent
                                    onClicked: root.selectedMinutes = Math.max(1, root.selectedMinutes - 1)
                                }
                            }

                            ColumnLayout {
                                spacing: 0
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: root.selectedMinutes + ""
                                    color: "#ffffff"
                                    font.pixelSize: root.fs(40)
                                    font.bold: true
                                }
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: qsTr("分钟")
                                    color: "#8aa0b4"
                                    font.pixelSize: root.fs(12)
                                }
                            }

                            // 加（苹果微光玻璃圆形纽扣）
                            Rectangle {
                                Layout.preferredWidth: root.dp(36)
                                Layout.preferredHeight: root.dp(36)
                                radius: root.dp(18)
                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: plusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.26) : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: plusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                                    }
                                }
                                border.color: plusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.40) : Qt.rgba(1.0, 1.0, 1.0, 0.18)
                                border.width: 1
                                scale: plusArea.pressed ? 0.90 : 1.0
                                clip: true

                                Behavior on scale { NumberAnimation { duration: 80 } }

                                Text {
                                    anchors.centerIn: parent
                                    text: "+"
                                    color: "#ffffff"
                                    font.pixelSize: root.fs(20)
                                    font.bold: true
                                }
                                MouseArea {
                                    id: plusArea
                                    anchors.fill: parent
                                    onClicked: root.selectedMinutes = Math.min(90, root.selectedMinutes + 1)
                                }
                            }
                        }

                        // 滑动条微调
                        Slider {
                            Layout.fillWidth: true
                            from: 1
                            to: 60
                            stepSize: 1
                            value: root.selectedMinutes
                            onMoved: root.selectedMinutes = Math.round(value)
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("选定: %1").arg(root.selectedDishName)
                            color: "#38bdf8"
                            font.pixelSize: root.fs(12)
                        }

                        Item { Layout.fillHeight: true }

                        // 开始按钮（苹果翡翠微光玻璃胶囊）
                        GlassButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: root.dp(44)
                            scaleUnit: root.scaleUnit
                            styleType: "primary"
                            text: qsTr("启动蒸煮 (%1 分钟)").arg(root.selectedMinutes)
                            textPixelSize: root.fs(13)
                            boldText: true
                            onClicked: {
                                appController.startSteamer(root.selectedMinutes, root.selectedDishName)
                            }
                        }
                    }
                }
            }

            // -------------------- 右侧：推荐食材快捷卡片网格 --------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.dp(8)

                Text {
                    text: (typeof appController !== "undefined" && appController && appController.steamerPresetModels && appController.steamerPresetModels.length > 0)
                          ? qsTr("HA烹饪模式 (从Home Assistant同步)")
                          : qsTr("推荐食材快捷蒸煮 (一键设定)")
                    color: "#ffffff"
                    font.pixelSize: root.fs(14)
                    font.bold: true
                }

                GridView {
                    id: dishesGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    cellWidth: width / 2
                    cellHeight: root.dp(70)
                    model: (typeof appController !== "undefined" && appController && appController.steamerPresetModels && appController.steamerPresetModels.length > 0)
                           ? appController.steamerPresetModels
                           : SteamerData.presetDishes

                    delegate: Item {
                        width: dishesGrid.cellWidth
                        height: root.dp(70)

                        Rectangle {
                            id: dishCard
                            anchors.centerIn: parent
                            width: parent.width - root.dp(8)
                            height: root.dp(62)
                            radius: root.dp(10)

                            readonly property bool isCurrentSelected: root.selectedDishName === modelData.name
                            readonly property bool isCookingThis: appController.steamerRunning && (appController.steamerMode === modelData.name || appController.steamerDishName === modelData.name)

                            color: isCookingThis ? Qt.rgba(0.18, 0.45, 0.30, 0.65)
                                                : (isCurrentSelected ? Qt.rgba(0.20, 0.45, 0.70, 0.35) : Qt.rgba(0.12, 0.17, 0.24, 0.80))
                            border.color: isCookingThis ? "#4ade80"
                                                       : (isCurrentSelected ? "#38bdf8" : Qt.rgba(1, 1, 1, 0.10))
                            border.width: (isCookingThis || isCurrentSelected) ? 1.5 : 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: root.dp(10)
                                spacing: root.dp(10)

                                Rectangle {
                                    Layout.preferredWidth: root.dp(36)
                                    Layout.preferredHeight: root.dp(36)
                                    radius: root.dp(18)
                                    color: Qt.rgba(1, 1, 1, 0.08)

                                    Image {
                                        anchors.centerIn: parent
                                        width: root.dp(20)
                                        height: root.dp(20)
                                        source: modelData.icon
                                        sourceSize: Qt.size(width, height)
                                        smooth: true
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    RowLayout {
                                        spacing: root.dp(6)
                                        Text {
                                            text: modelData.name
                                            color: "#ffffff"
                                            font.pixelSize: root.fs(13)
                                            font.bold: true
                                        }
                                        Rectangle {
                                            Layout.preferredHeight: root.dp(16)
                                            implicitWidth: badgeText.implicitWidth + root.dp(8)
                                            radius: root.dp(4)
                                            color: Qt.rgba(1, 1, 1, 0.08)
                                            Text {
                                                id: badgeText
                                                anchors.centerIn: parent
                                                text: modelData.badge
                                                color: modelData.color
                                                font.pixelSize: root.fs(9)
                                            }
                                        }
                                    }
                                    Text {
                                        text: modelData.desc
                                        color: "#728a9c"
                                        font.pixelSize: root.fs(10)
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }

                                ColumnLayout {
                                    spacing: 2
                                    Text {
                                        Layout.alignment: Qt.AlignRight
                                        text: qsTr("%1分").arg(modelData.time)
                                        color: "#86efac"
                                        font.pixelSize: root.fs(13)
                                        font.bold: true
                                    }
                                    Text {
                                        Layout.alignment: Qt.AlignRight
                                        text: dishCard.isCookingThis ? qsTr("烹饪中") : qsTr("选择")
                                        color: dishCard.isCookingThis ? "#4ade80" : "#64748b"
                                        font.pixelSize: root.fs(10)
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.selectedMinutes = modelData.time
                                    root.selectedDishName = modelData.name
                                }
                                onDoubleClicked: {
                                    root.selectedMinutes = modelData.time
                                    root.selectedDishName = modelData.name
                                    appController.startSteamer(modelData.time, modelData.name)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
}
