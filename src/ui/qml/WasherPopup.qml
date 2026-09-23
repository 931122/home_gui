import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import "WasherData.js" as WasherData

Popup {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)
    property int chipRadius: dp(10)

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    // 存储当前选中的洗衣机模型数据
    property var actionModel: null
    property string targetEntityId: actionModel ? (actionModel.entityId || "") : ""
    property string targetActionName: actionModel ? (actionModel.name || "") : "滚筒洗衣机"

    // 实时从 appController.haActionModels 获取最新聚合状态
    readonly property var liveModel: {
        var list = appController.haActionModels
        if (!list || list.length === 0) return actionModel
        for (var i = 0; i < list.length; ++i) {
            var item = list[i]
            if (!item) continue
            if (targetEntityId && item.entityId === targetEntityId) return item
            if (targetActionName && item.name === targetActionName) return item
            if (item.isWasher === true) return item
        }
        return actionModel
    }

    // 常用属性便捷绑定
    readonly property bool isPoweredOn: liveModel ? (liveModel.washerPower === "on") : false
    readonly property bool isControlRunning: liveModel ? (liveModel.washerControlStatus === "on") : false
    readonly property string runningStatus: liveModel ? (liveModel.washerRunningStatus || "idle") : "idle"
    readonly property string currentProgress: liveModel ? (liveModel.washerProgress || "idle") : "idle"
    readonly property string remainTime: liveModel ? (liveModel.washerRemainTime || "") : ""
    readonly property bool hasDoorSensor: liveModel ? Boolean(liveModel.washerHasDoorSensor) : false
    readonly property bool doorOpened: liveModel ? Boolean(liveModel.washerDoorOpened) : false
    readonly property bool childLocked: liveModel ? Boolean(liveModel.washerChildLock) : false
    readonly property bool detergentLack: liveModel ? Boolean(liveModel.washerDetergentLack) : false
    readonly property string currentProgram: liveModel ? (liveModel.washerProgram || "mixed_wash") : "mixed_wash"
    readonly property string currentTemp: liveModel ? (liveModel.washerTemp || "30c") : "30c"
    readonly property string currentSpeed: liveModel ? (liveModel.washerSpeed || "800rpm") : "800rpm"
    readonly property string currentRinse: liveModel ? (liveModel.washerRinseCount || "2_times") : "2_times"
    readonly property string currentWaterLevel: liveModel ? (liveModel.washerWaterLevel || "auto") : "auto"
    readonly property string currentDetergent: liveModel ? (liveModel.washerDetergent || "smart") : "smart"
    readonly property bool isWindDispel: liveModel ? Boolean(liveModel.washerWindDispel) : false
    readonly property bool isNightly: liveModel ? Boolean(liveModel.washerNightly) : false
    readonly property real waterUsage: liveModel ? Number(liveModel.washerWaterUsage || 0.0) : 0.0
    readonly property real powerUsage: liveModel ? Number(liveModel.washerPowerUsage || 0.0) : 0.0
    readonly property string lanIp: liveModel ? (liveModel.washerLanIp || "192.168.1.180") : "192.168.1.180"

    readonly property bool isBusy: WasherData.isRunning(runningStatus, liveModel ? liveModel.washerPower : "off", currentProgress, liveModel ? liveModel.washerControlStatus : "off")
    readonly property bool isPaused: runningStatus === "pause" || (isPoweredOn && runningStatus !== "standby" && runningStatus !== "idle" && runningStatus !== "off" && liveModel && liveModel.washerControlStatus === "off" && remainTime && remainTime !== "0")

    // 当前选中的程序对象
    property var activeProgram: {
        for (var i = 0; i < WasherData.commonPrograms.length; ++i) {
            if (WasherData.commonPrograms[i].key === currentProgram) return WasherData.commonPrograms[i]
        }
        for (var j = 0; j < WasherData.allPrograms.length; ++j) {
            if (WasherData.allPrograms[j].key === currentProgram) return WasherData.allPrograms[j]
        }
        return WasherData.commonPrograms[0]
    }

    // 全部程序弹层开关
    property bool showAllProgramsSheet: false

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
        color: Qt.rgba(0.08, 0.12, 0.18, 0.98)
        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.20)
        border.width: 1
        clip: true

        // 顶部月白玻璃边缘微折射高光
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: root.panelRadius
            anchors.rightMargin: root.panelRadius
            height: 1
            color: "#ffffff"
            opacity: 0.35
        }
    }

    function openWithAction(actionData) {
        root.actionModel = actionData
        root.showAllProgramsSheet = false
        root.open()
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
                onPressed: {
                    startY = mouse.y
                    root.isDraggingDown = true
                }
                onPositionChanged: {
                    var dy = mouse.y - startY
                    if (dy > 0) {
                        root.dragOffsetY = dy
                    } else {
                        root.dragOffsetY = dy * 0.2
                    }
                }
                onReleased: {
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
                color: Qt.rgba(0.20, 0.55, 0.90, 0.25)
                border.color: Qt.rgba(0.40, 0.75, 1.0, 0.45)
                border.width: 1

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/icons/washer.svg"
                    width: root.dp(20)
                    height: root.dp(20)
                    sourceSize: Qt.size(width, height)
                }
            }

            ColumnLayout {
                spacing: 1
                RowLayout {
                    spacing: root.dp(8)
                    Text {
                        text: qsTr("美的智能滚筒洗衣机")
                        color: "#ffffff"
                        font.pixelSize: root.fs(16)
                        font.bold: true
                    }
                    Rectangle {
                        radius: root.dp(4)
                        color: Qt.rgba(1, 1, 1, 0.12)
                        implicitWidth: modelTagTxt.implicitWidth + root.dp(10)
                        implicitHeight: root.dp(18)
                        Text {
                            id: modelTagTxt
                            anchors.centerIn: parent
                            text: qsTr("TG100V86WMDY5 · 10kg")
                            color: "#9dbcd1"
                            font.pixelSize: root.fs(9)
                        }
                    }
                }
                Text {
                    text: {
                        var ipTxt = root.lanIp ? ("IP: " + root.lanIp) : ""
                        var stTxt = WasherData.getRunningStatusText(root.runningStatus, root.isPoweredOn ? "on" : "off", root.currentProgress)
                        return qsTr("当前状态: %1 · %2").arg(stTxt).arg(ipTxt)
                    }
                    color: WasherData.getStatusColor(root.runningStatus, root.isPoweredOn ? "on" : "off", root.currentProgress)
                    font.pixelSize: root.fs(11)
                }
            }

            Item { Layout.fillWidth: true }

            // 门状态与童锁指示徽章
            RowLayout {
                spacing: root.dp(6)
                Rectangle {
                    visible: root.hasDoorSensor
                    radius: root.dp(12)
                    color: root.doorOpened ? Qt.rgba(0.9, 0.4, 0.1, 0.25) : Qt.rgba(0.2, 0.7, 0.4, 0.25)
                    border.color: root.doorOpened ? "#f59e0b" : "#4ade80"
                    border.width: 1
                    implicitWidth: doorTxt.implicitWidth + root.dp(16)
                    implicitHeight: root.dp(24)
                    Text {
                        id: doorTxt
                        anchors.centerIn: parent
                        text: root.doorOpened ? qsTr("门已开启") : qsTr("门已关好")
                        color: root.doorOpened ? "#fcd34d" : "#86efac"
                        font.pixelSize: root.fs(10)
                        font.bold: true
                    }
                }

                Rectangle {
                    radius: root.dp(12)
                    color: root.childLocked ? Qt.rgba(0.6, 0.3, 0.9, 0.25) : Qt.rgba(1, 1, 1, 0.08)
                    border.color: root.childLocked ? "#c084fc" : Qt.rgba(1, 1, 1, 0.15)
                    border.width: 1
                    implicitWidth: lockTxt.implicitWidth + root.dp(16)
                    implicitHeight: root.dp(24)
                    Text {
                        id: lockTxt
                        anchors.centerIn: parent
                        text: root.childLocked ? qsTr("童锁已开启") : qsTr("童锁未开启")
                        color: root.childLocked ? "#e9d5ff" : "#9dbcd1"
                        font.pixelSize: root.fs(10)
                    }
                }

                Rectangle {
                    visible: root.detergentLack
                    radius: root.dp(12)
                    color: Qt.rgba(0.9, 0.2, 0.2, 0.25)
                    border.color: "#ef4444"
                    border.width: 1
                    implicitWidth: lackTxt.implicitWidth + root.dp(16)
                    implicitHeight: root.dp(24)
                    Text {
                        id: lackTxt
                        anchors.centerIn: parent
                        text: qsTr("洗涤剂不足")
                        color: "#fca5a5"
                        font.pixelSize: root.fs(10)
                        font.bold: true
                    }
                }
            }

            // 开机待机状态下的精致关机按键
            GlassButton {
                visible: root.isPoweredOn && !root.isBusy
                scaleUnit: root.scaleUnit
                styleType: "danger"
                iconSource: "qrc:/icons/power.svg"
                text: qsTr("关机")
                textPixelSize: root.fs(10)
                iconPixelSize: root.dp(12)
                implicitWidth: root.dp(64)
                implicitHeight: root.dp(24)
                onClicked: appController.setWasherPower(false)
            }

            CloseButton {
                Layout.preferredWidth: root.dp(36)
                Layout.preferredHeight: root.dp(36)
                iconSize: root.dp(16)
                onClicked: root.close()
            }
        }

        // ==================== 主内容区（双栏布局） ====================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.dp(16)

            // ---------- 左栏：拟物科技滚筒视窗与主控 ----------
            Rectangle {
                Layout.preferredWidth: root.dp(250)
                Layout.fillHeight: true
                radius: root.cardRadius
                color: Qt.rgba(0.12, 0.17, 0.24, 0.85)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                border.width: 1
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.dp(14)
                    spacing: root.dp(10)

                    // 滚筒圆形拟物视窗
                    Item {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: root.dp(154)
                        Layout.preferredHeight: root.dp(154)
                        scale: (!root.isPoweredOn && drumClickArea.pressed) ? 0.96 : 1.0

                        Behavior on scale { NumberAnimation { duration: 90 } }

                        // 外圈金属拉丝渐变圆环
                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "#475569" }
                                GradientStop { position: 0.5; color: "#1e293b" }
                                GradientStop { position: 1.0; color: "#64748b" }
                            }
                            border.color: root.isBusy ? "#38bdf8" : (!root.isPoweredOn ? Qt.rgba(0.22, 0.74, 0.97, 0.45) : Qt.rgba(1, 1, 1, 0.25))
                            border.width: root.dp(3)
                        }

                        // 内圈深邃水波视窗
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width - root.dp(14)
                            height: parent.height - root.dp(14)
                            radius: width / 2
                            color: root.isPoweredOn ? "#0f172a" : "#0a0f18"
                            clip: true

                            // 动态旋转的滚筒叶片/波纹
                            Item {
                                id: drumRotor
                                anchors.fill: parent
                                visible: root.isBusy

                                Repeater {
                                    model: 3
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: root.dp(6)
                                        height: parent.height - root.dp(16)
                                        radius: root.dp(3)
                                        color: Qt.rgba(0.22, 0.74, 0.97, 0.35)
                                        rotation: index * 60
                                    }
                                }

                                RotationAnimation on rotation {
                                    running: root.isBusy
                                    loops: Animation.Infinite
                                    from: 0
                                    to: 360
                                    duration: (root.runningStatus === "spin" || root.runningStatus === "dehydration" || root.currentProgress === "spin" || root.currentProgress === "dehydration") ? 600 : 2800
                                }
                            }

                            // 视窗中心信息展现
                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: {
                                        if (!root.isPoweredOn) return qsTr("已关机")
                                        if (root.remainTime && root.remainTime !== "") return root.remainTime
                                        return WasherData.getProgramEstTime(root.currentProgram)
                                    }
                                    color: root.isPoweredOn ? "#ffffff" : "#94a3b8"
                                    font.pixelSize: root.fs(32)
                                    font.bold: true
                                }

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: {
                                        if (!root.isPoweredOn) return qsTr("轻触开机")
                                        if (root.remainTime && root.remainTime !== "") return qsTr("分钟 · 剩余")
                                        return qsTr("分钟 · 预估")
                                    }
                                    color: !root.isPoweredOn ? "#38bdf8" : "#9dbcd1"
                                    font.pixelSize: root.fs(!root.isPoweredOn ? 12 : 10)
                                    font.bold: !root.isPoweredOn
                                }

                                Rectangle {
                                    Layout.alignment: Qt.AlignHCenter
                                    radius: root.dp(4)
                                    color: Qt.rgba(0.22, 0.74, 0.97, 0.20)
                                    implicitWidth: progTxt.implicitWidth + root.dp(12)
                                    implicitHeight: root.dp(16)
                                    visible: root.isPoweredOn
                                    Text {
                                        id: progTxt
                                        anchors.centerIn: parent
                                        text: WasherData.getProgramName(root.currentProgram) + " · " + WasherData.getProgressText(root.currentProgress)
                                        color: "#38bdf8"
                                        font.pixelSize: root.fs(9)
                                        font.bold: true
                                    }
                                }
                            }
                        }

                        // 支持点击左边大的已关机开机
                        MouseArea {
                            id: drumClickArea
                            anchors.fill: parent
                            cursorShape: !root.isPoweredOn ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: {
                                if (!root.isPoweredOn) {
                                    appController.setWasherPower(true)
                                }
                            }
                        }
                    }

                    // 能耗看板
                    Rectangle {
                        Layout.fillWidth: true
                        radius: root.dp(8)
                        color: Qt.rgba(0.08, 0.12, 0.18, 0.75)
                        border.color: Qt.rgba(1, 1, 1, 0.08)
                        border.width: 1
                        implicitHeight: root.dp(34)

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: root.dp(8)
                            spacing: root.dp(8)

                            Text {
                                text: qsTr("本次用水 %1 L").arg(root.waterUsage.toFixed(1))
                                color: "#9dbcd1"
                                font.pixelSize: root.fs(10)
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: qsTr("耗电 %1 度").arg(root.powerUsage.toFixed(2))
                                color: "#9dbcd1"
                                font.pixelSize: root.fs(10)
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // 主控按钮行：启动/暂停大按键（始终为核心洗涤控制，苹果流光全圆角玻璃胶囊，独占整行）
                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.dp(44)
                        scaleUnit: root.scaleUnit
                        styleType: (root.isBusy && !root.isPaused) ? "danger" : "primary"
                        text: (root.isBusy && !root.isPaused) ? qsTr("暂停运行") : (root.isPaused ? qsTr("继续洗涤") : qsTr("开始洗涤"))
                        textPixelSize: root.fs(14)
                        boldText: true
                        onClicked: {
                            if (root.isBusy && !root.isPaused) {
                                appController.setWasherStartPause(false)
                            } else {
                                if (!root.isPoweredOn) {
                                    appController.setWasherPower(true)
                                }
                                appController.setWasherStartPause(true)
                            }
                        }
                    }
                }
            }

            // ---------- 右栏：洗涤程序网格与参数微调面板 ----------
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: root.cardRadius
                color: Qt.rgba(0.10, 0.15, 0.22, 0.85)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                border.width: 1
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.dp(14)
                    spacing: root.dp(10)

                    // 标题行：常用程序选择 + 全部程序苹果玻璃胶囊按钮
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: qsTr("常用洗涤程序")
                            color: "#ffffff"
                            font.pixelSize: root.fs(13)
                            font.bold: true
                        }
                        Item { Layout.fillWidth: true }
                        GlassButton {
                            scaleUnit: root.scaleUnit
                            styleType: "neutral"
                            implicitHeight: root.dp(24)
                            implicitWidth: root.dp(106)
                            text: qsTr("全部 33 种程序")
                            textPixelSize: root.fs(10)
                            onClicked: root.showAllProgramsSheet = true
                        }
                    }

                    // 常用 8 个程序 4x2 网格
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 4
                        rowSpacing: root.dp(6)
                        columnSpacing: root.dp(6)

                        Repeater {
                            model: WasherData.commonPrograms
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: root.dp(54)
                                radius: root.dp(8)

                                readonly property bool isSelected: root.currentProgram === modelData.key

                                color: isSelected
                                       ? Qt.rgba(0.18, 0.45, 0.30, 0.55)
                                       : (progBtnArea.pressed ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0.14, 0.20, 0.28, 0.70))
                                border.color: isSelected ? "#4ade80" : Qt.rgba(1, 1, 1, 0.10)
                                border.width: isSelected ? 1.5 : 1
                                scale: progBtnArea.pressed ? 0.95 : 1.0

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 1
                                    RowLayout {
                                        Layout.alignment: Qt.AlignHCenter
                                        spacing: root.dp(4)
                                        Rectangle {
                                            width: root.dp(18)
                                            height: root.dp(18)
                                            radius: root.dp(4)
                                            color: isSelected ? Qt.rgba(0.2, 0.8, 0.4, 0.3) : Qt.rgba(0.2, 0.6, 0.9, 0.2)
                                            border.color: isSelected ? "#4ade80" : "#38bdf8"
                                            border.width: 1
                                            Layout.alignment: Qt.AlignVCenter

                                            Image {
                                                anchors.centerIn: parent
                                                width: root.dp(12)
                                                height: root.dp(12)
                                                sourceSize.width: root.dp(12)
                                                sourceSize.height: root.dp(12)
                                                source: WasherData.getProgramIcon(modelData.key, modelData.name)
                                                fillMode: Image.PreserveAspectFit
                                                smooth: true
                                            }
                                        }
                                        Text {
                                            text: modelData.name
                                            color: isSelected ? "#ffffff" : "#c8dceb"
                                            font.pixelSize: root.fs(11)
                                            font.bold: isSelected
                                        }
                                    }
                                    Text {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: qsTr("约 %1 分钟").arg(modelData.estTime)
                                        color: isSelected ? "#86efac" : "#728a9c"
                                        font.pixelSize: root.fs(9)
                                    }
                                }

                                MouseArea {
                                    id: progBtnArea
                                    anchors.fill: parent
                                    onClicked: {
                                        appController.setWasherProgram(modelData.key)
                                    }
                                }
                            }
                        }
                    }

                    // 参数精细微调区域
                    Text {
                        text: qsTr("洗涤参数设定")
                        color: "#ffffff"
                        font.pixelSize: root.fs(12)
                        font.bold: true
                    }

                    // 1. 水温档位选择胶囊
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(6)

                        Text {
                            text: qsTr("水温:")
                            color: "#9dbcd1"
                            font.pixelSize: root.fs(10)
                            Layout.preferredWidth: root.dp(36)
                        }

                        Repeater {
                            model: WasherData.temperatures
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: root.dp(26)
                                radius: height / 2
                                readonly property bool isSelected: root.currentTemp === modelData.key
                                color: isSelected ? Qt.rgba(0.15, 0.55, 0.95, 0.45) : (tempArea.pressed ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.06))
                                border.color: isSelected ? Qt.rgba(0.35, 0.80, 1.0, 0.85) : Qt.rgba(1, 1, 1, 0.12)
                                border.width: 1
                                scale: tempArea.pressed ? 0.94 : 1.0
                                Behavior on scale { NumberAnimation { duration: 80 } }
                                clip: true

                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.topMargin: 1
                                    anchors.leftMargin: Math.round(parent.height * 0.25)
                                    anchors.rightMargin: Math.round(parent.height * 0.25)
                                    height: Math.round(parent.height * 0.45)
                                    radius: height / 2
                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, isSelected ? 0.45 : 0.20) }
                                        GradientStop { position: 1.0; color: "transparent" }
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.name
                                    color: isSelected ? "#ffffff" : "#c8dceb"
                                    font.pixelSize: root.fs(10)
                                    font.bold: isSelected
                                }
                                MouseArea {
                                    id: tempArea
                                    anchors.fill: parent
                                    onClicked: appController.setWasherTemperature(modelData.key)
                                }
                            }
                        }
                    }

                    // 2. 脱水转速档位选择胶囊
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(6)

                        Text {
                            text: qsTr("转速:")
                            color: "#9dbcd1"
                            font.pixelSize: root.fs(10)
                            Layout.preferredWidth: root.dp(36)
                        }

                        Repeater {
                            model: WasherData.spinSpeeds
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: root.dp(26)
                                radius: height / 2
                                readonly property bool isSelected: root.currentSpeed === modelData.key
                                color: isSelected ? Qt.rgba(0.65, 0.35, 0.95, 0.45) : (speedArea.pressed ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.06))
                                border.color: isSelected ? Qt.rgba(0.75, 0.50, 1.0, 0.85) : Qt.rgba(1, 1, 1, 0.12)
                                border.width: 1
                                scale: speedArea.pressed ? 0.94 : 1.0
                                Behavior on scale { NumberAnimation { duration: 80 } }
                                clip: true

                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.topMargin: 1
                                    anchors.leftMargin: Math.round(parent.height * 0.25)
                                    anchors.rightMargin: Math.round(parent.height * 0.25)
                                    height: Math.round(parent.height * 0.45)
                                    radius: height / 2
                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, isSelected ? 0.45 : 0.20) }
                                        GradientStop { position: 1.0; color: "transparent" }
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.name
                                    color: isSelected ? "#ffffff" : "#c8dceb"
                                    font.pixelSize: root.fs(10)
                                    font.bold: isSelected
                                }
                                MouseArea {
                                    id: speedArea
                                    anchors.fill: parent
                                    onClicked: appController.setWasherSpinSpeed(modelData.key)
                                }
                            }
                        }
                    }

                    // 3. 漂洗次数与水位/智投组合行
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(10)

                        // 漂洗次数
                        RowLayout {
                            spacing: root.dp(4)
                            Text {
                                text: qsTr("漂洗:")
                                color: "#9dbcd1"
                                font.pixelSize: root.fs(10)
                            }
                            Repeater {
                                model: WasherData.rinseCounts
                                Rectangle {
                                    implicitWidth: root.dp(34)
                                    implicitHeight: root.dp(26)
                                    radius: height / 2
                                    readonly property bool isSelected: root.currentRinse === modelData.key
                                    color: isSelected ? Qt.rgba(0.20, 0.75, 0.45, 0.45) : (rinseArea.pressed ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.06))
                                    border.color: isSelected ? Qt.rgba(0.40, 0.95, 0.60, 0.85) : Qt.rgba(1, 1, 1, 0.12)
                                    border.width: 1
                                    scale: rinseArea.pressed ? 0.94 : 1.0
                                    Behavior on scale { NumberAnimation { duration: 80 } }
                                    clip: true

                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.topMargin: 1
                                        anchors.leftMargin: Math.round(parent.height * 0.25)
                                        anchors.rightMargin: Math.round(parent.height * 0.25)
                                        height: Math.round(parent.height * 0.45)
                                        radius: height / 2
                                        gradient: Gradient {
                                            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, isSelected ? 0.45 : 0.20) }
                                            GradientStop { position: 1.0; color: "transparent" }
                                        }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.name
                                        color: isSelected ? "#ffffff" : "#c8dceb"
                                        font.pixelSize: root.fs(10)
                                        font.bold: isSelected
                                    }
                                    MouseArea {
                                        id: rinseArea
                                        anchors.fill: parent
                                        onClicked: appController.setWasherRinseCount(modelData.key)
                                    }
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // 洗衣液智投
                        RowLayout {
                            spacing: root.dp(4)
                            Text {
                                text: qsTr("洗涤剂:")
                                color: "#9dbcd1"
                                font.pixelSize: root.fs(10)
                            }
                            Rectangle {
                                implicitWidth: detTxt.implicitWidth + root.dp(16)
                                implicitHeight: root.dp(26)
                                radius: height / 2
                                color: root.currentDetergent === "off" ? (detArea.pressed ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.08)) : Qt.rgba(0.15, 0.55, 0.95, 0.45)
                                border.color: root.currentDetergent === "off" ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0.35, 0.80, 1.0, 0.85)
                                border.width: 1
                                scale: detArea.pressed ? 0.94 : 1.0
                                Behavior on scale { NumberAnimation { duration: 80 } }
                                clip: true

                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.topMargin: 1
                                    anchors.leftMargin: Math.round(parent.height * 0.25)
                                    anchors.rightMargin: Math.round(parent.height * 0.25)
                                    height: Math.round(parent.height * 0.45)
                                    radius: height / 2
                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, root.currentDetergent !== "off" ? 0.45 : 0.20) }
                                        GradientStop { position: 1.0; color: "transparent" }
                                    }
                                }

                                Text {
                                    id: detTxt
                                    anchors.centerIn: parent
                                    text: WasherData.getDetergentName(root.currentDetergent)
                                    color: "#ffffff"
                                    font.pixelSize: root.fs(10)
                                }
                                MouseArea {
                                    id: detArea
                                    anchors.fill: parent
                                    onClicked: {
                                        var nextD = (root.currentDetergent === "smart") ? "off" : "smart"
                                        appController.setWasherDetergent(nextD)
                                    }
                                }
                            }
                        }
                    }

                    // 4. 辅助功能快捷开关（忘取无忧、夜间洗、童锁）
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: root.dp(8)

                        // 忘取无忧（防皱抖散）
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: root.dp(30)
                            radius: height / 2
                            color: root.isWindDispel ? Qt.rgba(0.15, 0.55, 0.95, 0.45) : (windArea.pressed ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.06))
                            border.color: root.isWindDispel ? Qt.rgba(0.35, 0.80, 1.0, 0.85) : Qt.rgba(1, 1, 1, 0.12)
                            border.width: 1
                            scale: windArea.pressed ? 0.95 : 1.0
                            Behavior on scale { NumberAnimation { duration: 80 } }
                            clip: true

                            Rectangle {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.topMargin: 1
                                anchors.leftMargin: Math.round(parent.height * 0.25)
                                anchors.rightMargin: Math.round(parent.height * 0.25)
                                height: Math.round(parent.height * 0.45)
                                radius: height / 2
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, root.isWindDispel ? 0.45 : 0.20) }
                                    GradientStop { position: 1.0; color: "transparent" }
                                }
                            }

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: root.dp(4)
                                Text {
                                    text: "抖"
                                    color: root.isWindDispel ? "#ffffff" : "#38bdf8"
                                    font.pixelSize: root.fs(10)
                                    font.bold: true
                                }
                                Text {
                                    text: root.isWindDispel ? qsTr("忘取抖散: 开") : qsTr("忘取抖散: 关")
                                    color: root.isWindDispel ? "#ffffff" : "#9dbcd1"
                                    font.pixelSize: root.fs(10)
                                    font.bold: root.isWindDispel
                                }
                            }
                            MouseArea {
                                id: windArea
                                anchors.fill: parent
                                onClicked: appController.setWasherWindDispel(!root.isWindDispel)
                            }
                        }

                        // 夜间低噪洗
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: root.dp(30)
                            radius: height / 2
                            color: root.isNightly ? Qt.rgba(0.65, 0.35, 0.95, 0.45) : (nightArea.pressed ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.06))
                            border.color: root.isNightly ? Qt.rgba(0.75, 0.50, 1.0, 0.85) : Qt.rgba(1, 1, 1, 0.12)
                            border.width: 1
                            scale: nightArea.pressed ? 0.95 : 1.0
                            Behavior on scale { NumberAnimation { duration: 80 } }
                            clip: true

                            Rectangle {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.topMargin: 1
                                anchors.leftMargin: Math.round(parent.height * 0.25)
                                anchors.rightMargin: Math.round(parent.height * 0.25)
                                height: Math.round(parent.height * 0.45)
                                radius: height / 2
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, root.isNightly ? 0.45 : 0.20) }
                                    GradientStop { position: 1.0; color: "transparent" }
                                }
                            }

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: root.dp(4)
                                Text {
                                    text: "夜"
                                    color: root.isNightly ? "#ffffff" : "#c084fc"
                                    font.pixelSize: root.fs(10)
                                    font.bold: true
                                }
                                Text {
                                    text: root.isNightly ? qsTr("夜间模式: 开") : qsTr("夜间模式: 关")
                                    color: root.isNightly ? "#ffffff" : "#9dbcd1"
                                    font.pixelSize: root.fs(10)
                                    font.bold: root.isNightly
                                }
                            }
                            MouseArea {
                                id: nightArea
                                anchors.fill: parent
                                onClicked: appController.setWasherNightly(!root.isNightly)
                            }
                        }

                        // 童锁
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: root.dp(30)
                            radius: height / 2
                            color: root.childLocked ? Qt.rgba(0.95, 0.45, 0.15, 0.45) : (lockArea.pressed ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.06))
                            border.color: root.childLocked ? Qt.rgba(1.0, 0.65, 0.25, 0.85) : Qt.rgba(1, 1, 1, 0.12)
                            border.width: 1
                            scale: lockArea.pressed ? 0.95 : 1.0
                            Behavior on scale { NumberAnimation { duration: 80 } }
                            clip: true

                            Rectangle {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.topMargin: 1
                                anchors.leftMargin: Math.round(parent.height * 0.25)
                                anchors.rightMargin: Math.round(parent.height * 0.25)
                                height: Math.round(parent.height * 0.45)
                                radius: height / 2
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, root.childLocked ? 0.45 : 0.20) }
                                    GradientStop { position: 1.0; color: "transparent" }
                                }
                            }

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: root.dp(4)
                                Text {
                                    text: "锁"
                                    color: root.childLocked ? "#ffffff" : "#fbbf24"
                                    font.pixelSize: root.fs(10)
                                    font.bold: true
                                }
                                Text {
                                    text: root.childLocked ? qsTr("童锁保护: 开") : qsTr("童锁保护: 关")
                                    color: root.childLocked ? "#ffffff" : "#9dbcd1"
                                    font.pixelSize: root.fs(10)
                                    font.bold: root.childLocked
                                }
                            }
                            MouseArea {
                                id: lockArea
                                anchors.fill: parent
                                onClicked: appController.setWasherChildLock(!root.childLocked)
                            }
                        }
                    }
                }
            }
        }
    }
    }

    // ==================== 全部 33 种程序底部抽屉 ====================
    Rectangle {
        id: allProgramsSheet
        visible: root.showAllProgramsSheet
        z: 99
        anchors.fill: parent
        color: Qt.rgba(0.08, 0.12, 0.18, 0.96)
        radius: root.panelRadius
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.dp(16)
            spacing: root.dp(12)

            // 抽屉头部
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: qsTr("全部洗涤程序 (共 33 种)")
                    color: "#ffffff"
                    font.pixelSize: root.fs(15)
                    font.bold: true
                }
                Item { Layout.fillWidth: true }

                CloseButton {
                    implicitWidth: root.dp(32)
                    implicitHeight: root.dp(32)
                    iconSize: root.dp(14)
                    onClicked: root.showAllProgramsSheet = false
                }
            }

            GridView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                cellWidth: (width) / 2
                cellHeight: root.dp(56)
                model: WasherData.allPrograms

                delegate: Item {
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight

                    Rectangle {
                        id: progCard
                        anchors.centerIn: parent
                        width: parent.width - root.dp(8)
                        height: root.dp(48)
                        radius: root.dp(8)

                        readonly property bool isSelected: root.currentProgram === modelData.key
                        color: progCard.isSelected ? Qt.rgba(0.18, 0.45, 0.30, 0.65) : Qt.rgba(0.14, 0.20, 0.28, 0.80)
                        border.color: progCard.isSelected ? "#4ade80" : Qt.rgba(1, 1, 1, 0.12)
                        border.width: progCard.isSelected ? 1.5 : 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: root.dp(8)
                            spacing: root.dp(8)

                            Rectangle {
                                width: root.dp(24)
                                height: root.dp(24)
                                radius: root.dp(6)
                                color: progCard.isSelected ? Qt.rgba(0.2, 0.8, 0.4, 0.3) : Qt.rgba(0.2, 0.6, 0.9, 0.2)
                                border.color: progCard.isSelected ? "#4ade80" : "#38bdf8"
                                border.width: 1
                                Layout.alignment: Qt.AlignVCenter

                                Image {
                                    anchors.centerIn: parent
                                    width: root.dp(14)
                                    height: root.dp(14)
                                    sourceSize.width: root.dp(14)
                                    sourceSize.height: root.dp(14)
                                    source: WasherData.getProgramIcon(modelData.key, modelData.name)
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    text: modelData.name
                                    color: progCard.isSelected ? "#ffffff" : "#c8dceb"
                                    font.pixelSize: root.fs(11)
                                    font.bold: progCard.isSelected
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: modelData.desc
                                    color: "#728a9c"
                                    font.pixelSize: root.fs(9)
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Text {
                                text: qsTr("%1分").arg(modelData.estTime)
                                color: progCard.isSelected ? "#86efac" : "#9dbcd1"
                                font.pixelSize: root.fs(10)
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                appController.setWasherProgram(modelData.key)
                                root.showAllProgramsSheet = false
                            }
                        }
                    }
                }
            }
        }
    }
}
