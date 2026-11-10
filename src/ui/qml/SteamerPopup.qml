import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0
import "SteamerData.js" as SteamerData

Popup {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)
    property int chipRadius: dp(10)

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    // 当前选定的时长（分钟）与食材名称
    property int selectedMinutes: (typeof appController !== "undefined" && appController && appController.steamerTotalMinutes > 0) ? appController.steamerTotalMinutes : 15
    property string selectedDishName: (typeof appController !== "undefined" && appController && appController.steamerDishName && appController.steamerDishName.length > 0) ? appController.steamerDishName : qsTr("蒸鸡蛋")

    property var actionModel: null

    function openWithAction(actionData) {
        root.actionModel = actionData
        if (typeof appController !== "undefined" && appController) {
            if (appController.steamerRunning) {
                root.selectedMinutes = appController.steamerTotalMinutes
                root.selectedDishName = appController.steamerDishName
            }
        }
        root.open()
    }

    readonly property bool isPortrait: Overlay.overlay ? (Overlay.overlay.width < Overlay.overlay.height) : (parent ? (parent.width < parent.height) : (width < height))

    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: Math.min((parent ? parent.width : 800) * 0.96, root.dp(root.isPortrait ? 470 : 780))
    height: Math.min((parent ? parent.height : 480) * 0.95, root.dp(root.isPortrait ? 760 : 460))
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

    // ==================== 可复用组件 ====================

    // 1. 控制台与计时面板组件
    Component {
        id: dashboardComp
        Rectangle {
            anchors.fill: parent
            radius: root.cardRadius
            color: Qt.rgba(0.10, 0.15, 0.22, 0.85)
            border.color: appController.steamerRunning ? Qt.rgba(0.40, 0.85, 0.55, 0.40) : Qt.rgba(1, 1, 1, 0.12)
            border.width: appController.steamerRunning ? 1.5 : 1
            clip: true
            implicitHeight: root.dp(28) + (appController.steamerRunning ? runningCol.implicitHeight : standbyCol.implicitHeight)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: root.dp(14)
                spacing: root.dp(10)

                // 运行中状态
                ColumnLayout {
                    id: runningCol
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: appController.steamerRunning
                    spacing: root.dp(8)

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

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: SteamerData.formatRemainTime(appController.steamerRemainSeconds)
                        color: "#ffffff"
                        font.pixelSize: root.fs(38)
                        font.bold: true
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("食材: %1 (%2分钟)").arg(appController.steamerDishName).arg(appController.steamerTotalMinutes)
                        color: "#c2d6e8"
                        font.pixelSize: root.fs(12)
                    }

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

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.dp(42)
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
                    id: standbyCol
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: !appController.steamerRunning
                    spacing: root.dp(10)

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: qsTr("设定烹饪时长")
                            color: "#ffffff"
                            font.pixelSize: root.fs(13)
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            implicitWidth: selDishBadgeText.implicitWidth + root.dp(16)
                            implicitHeight: root.dp(22)
                            radius: root.dp(11)
                            color: Qt.rgba(0.22, 0.74, 0.97, 0.15)
                            border.color: Qt.rgba(0.22, 0.74, 0.97, 0.35)
                            border.width: 1

                            Text {
                                id: selDishBadgeText
                                anchors.centerIn: parent
                                text: qsTr("食材: %1").arg(root.selectedDishName)
                                color: "#38bdf8"
                                font.pixelSize: root.fs(11)
                                font.bold: true
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: root.dp(16)

                        Rectangle {
                            Layout.preferredWidth: root.dp(36)
                            Layout.preferredHeight: root.dp(36)
                            radius: root.dp(18)
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: minusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.26) : Qt.rgba(1.0, 1.0, 1.0, 0.12) }
                                GradientStop { position: 1.0; color: minusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : Qt.rgba(1.0, 1.0, 1.0, 0.04) }
                            }
                            border.color: minusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.40) : Qt.rgba(1.0, 1.0, 1.0, 0.18)
                            border.width: 1
                            scale: minusArea.pressed ? 0.90 : 1.0
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
                                text: root.selectedMinutes
                                color: "#ffffff"
                                font.pixelSize: root.fs(36)
                                font.bold: true
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("分钟")
                                color: "#8aa0b4"
                                font.pixelSize: root.fs(11)
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: root.dp(36)
                            Layout.preferredHeight: root.dp(36)
                            radius: root.dp(18)
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: plusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.26) : Qt.rgba(1.0, 1.0, 1.0, 0.12) }
                                GradientStop { position: 1.0; color: plusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : Qt.rgba(1.0, 1.0, 1.0, 0.04) }
                            }
                            border.color: plusArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.40) : Qt.rgba(1.0, 1.0, 1.0, 0.18)
                            border.width: 1
                            scale: plusArea.pressed ? 0.90 : 1.0
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

                    Slider {
                        id: steamerTimeSlider
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.dp(24)
                        from: 1
                        to: 60
                        stepSize: 1
                        value: root.selectedMinutes
                        onMoved: root.selectedMinutes = Math.round(value)

                        background: Rectangle {
                            x: steamerTimeSlider.leftPadding
                            y: steamerTimeSlider.topPadding + steamerTimeSlider.availableHeight / 2 - height / 2
                            implicitWidth: 200
                            implicitHeight: root.dp(4)
                            width: steamerTimeSlider.availableWidth
                            height: implicitHeight
                            radius: root.dp(2)
                            color: Qt.rgba(1, 1, 1, 0.15)

                            Rectangle {
                                width: steamerTimeSlider.visualPosition * parent.width
                                height: parent.height
                                radius: parent.radius
                                color: "#38bdf8"
                            }
                        }

                        handle: Rectangle {
                            x: steamerTimeSlider.leftPadding + steamerTimeSlider.visualPosition * (steamerTimeSlider.availableWidth - width)
                            y: steamerTimeSlider.topPadding + steamerTimeSlider.availableHeight / 2 - height / 2
                            implicitWidth: root.dp(18)
                            implicitHeight: root.dp(18)
                            radius: width / 2
                            color: "#ffffff"
                            border.color: "#38bdf8"
                            border.width: 2
                            scale: steamerTimeSlider.pressed ? 1.2 : 1.0
                            Behavior on scale { NumberAnimation { duration: 80 } }
                        }
                    }

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.dp(44)
                        scaleUnit: root.scaleUnit
                        styleType: "primary"
                        text: qsTr("启动蒸煮 (%1 分钟 · %2)").arg(root.selectedMinutes).arg(root.selectedDishName)
                        textPixelSize: root.fs(13.5)
                        boldText: true
                        onClicked: {
                            appController.startSteamer(root.selectedMinutes, root.selectedDishName)
                        }
                    }
                }
            }
        }
    }

    // 2. 预设食材快捷卡片网格组件
    Component {
        id: presetDishesGridComp
        ColumnLayout {
            id: presetContainer
            property int gridCols: 2
            spacing: root.dp(8)

            Text {
                text: (typeof appController !== "undefined" && appController && appController.steamerPresetModels && appController.steamerPresetModels.length > 0)
                      ? qsTr("HA烹饪模式 (从Home Assistant同步)")
                      : qsTr("推荐食材快捷蒸煮 (一键设定)")
                color: "#ffffff"
                font.pixelSize: root.fs(13)
                font.bold: true
            }

            GridLayout {
                id: dishesGrid
                Layout.fillWidth: true
                columns: presetContainer.gridCols
                rowSpacing: root.dp(6)
                columnSpacing: root.dp(6)

                Repeater {
                    model: (typeof appController !== "undefined" && appController && appController.steamerPresetModels && appController.steamerPresetModels.length > 0)
                           ? appController.steamerPresetModels
                           : SteamerData.presetDishes

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: root.dp(56)
                        radius: root.dp(10)

                        readonly property bool isCurrentSelected: root.selectedDishName === modelData.name
                        readonly property bool isCookingThis: appController.steamerRunning && (appController.steamerMode === modelData.name || appController.steamerDishName === modelData.name)

                        color: isCookingThis ? Qt.rgba(0.18, 0.45, 0.30, 0.65)
                                            : (isCurrentSelected ? Qt.rgba(0.20, 0.45, 0.70, 0.35) : Qt.rgba(0.12, 0.17, 0.24, 0.80))
                        border.color: isCookingThis ? "#4ade80"
                                                   : (isCurrentSelected ? "#38bdf8" : Qt.rgba(1, 1, 1, 0.10))
                        border.width: (isCookingThis || isCurrentSelected) ? 1.5 : 1
                        scale: dishArea.pressed ? 0.95 : 1.0
                        Behavior on scale { NumberAnimation { duration: 80 } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: root.dp(8)
                            spacing: root.dp(8)

                            Rectangle {
                                Layout.preferredWidth: root.dp(32)
                                Layout.preferredHeight: root.dp(32)
                                radius: root.dp(16)
                                color: isCookingThis ? Qt.rgba(0.2, 0.8, 0.4, 0.3) : Qt.rgba(1, 1, 1, 0.10)
                                border.color: isCookingThis ? "#4ade80" : Qt.rgba(1, 1, 1, 0.15)
                                border.width: 1

                                Image {
                                    anchors.centerIn: parent
                                    width: root.dp(18)
                                    height: root.dp(18)
                                    sourceSize.width: root.dp(18)
                                    sourceSize.height: root.dp(18)
                                    source: modelData.icon || "qrc:/icons/egg.svg"
                                    smooth: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                Text {
                                    text: modelData.name
                                    color: isCookingThis ? "#86efac" : (isCurrentSelected ? "#ffffff" : "#dbeafe")
                                    font.pixelSize: root.fs(12)
                                    font.bold: true
                                }

                                Text {
                                    text: qsTr("推荐 %1 分钟").arg((typeof modelData.minutes !== "undefined" && modelData.minutes > 0) ? modelData.minutes : ((typeof modelData.time !== "undefined" && modelData.time > 0) ? modelData.time : 15))
                                    color: isCookingThis ? "#bbf7d0" : "#94a3b8"
                                    font.pixelSize: root.fs(9.5)
                                }
                            }
                        }

                        MouseArea {
                            id: dishArea
                            anchors.fill: parent
                            onClicked: {
                                var dishMins = (typeof modelData.minutes !== "undefined" && modelData.minutes > 0) ? modelData.minutes : ((typeof modelData.time !== "undefined" && modelData.time > 0) ? modelData.time : 15)
                                root.selectedMinutes = dishMins
                                root.selectedDishName = modelData.name
                                if (modelData.mode && typeof appController !== "undefined" && appController) {
                                    appController.setSteamerMode(modelData.mode)
                                }
                            }
                            onDoubleClicked: {
                                var dishMins = (typeof modelData.minutes !== "undefined" && modelData.minutes > 0) ? modelData.minutes : ((typeof modelData.time !== "undefined" && modelData.time > 0) ? modelData.time : 15)
                                root.selectedMinutes = dishMins
                                root.selectedDishName = modelData.name
                                if (modelData.mode && typeof appController !== "undefined" && appController) {
                                    appController.setSteamerMode(modelData.mode)
                                }
                                appController.startSteamer(dishMins, modelData.name)
                            }
                        }
                    }
                }
            }
        }
    }

    contentItem: Item {
        // 下滑把手
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
            anchors.margins: root.dp(14)
            spacing: root.dp(10)

            // ==================== 顶部状态栏 ====================
            RowLayout {
                Layout.fillWidth: true
                spacing: root.dp(8)

                Rectangle {
                    Layout.preferredWidth: root.dp(34)
                    Layout.preferredHeight: root.dp(34)
                    radius: root.dp(17)
                    color: Qt.rgba(0.20, 0.70, 0.40, 0.25)
                    border.color: Qt.rgba(0.40, 0.85, 0.55, 0.45)
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/icons/egg.svg"
                        width: root.dp(18)
                        height: root.dp(18)
                        sourceSize: Qt.size(width, height)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    Text {
                        text: qsTr("智能定时蒸煮控制")
                        color: "#ffffff"
                        font.pixelSize: root.fs(15)
                        font.bold: true
                    }
                    Text {
                        text: {
                            if (typeof appController === "undefined" || !appController) return qsTr("就绪")
                            if (appController.steamerRunning) {
                                return qsTr("状态: 运行中 · %1").arg(appController.steamerDishName)
                            }
                            return appController.steamerSocketState ? qsTr("状态: 待机 · 插座已通电") : qsTr("状态: 待机 · 插座断电")
                        }
                        color: (typeof appController !== "undefined" && appController && appController.steamerRunning) ? "#4ade80" : "#94a3b8"
                        font.pixelSize: root.fs(11)
                    }
                }

                // 插座电源独立胶囊开关
                Rectangle {
                    readonly property bool isOn: typeof appController !== "undefined" && appController && appController.steamerSocketState
                    Layout.preferredHeight: root.dp(28)
                    implicitWidth: socketRow.implicitWidth + root.dp(18)
                    radius: root.dp(14)
                    gradient: Gradient {
                        GradientStop {
                            position: 0.0
                            color: parent.isOn
                                   ? (socketArea.pressed ? Qt.rgba(0.20, 0.70, 0.40, 0.95) : Qt.rgba(0.15, 0.60, 0.32, 0.85))
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
                            Layout.preferredWidth: root.dp(6)
                            Layout.preferredHeight: root.dp(6)
                            radius: root.dp(3)
                            color: appController.steamerSocketState ? "#4ade80" : "#64748b"
                        }

                        Text {
                            text: appController.steamerSocketState ? qsTr("插座通电") : qsTr("插座断电")
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
                    Layout.preferredWidth: root.dp(32)
                    Layout.preferredHeight: root.dp(32)
                    iconSize: root.dp(14)
                    onClicked: root.close()
                }
            }

            // ==================== 主体工作区 ====================

            // 1. 横屏：双栏布局
            RowLayout {
                visible: !root.isPortrait
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.dp(12)

                Loader {
                    Layout.preferredWidth: root.dp(250)
                    Layout.fillHeight: true
                    sourceComponent: dashboardComp
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: root.cardRadius
                    color: Qt.rgba(0.10, 0.15, 0.22, 0.85)
                    border.color: Qt.rgba(1, 1, 1, 0.12)
                    border.width: 1
                    clip: true

                    Flickable {
                        anchors.fill: parent
                        anchors.margins: root.dp(12)
                        contentWidth: width
                        contentHeight: lDishes.implicitHeight
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        Loader {
                            id: lDishes
                            width: parent.width
                            sourceComponent: presetDishesGridComp
                            property int gridCols: 2
                        }
                    }
                }
            }

            // 2. 竖屏：自适应纵向弹性滑动流
            Flickable {
                visible: root.isPortrait
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: width
                contentHeight: portSteamerCol.implicitHeight
                clip: true
                boundsBehavior: Flickable.DragAndOvershootBounds
                flickableDirection: Flickable.VerticalFlick
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: portSteamerCol
                    width: parent.width
                    spacing: root.dp(10)

                    // Card 1: 计时看板与时长控制
                    Loader {
                        id: portDashboardLoader
                        Layout.fillWidth: true
                        Layout.preferredHeight: item ? item.implicitHeight : (appController.steamerRunning ? root.dp(210) : root.dp(270))
                        sourceComponent: dashboardComp
                    }

                    // Card 2: 推荐食材快捷点选 (竖屏2列自适应)
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: portDishesCol.implicitHeight + root.dp(24)
                        radius: root.cardRadius
                        color: Qt.rgba(0.10, 0.15, 0.22, 0.85)
                        border.color: Qt.rgba(1, 1, 1, 0.12)
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            id: portDishesCol
                            anchors.fill: parent
                            anchors.margins: root.dp(12)

                            Loader {
                                Layout.fillWidth: true
                                sourceComponent: presetDishesGridComp
                                property int gridCols: 2
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.dp(20)
                    }
                }
            }
        }
    }
}
