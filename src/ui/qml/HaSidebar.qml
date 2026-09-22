import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import HomeGui 1.0
import "CookerRecipes.js" as RecipesData
import "SteamerData.js" as SteamerData

Rectangle {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)
    readonly property bool moreDevicesOpened: moreDevicesPopup.opened
    readonly property bool hasActionStateData: appController.haActionModels && appController.haActionModels.length > 0

    // 核心大卡片设备（固定展示配置文件中的前 4 个设备，各独占一行）
    readonly property var coreCardModels: {
        var fullModel = root.hasActionStateData ? appController.haActionModels : appController.haActionNames
        return (fullModel || []).slice(0, 4)
    }

    // 小磁贴数据源（从第 5 个设备开始取，横屏下少一行双列共 2 行：3 个设备 + 1 个“更多设备”）
    readonly property var miniTileModel: {
        var fullModel = root.hasActionStateData ? appController.haActionModels : appController.haActionNames
        if (!fullModel) return []
        var items = fullModel.slice(4)
        var res = []

        var maxTileDevices = root.width > root.dp(320) ? 5 : 3
        for (var i = 0; i < Math.min(items.length, maxTileDevices); ++i) {
            res.push({
                isDevice: true,
                data: items[i]
            })
        }

        var remainingCount = Math.max(0, items.length - res.length)
        res.push({
            isDevice: false,
            isMore: true,
            name: qsTr("更多设备"),
            subText: remainingCount > 0 ? qsTr("还有 %1 个").arg(remainingCount) : qsTr("全部设备")
        })

        return res
    }

    // 更多设备弹窗专用数据源：排除大卡片与小磁贴后的所有剩余设备
    readonly property var unlistedDevices: {
        var fullModel = root.hasActionStateData ? appController.haActionModels : appController.haActionNames
        if (!fullModel) return []
        var maxTileDevices = root.width > root.dp(320) ? 5 : 3
        var displayedCount = 4 + Math.min(Math.max(0, fullModel.length - 4), maxTileDevices)
        return fullModel.slice(displayedCount)
    }

    readonly property bool isAndroidPlatform: (typeof appController !== "undefined" && appController.isAndroid) || Qt.platform.os === "android"

    // 侧边栏布局高度自适应自动计算（按屏幕可用高度动态伸展，彻底消除底部留白）
    readonly property bool isWideLayout: root.width > root.dp(340)
    readonly property int coreCardRows: isWideLayout ? 2 : 4
    readonly property int titleBarHeight: root.dp(20)
    readonly property int baseMainSpacing: root.dp(4)
    readonly property int miniGridRowSpacing: root.dp(5)
    readonly property int miniGridColSpacing: root.dp(6)

    readonly property int dynamicTileRows: {
        var count = miniTileModel ? miniTileModel.length : 0
        var cols = isWideLayout ? 4 : (root.width > root.dp(320) ? 3 : 2)
        var rows = Math.ceil(count / cols)
        return Math.max(1, Math.min(3, rows))
    }

    readonly property var dynamicLayoutMetrics: {
        var availH = sidebarFlickable.height
        var rows = root.dynamicTileRows
        var cRows = root.coreCardRows
        var minCardH = root.dp(46)
        var minTileH = root.dp(36)
        var defMainSpacing = root.baseMainSpacing
        var defRowSpacing = root.miniGridRowSpacing

        if (availH <= 0) {
            return {
                cardHeight: root.dp(56),
                tileHeight: root.dp(40),
                mainSpacing: defMainSpacing,
                rowSpacing: defRowSpacing
            }
        }

        var fixedOverhead = root.titleBarHeight + ((cRows + 1) * defMainSpacing) + (Math.max(0, rows - 1) * defRowSpacing)
        var netH = availH - fixedOverhead

        var weightCard = 1.35
        var weightTile = 1.0
        var totalWeight = (cRows * weightCard) + (rows * weightTile)

        if (netH > 0 && (netH / totalWeight) * weightCard >= minCardH) {
            var unit = netH / totalWeight
            var tileH = Math.floor(unit * weightTile)
            var cardH = Math.floor(unit * weightCard)
            var usedH = (cRows * cardH) + (rows * tileH) + fixedOverhead
            var remainder = availH - usedH

            var cardBonus = Math.floor(remainder / Math.max(1, cRows))
            cardH += cardBonus
            remainder -= (cardBonus * cRows)

            var spacingBonus = Math.floor(remainder / (cRows + 1))
            var finalSpacing = defMainSpacing + spacingBonus

            return {
                cardHeight: cardH,
                tileHeight: tileH,
                mainSpacing: finalSpacing,
                rowSpacing: defRowSpacing
            }
        } else {
            return {
                cardHeight: minCardH,
                tileHeight: minTileH,
                mainSpacing: defMainSpacing,
                rowSpacing: defRowSpacing
            }
        }
    }

    readonly property int calculatedCardHeight: dynamicLayoutMetrics.cardHeight
    readonly property int calculatedTileHeight: dynamicLayoutMetrics.tileHeight
    readonly property int calculatedMainSpacing: dynamicLayoutMetrics.mainSpacing
    readonly property int calculatedRowSpacing: dynamicLayoutMetrics.rowSpacing

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }
    
    function isLightAction(model) { return model && model.domain === "light" }
    function isCoverAction(model) { return model && model.domain === "cover" }
    function isMediaPlayerAction(model) { return model && model.domain === "media_player" }
    function isCookerAction(model, name) {
        if (model) {
            if (model.isCooker === true || model.domain === "cooker" || model.domain === "chunmi_pre_cooker") return true
            if (model.name && (model.name.indexOf("饭煲") !== -1 || model.name.indexOf("压力锅") !== -1)) return true
        }
        if (name && (name.indexOf("饭煲") !== -1 || name.indexOf("压力锅") !== -1)) return true
        return false
    }
    function isWasherAction(model, name) {
        if (model) {
            if (model.isWasher === true || model.domain === "washer" || model.domain === "washing_machine") return true
            if (model.name && model.name.indexOf("洗衣机") !== -1) return true
            if (model.entityId && (model.entityId.indexOf("123456789012345") !== -1 || model.entityId.indexOf("washer") !== -1)) return true
        }
        if (name && name.indexOf("洗衣机") !== -1) return true
        return false
    }
    function isSteamerAction(model, name) {
        if (model) {
            if (model.isSteamer === true || model.domain === "steamer") return true
            if (model.name && (model.name.indexOf("蒸") !== -1 || model.name.indexOf("煮") !== -1 || model.name.indexOf("蛋") !== -1)) return true
            if (model.entityId && (model.entityId.indexOf("timed_cook") !== -1 || model.entityId.indexOf("boil_eggs") !== -1 || model.entityId.indexOf("steam_") !== -1)) return true
        }
        if (name && (name.indexOf("蒸") !== -1 || name.indexOf("煮") !== -1 || name.indexOf("蛋") !== -1)) return true
        return false
    }
    function hasDetailAction(model) { return isLightAction(model) || isCoverAction(model) || isMediaPlayerAction(model) || (model && model.domain === "lock") }

    function actionLevel(model) {
        if (!model || model.available === false) return 0.0
        if (isLightAction(model)) return Number(model.brightness || (model.active ? 1.0 : 0.0))
        if (isCoverAction(model)) return Number(model.position || 0.0)
        if (isMediaPlayerAction(model)) return Number(model.volume || 0.0)
        return model.active ? 1.0 : 0.0
    }

    function getIconPath(model, name) {
        if (isCookerAction(model, name)) return "qrc:/icons/cooker.svg"
        if (isWasherAction(model, name)) return "qrc:/icons/washer.svg"
        if ((model && (model.entityId && model.entityId.indexOf("egg") !== -1)) || (name && (name.indexOf("蛋") !== -1 || name.indexOf("egg") !== -1))) return "qrc:/icons/egg.svg"
        if ((model && (model.entityId && (model.entityId.indexOf("sweet_potato") !== -1 || model.entityId.indexOf("potato") !== -1))) || (name && (name.indexOf("地瓜") !== -1 || name.indexOf("红薯") !== -1))) return "qrc:/icons/sweet_potato.svg"
        if ((model && (model.entityId && model.entityId.indexOf("taro") !== -1)) || (name && (name.indexOf("芋头") !== -1 || name.indexOf("taro") !== -1))) return "qrc:/icons/taro.svg"
        const domain = model ? model.domain : ""
        if (domain === "light" || (name && (name.indexOf("灯") !== -1 || name.indexOf("照明") !== -1))) return "qrc:/icons/light.svg"
        if (domain === "switch" || domain === "input_boolean" || (name && (name.indexOf("开关") !== -1 || name.indexOf("台") !== -1 || name.indexOf("插座") !== -1 || name.indexOf("总控") !== -1))) return "qrc:/icons/switch.svg"
        if (domain === "cover" || (name && name.indexOf("窗帘") !== -1)) return "qrc:/icons/cover.svg"
        if (domain === "lock" || (name && name.indexOf("门锁") !== -1)) return (model && model.active) ? "qrc:/icons/lock-locked.svg" : "qrc:/icons/lock-unlocked.svg"
        if (domain === "media_player") return "qrc:/icons/media.svg"
        if (domain === "scene") return "qrc:/icons/scene.svg"
        if (!model || model.available === false) return "qrc:/icons/unknown.svg"
        return "qrc:/icons/switch.svg"
    }

    radius: root.panelRadius
    color: Qt.rgba(0.06, 0.11, 0.16, 0.80)
    border.color: Qt.rgba(1, 1, 1, 0.12)
    border.width: 1
    clip: true

    // 顶部月白玻璃边缘折射高光
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.panelRadius
        anchors.rightMargin: root.panelRadius
        height: 1
        color: "#ffffff"
        opacity: 0.20
    }

    // 复用按钮组件
    Component {
        id: actionComponent
        Rectangle {
            id: actionDelegate
            Layout.fillWidth: true
            implicitHeight: root.calculatedCardHeight
            Layout.preferredHeight: root.calculatedCardHeight
            anchors.fill: (parent && typeof parent.actionData !== "undefined") ? parent : undefined
            radius: root.dp(14)
            clip: true
            
            readonly property var currentItemData: {
                if (typeof parent !== "undefined" && parent && typeof parent.actionData !== "undefined" && parent.actionData !== null) {
                    return parent.actionData
                }
                if (typeof modelData !== "undefined" && modelData !== null) {
                    return modelData
                }
                return null
            }
            property var actionModel: {
                if (typeof currentItemData === "object" && currentItemData !== null) {
                    return currentItemData
                }
                if (typeof currentItemData === "string" && currentItemData !== "") {
                    var list = appController.haActionModels
                    if (list) {
                        for (var i = 0; i < list.length; ++i) {
                            if (list[i] && (list[i].name === currentItemData || list[i].entityId === currentItemData)) {
                                return list[i]
                            }
                        }
                    }
                }
                return null
            }
            property string actionName: actionModel ? (actionModel.name || actionModel.entityId || "") : (currentItemData ? String(currentItemData) : "")
            property bool localActiveOverride: false
            property bool hasLocalOverride: false
            readonly property bool isActive: {
                if (hasLocalOverride) return localActiveOverride
                if (root.isSteamerAction(actionModel, actionName)) {
                    return appController.steamerRunning
                }
                return actionModel ? Boolean(actionModel.active) : false
            }
            readonly property bool isAvailable: actionModel ? (actionModel.available !== false) : true
            readonly property bool isSlideToTurnOff: actionModel ? Boolean(actionModel.slideToTurnOff || actionModel.slideToClose || actionModel.slide_to_turn_off || actionModel.slide_to_close) : false

            property real slideProgress: 0.0
            property bool isDraggingSlide: false
            property bool showSlideHint: false
            property real shakeX: 0

            transform: Translate {
                x: actionDelegate.shakeX
            }

            SequentialAnimation {
                id: shakeAnim
                NumberAnimation { target: actionDelegate; property: "shakeX"; from: 0; to: -root.dp(6); duration: 45; easing.type: Easing.OutQuad }
                NumberAnimation { target: actionDelegate; property: "shakeX"; from: -root.dp(6); to: root.dp(6); duration: 60; easing.type: Easing.InOutQuad }
                NumberAnimation { target: actionDelegate; property: "shakeX"; from: root.dp(6); to: -root.dp(4); duration: 50; easing.type: Easing.InOutQuad }
                NumberAnimation { target: actionDelegate; property: "shakeX"; from: -root.dp(4); to: root.dp(4); duration: 50; easing.type: Easing.InOutQuad }
                NumberAnimation { target: actionDelegate; property: "shakeX"; from: root.dp(4); to: 0; duration: 45; easing.type: Easing.InQuad }
            }

            Timer {
                id: slideHintTimer
                interval: 2200
                onTriggered: actionDelegate.showSlideHint = false
            }

            NumberAnimation {
                id: resetSlideAnim
                target: actionDelegate
                property: "slideProgress"
                to: 0.0
                duration: 220
                easing.type: Easing.OutQuad
            }

            function turnOffDevice() {
                hasLocalOverride = true
                localActiveOverride = false
                if (actionDelegate.actionModel && (actionDelegate.actionModel.domain === "switch" || actionDelegate.actionModel.domain === "light" || actionDelegate.actionModel.domain === "input_boolean")) {
                    appController.callHaActionService(actionDelegate.actionName, "turn_off")
                } else {
                    appController.triggerHaAction(actionDelegate.actionName)
                }
            }

            function turnOnDevice() {
                hasLocalOverride = true
                localActiveOverride = true
                if (actionDelegate.actionModel && (actionDelegate.actionModel.domain === "switch" || actionDelegate.actionModel.domain === "light" || actionDelegate.actionModel.domain === "input_boolean")) {
                    appController.callHaActionService(actionDelegate.actionName, "turn_on")
                } else {
                    appController.triggerHaAction(actionDelegate.actionName)
                }
            }

            scale: (actionArea.pressed && !actionDelegate.isDraggingSlide) ? 0.96 : 1.0
            Behavior on scale { NumberAnimation { duration: 100 } }

            color: !isAvailable ? Qt.rgba(1, 1, 1, 0.02)
                                : (actionDelegate.isDraggingSlide
                                   ? Qt.rgba(0.18 + 0.60 * actionDelegate.slideProgress, 0.45 - 0.25 * actionDelegate.slideProgress, 0.30 - 0.20 * actionDelegate.slideProgress, 0.40)
                                   : (isActive ? Qt.rgba(0.18, 0.45, 0.30, 0.32) : Qt.rgba(1, 1, 1, 0.055)))
            border.color: (actionDelegate.isSlideToTurnOff && actionDelegate.isActive && actionDelegate.showSlideHint)
                          ? "#ff7875"
                          : (isActive ? (actionDelegate.slideProgress >= 0.65 ? "#ff7875" : Qt.rgba(0.40, 0.85, 0.55, 0.45)) : Qt.rgba(1, 1, 1, 0.10))
            border.width: (actionDelegate.isSlideToTurnOff && actionDelegate.isActive && actionDelegate.showSlideHint) ? 1.5 : 1

            // 卡片顶部月白微高光
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: root.dp(16)
                anchors.rightMargin: root.dp(16)
                height: 1
                color: isActive ? "#a8f0c2" : "#ffffff"
                opacity: isActive ? 0.35 : 0.15
            }

            // 亮度/窗帘微调层
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 1
                radius: parent.radius - 1
                width: (parent.width - 2) * (actionArea.brightnessDrag ? actionArea.previewLevel : root.actionLevel(actionModel))
                color: "#ffffff"
                opacity: 0.10
                visible: root.hasDetailAction(actionModel) && isAvailable
                
                Behavior on width {
                    enabled: !actionArea.brightnessDrag
                    NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                }
            }

            // 滑动关闭的红色/橙色展开填充层（仅在拖动时展现，未拖动时绝无任何深色遮罩阴影）
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 1
                radius: parent.radius - 1
                width: Math.max(0, (parent.width - 2) * actionDelegate.slideProgress)
                color: actionDelegate.slideProgress >= 0.60 ? Qt.rgba(0.92, 0.28, 0.24, 0.75) : Qt.rgba(0.90, 0.45, 0.20, 0.45)
                visible: actionDelegate.isSlideToTurnOff && actionDelegate.isActive && (actionDelegate.slideProgress > 0.005 || actionDelegate.isDraggingSlide)
                clip: true
                z: 15

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: root.dp(3)
                    color: "#ffffff"
                    opacity: 0.8
                }
            }

            // 跟随手指移动的滑动滑钮（Handle，仅在用户拖动时显示，平时隐藏）
            Rectangle {
                id: slideHandle
                x: Math.max(root.dp(4), Math.min(parent.width - width - root.dp(4), (parent.width - width - root.dp(8)) * actionDelegate.slideProgress + root.dp(4)))
                anchors.verticalCenter: parent.verticalCenter
                width: root.dp(38)
                height: root.dp(38)
                radius: root.dp(19)
                color: actionDelegate.slideProgress >= 0.60 ? "#ff4d4f" : "#f59e0b"
                border.color: "#ffffff"
                border.width: 2
                visible: actionDelegate.isSlideToTurnOff && actionDelegate.isActive && (actionDelegate.isDraggingSlide || actionDelegate.slideProgress > 0.01)
                z: 20

                Image {
                    anchors.centerIn: parent
                    width: root.dp(18)
                    height: root.dp(18)
                    source: actionDelegate.slideProgress >= 0.60 ? "qrc:/icons/power.svg" : "qrc:/icons/arrow-right.svg"
                    sourceSize: Qt.size(width, height)
                    smooth: true
                }
            }

            MouseArea {
                id: actionArea
                anchors.fill: parent
                property real previewLevel: 0
                property bool brightnessDrag: false
                property real startX: 0
                property real startY: 0
                property bool dragTriggered: false

                onPressed: {
                    startX = mouse.x
                    startY = mouse.y
                    dragTriggered = false
                    brightnessDrag = false
                    resetSlideAnim.stop()

                    // 如果是滑动关闭卡片，优先锁定触摸事件，绝不让 Flickable 抢夺
                    if (actionDelegate.isSlideToTurnOff && actionDelegate.isActive) {
                        actionArea.preventStealing = true
                    }

                    if (root.hasDetailAction(actionDelegate.actionModel)) {
                        previewLevel = root.actionLevel(actionDelegate.actionModel)
                    }
                }

                onPositionChanged: {
                    var dx = mouse.x - startX
                    var dy = mouse.y - startY

                    // 1. 滑动关闭手势处理 (仅在开启且是 slideToTurnOff 开关时触发)
                    if (actionDelegate.isSlideToTurnOff && actionDelegate.isActive) {
                        // 如果还未开始水平拖动且垂直位移明显占优，放开给外层列表滚动
                        if (!actionDelegate.isDraggingSlide && Math.abs(dy) > root.dp(12) && Math.abs(dy) > Math.abs(dx) * 1.6) {
                            actionArea.preventStealing = false
                            return
                        }
                        if (!dragTriggered && (dx > root.dp(4) || Math.abs(dx) > Math.abs(dy))) {
                            dragTriggered = true
                            actionDelegate.isDraggingSlide = true
                            actionArea.preventStealing = true
                        }
                        if (actionDelegate.isDraggingSlide) {
                            var travelDist = Math.max(1, actionDelegate.width - root.dp(48))
                            var curDist = Math.max(0, Math.min(travelDist, dx))
                            actionDelegate.slideProgress = curDist / travelDist
                        }
                        return
                    }

                    // 2. 亮度 / 窗帘 / 媒体音量滑动处理
                    if (root.hasDetailAction(actionDelegate.actionModel)) {
                        if (!brightnessDrag && Math.abs(dx) > root.dp(10)) {
                            brightnessDrag = true
                        }
                        if (brightnessDrag) {
                            previewLevel = Math.max(0, Math.min(1.0, mouse.x / width))
                        }
                    }
                }

                onReleased: {
                    actionArea.preventStealing = false

                    // 1. 滑动关闭释放处理
                    if (actionDelegate.isSlideToTurnOff && actionDelegate.isActive) {
                        if (actionDelegate.isDraggingSlide) {
                            actionDelegate.isDraggingSlide = false
                            if (actionDelegate.slideProgress >= 0.60) {
                                actionDelegate.slideProgress = 0.0
                                actionDelegate.turnOffDevice()
                            } else {
                                resetSlideAnim.start()
                            }
                        }
                        return
                    }

                    // 2. 亮度 / 窗帘 / 媒体音量释放处理
                    if (brightnessDrag) {
                        const name = actionDelegate.actionName
                        if (root.isLightAction(actionDelegate.actionModel)) appController.setHaLightBrightness(name, Math.max(0.01, previewLevel))
                        else if (root.isCoverAction(actionDelegate.actionModel)) appController.setHaCoverPosition(name, previewLevel)
                        else if (root.isMediaPlayerAction(actionDelegate.actionModel)) appController.setHaMediaVolume(name, previewLevel)
                        brightnessDrag = false
                    }
                }

                onCanceled: {
                    actionArea.preventStealing = false
                    if (actionDelegate.isSlideToTurnOff && actionDelegate.isActive) {
                        actionDelegate.isDraggingSlide = false
                        resetSlideAnim.start()
                    }
                }

                onClicked: {
                    if (brightnessDrag || dragTriggered) return

                    // 关键防误触：开启状态下防误触，提示向右滑动关闭
                    if (actionDelegate.isSlideToTurnOff && actionDelegate.isActive) {
                        actionDelegate.showSlideHint = true
                        slideHintTimer.restart()
                        shakeAnim.restart()
                        return
                    }

                    // 关闭状态下：单击直接开启！
                    if (actionDelegate.isSlideToTurnOff && !actionDelegate.isActive) {
                        actionDelegate.turnOnDevice()
                        return
                    }

                    // 其他设备普通点击
                    if (root.isCookerAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                        root.openCooker(actionDelegate.actionModel, actionDelegate.actionName)
                    } else if (root.isWasherAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                        root.openWasher(actionDelegate.actionModel, actionDelegate.actionName)
                    } else if (root.isSteamerAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                        root.openSteamer(actionDelegate.actionModel, actionDelegate.actionName)
                    } else {
                        appController.triggerHaAction(actionDelegate.actionName)
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: root.dp(8)
                spacing: root.dp(10)

                Rectangle {
                    Layout.preferredWidth: root.dp(38)
                    Layout.preferredHeight: root.dp(38)
                    radius: root.dp(19)
                    color: actionDelegate.isActive
                           ? (root.isWasherAction(actionDelegate.actionModel, actionDelegate.actionName)
                              ? "#38bdf8"
                              : (root.isCookerAction(actionDelegate.actionModel, actionDelegate.actionName) && (actionDelegate.actionModel && (actionDelegate.actionModel.cookerIsKeepWarm || (actionDelegate.actionModel.stateText && actionDelegate.actionModel.stateText.indexOf("保温") !== -1)))
                                 ? "#f3a83c"
                                 : "#9af06d"))
                           : "#263542"
                    opacity: actionDelegate.isAvailable ? 1.0 : 0.4

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(20)
                        height: root.dp(20)
                        source: root.getIconPath(actionDelegate.actionModel, actionDelegate.actionName)
                        sourceSize: Qt.size(width, height)
                        smooth: true
                        visible: status === Image.Ready
                    }
                    
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "white"
                        opacity: 0.15
                        visible: actionDelegate.isActive
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    Text {
                        Layout.fillWidth: true
                        text: actionDelegate.actionName || qsTr("未知设备")
                        color: actionDelegate.isActive ? "#ffffff" : "#c5d1dd"
                        font.pixelSize: root.fs(14)
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: {
                            if (actionDelegate.isSlideToTurnOff) {
                                if (actionDelegate.isActive) {
                                    if (actionDelegate.showSlideHint) {
                                        return qsTr("向右滑动以关闭")
                                    }
                                    if (actionDelegate.isDraggingSlide) {
                                        return actionDelegate.slideProgress >= 0.65 ? qsTr("松手立即关闭") : qsTr("向右滑动关闭...")
                                    }
                                    return qsTr("已开启 · 滑动关闭")
                                }
                                return qsTr("单击开启")
                            }
                            if (root.isCookerAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                                var st = (actionDelegate.actionModel && actionDelegate.actionModel.stateText) ? actionDelegate.actionModel.stateText : qsTr("待机")
                                if (actionDelegate.isActive && actionDelegate.actionModel) {
                                    var lt = RecipesData.formatCookerTime(actionDelegate.actionModel)
                                    if (lt !== "" && lt !== "--") {
                                        var isKw = Boolean(actionDelegate.actionModel.cookerIsKeepWarm || actionDelegate.actionModel.is_keep_warm || st.indexOf("保温") !== -1)
                                        if (isKw) {
                                            st += " · " + lt
                                        } else {
                                            st += " · 剩" + lt
                                        }
                                    }
                                }
                                return st
                            }
                            if (root.isWasherAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                                var m = actionDelegate.actionModel
                                var isPwr = m ? (m.washerPower === "on") : false
                                if (!isPwr) return qsTr("已关机")
                                var rSt = m ? (m.stateText || qsTr("待机中")) : qsTr("待机中")
                                var rt = m ? (m.washerRemainTime || "") : ""
                                if (rt !== "" && rt !== "0" && (actionDelegate.isActive || (rSt !== qsTr("待机中") && rSt !== qsTr("已关机")))) {
                                    return rSt + " · 剩" + rt + "分"
                                }
                                return rSt
                            }
                            if (root.isSteamerAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                                if (appController.steamerRunning) {
                                    return qsTr("%1 · 剩%2").arg(appController.steamerDishName).arg(SteamerData.formatRemainTime(appController.steamerRemainSeconds))
                                }
                                return appController.steamerSocketState ? qsTr("待机 · 插座通电") : qsTr("待机")
                            }
                            return actionDelegate.actionModel ? actionDelegate.actionModel.stateText : ""
                        }
                        color: {
                            if (actionDelegate.isSlideToTurnOff && actionDelegate.isActive) {
                                if (actionDelegate.showSlideHint) return "#ff7875"
                                if (actionDelegate.isDraggingSlide) return actionDelegate.slideProgress >= 0.65 ? "#ff7875" : "#ffc069"
                                return "#85e89d"
                            }
                            if (actionDelegate.isActive) {
                                if (root.isCookerAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                                    var isKw = Boolean(actionDelegate.actionModel && (actionDelegate.actionModel.cookerIsKeepWarm || actionDelegate.actionModel.is_keep_warm || (actionDelegate.actionModel.stateText && actionDelegate.actionModel.stateText.indexOf("保温") !== -1)))
                                    return isKw ? "#f3a83c" : "#9af06d"
                                }
                                if (root.isWasherAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                                    return "#38bdf8"
                                }
                                if (root.isSteamerAction(actionDelegate.actionModel, actionDelegate.actionName)) {
                                    return "#4ade80"
                                }
                                return "#9af06d"
                            }
                            return "#7b8ea0"
                        }
                        font.pixelSize: root.fs(11)
                        visible: text !== ""
                    }
                }

                // 开启状态下且需要滑动关闭时的提示小胶囊
                Rectangle {
                    Layout.preferredWidth: root.dp(24)
                    Layout.preferredHeight: root.dp(24)
                    radius: root.dp(12)
                    color: actionDelegate.showSlideHint ? Qt.rgba(0.92, 0.28, 0.24, 0.35) : Qt.rgba(1, 1, 1, 0.10)
                    border.color: actionDelegate.showSlideHint ? "#ff7875" : Qt.rgba(1, 1, 1, 0.18)
                    border.width: 1
                    visible: actionDelegate.isSlideToTurnOff && actionDelegate.isActive && !actionDelegate.isDraggingSlide && actionDelegate.slideProgress <= 0.01

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(11)
                        height: root.dp(11)
                        source: "qrc:/icons/arrow-right.svg"
                        sourceSize: Qt.size(width, height)
                        smooth: true
                    }

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        running: actionDelegate.isSlideToTurnOff && actionDelegate.isActive && !actionDelegate.isDraggingSlide && !actionDelegate.showSlideHint
                        NumberAnimation { to: 0.35; duration: 800; easing.type: Easing.InOutQuad }
                        NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                    }
                }
            }
        }
    }

    // 六宫格小磁贴组件（78dp x 44dp）
    Component {
        id: miniTileComponent

        Rectangle {
            id: tileDelegate
            Layout.fillWidth: true
            implicitHeight: root.calculatedTileHeight
            Layout.preferredHeight: root.calculatedTileHeight
            radius: root.dp(10)
            clip: true

            readonly property bool isDevice: modelData && modelData.isDevice === true
            readonly property var itemData: isDevice ? modelData.data : null
            readonly property var actionModel: {
                if (!isDevice) return null
                if (typeof itemData === "object" && itemData !== null) return itemData
                if (typeof itemData === "string" && itemData !== "") {
                    var list = appController.haActionModels
                    if (list) {
                        for (var i = 0; i < list.length; ++i) {
                            if (list[i] && (list[i].name === itemData || list[i].entityId === itemData)) {
                                return list[i]
                            }
                        }
                    }
                }
                return null
            }
            readonly property string actionName: isDevice ? (actionModel ? (actionModel.name || actionModel.entityId || "") : (itemData ? String(itemData) : "")) : (modelData ? modelData.name : "")
            readonly property bool isActive: {
                if (root.isSteamerAction(actionModel, actionName)) {
                    return appController.steamerRunning
                }
                return isDevice ? (actionModel ? Boolean(actionModel.active) : false) : false
            }
            readonly property bool isAvailable: isDevice ? (actionModel ? (actionModel.available !== false) : true) : true

            scale: tileArea.pressed ? 0.94 : 1.0
            Behavior on scale { NumberAnimation { duration: 80 } }

            color: !isDevice
                   ? (tileArea.pressed ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.055))
                   : (!isAvailable
                      ? Qt.rgba(1, 1, 1, 0.02)
                      : (isActive ? Qt.rgba(0.18, 0.45, 0.30, 0.35) : Qt.rgba(1, 1, 1, 0.055)))

            border.color: !isDevice
                          ? Qt.rgba(1, 1, 1, 0.12)
                          : (isActive ? Qt.rgba(0.40, 0.85, 0.55, 0.50) : Qt.rgba(1, 1, 1, 0.10))
            border.width: 1

            // 顶部月白微高光
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: root.dp(8)
                anchors.rightMargin: root.dp(8)
                height: 1
                color: tileDelegate.isActive ? "#a8f0c2" : "#ffffff"
                opacity: tileDelegate.isActive ? 0.35 : 0.15
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: root.dp(5)
                anchors.bottomMargin: root.dp(5)
                anchors.leftMargin: root.dp(7)
                anchors.rightMargin: root.dp(7)
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    // 图标容器 (20 x 20)
                    Rectangle {
                        Layout.preferredWidth: root.dp(20)
                        Layout.preferredHeight: root.dp(20)
                        radius: root.dp(10)
                        color: !tileDelegate.isDevice
                               ? Qt.rgba(1, 1, 1, 0.08)
                               : (tileDelegate.isActive
                                  ? (root.isWasherAction(tileDelegate.actionModel, tileDelegate.actionName) ? "#38bdf8" : "#9af06d")
                                  : "#263542")

                        Image {
                            anchors.centerIn: parent
                            width: root.dp(12)
                            height: root.dp(12)
                            sourceSize: Qt.size(width, height)
                            source: !tileDelegate.isDevice ? "qrc:/icons/more.svg" : root.getIconPath(tileDelegate.actionModel, tileDelegate.actionName)
                            smooth: true
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // 激活状态发光小圆点（开启时亮起，未开启时不显示）
                    Rectangle {
                        visible: tileDelegate.isDevice && tileDelegate.isActive
                        Layout.preferredWidth: root.dp(6)
                        Layout.preferredHeight: root.dp(6)
                        radius: root.dp(3)
                        color: root.isWasherAction(tileDelegate.actionModel, tileDelegate.actionName) ? "#38bdf8" : "#4ade80"
                    }

                    // 更多设备小尖头
                    Text {
                        visible: !tileDelegate.isDevice
                        text: "›"
                        color: "#7b8ea0"
                        font.pixelSize: root.fs(13)
                        font.bold: true
                    }
                }

                Item { Layout.fillHeight: true }

                Text {
                    Layout.fillWidth: true
                    text: tileDelegate.actionName || qsTr("未知")
                    color: tileDelegate.isActive ? "#ffffff" : "#c5d1dd"
                    font.pixelSize: root.fs(12)
                    font.bold: true
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: tileArea
                anchors.fill: parent
                onClicked: {
                    if (!tileDelegate.isDevice) {
                        moreDevicesPopup.open()
                        return
                    }

                    if (root.isCookerAction(tileDelegate.actionModel, tileDelegate.actionName)) {
                        root.openCooker(tileDelegate.actionModel, tileDelegate.actionName)
                    } else if (root.isWasherAction(tileDelegate.actionModel, tileDelegate.actionName)) {
                        root.openWasher(tileDelegate.actionModel, tileDelegate.actionName)
                    } else if (root.isSteamerAction(tileDelegate.actionModel, tileDelegate.actionName)) {
                        root.openSteamer(tileDelegate.actionModel, tileDelegate.actionName)
                    } else {
                        appController.triggerHaAction(tileDelegate.actionName)
                    }
                }
            }
        }
    }

    Flickable {
        id: sidebarFlickable
        anchors.fill: parent
        anchors.margins: root.dp(7)
        contentWidth: width
        contentHeight: sidebarCol.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick
        interactive: sidebarCol.implicitHeight > sidebarFlickable.height + 2

        ColumnLayout {
            id: sidebarCol
            width: sidebarFlickable.width
            spacing: root.calculatedMainSpacing

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: root.titleBarHeight

                Text {
                    text: qsTr("Smart Home")
                    color: "#ffffff"
                    font.pixelSize: root.fs(15)
                    font.bold: true
                    opacity: 0.95
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            // 侧边栏前 4 个核心设备（宽屏下自动启用 2 列精致卡片；窄屏保持单列）
            GridLayout {
                id: coreCardGrid
                Layout.fillWidth: true
                columns: root.isWideLayout ? 2 : 1
                rowSpacing: root.calculatedMainSpacing
                columnSpacing: root.miniGridColSpacing

                Repeater {
                    model: root.coreCardModels
                    delegate: actionComponent
                }
            }

            // 次级小磁贴矩阵（宽屏下 4 列排布；窄屏 2~3 列）
            GridLayout {
                id: miniTileGrid
                Layout.fillWidth: true
                columns: root.isWideLayout ? 4 : (root.width > root.dp(320) ? 3 : 2)
                rowSpacing: root.calculatedRowSpacing
                columnSpacing: root.miniGridColSpacing

                Repeater {
                    model: root.miniTileModel
                    delegate: miniTileComponent
                }
            }
        }
    }

    // 全屏展示弹窗
    Popup {
        id: moreDevicesPopup
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        width: Math.min((parent ? parent.width : 800) * 0.94, root.dp(720))
        height: Math.min((parent ? parent.height : 480) * 0.90, root.dp(parent && parent.width < parent.height ? 600 : 420))
        modal: true
        focus: true
        clip: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Overlay.modal: Rectangle {
            color: isAndroidPlatform ? Qt.rgba(0.02, 0.04, 0.07, 0.20) : Qt.rgba(0.02, 0.04, 0.07, 0.65)
        }

        background: Rectangle {
            radius: root.panelRadius
            color: Qt.rgba(0.07, 0.12, 0.18, 0.85)
            border.color: Qt.rgba(1, 1, 1, 0.16)
            border.width: 1
            clip: true

            // 顶部月白玻璃微折射高光线
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: root.panelRadius
                anchors.rightMargin: root.panelRadius
                height: 1
                color: "#ffffff"
                opacity: 0.28
            }
            
            // 顶部小抓手饰条
            Rectangle {
                width: root.dp(40)
                height: root.dp(4)
                radius: 2
                color: Qt.rgba(1, 1, 1, 0.22)
                anchors.top: parent.top
                anchors.topMargin: root.dp(8)
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.dp(20)
            spacing: root.dp(16)

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: qsTr("更多智能设备")
                    color: "#ffffff"
                    font.pixelSize: root.fs(20)
                    font.bold: true
                    Layout.fillWidth: true
                }
                CloseButton {
                    implicitWidth: root.dp(36)
                    implicitHeight: root.dp(36)
                    iconSize: root.dp(16)
                    onClicked: moreDevicesPopup.close()
                }
            }

            GridView {
                id: moreDevicesGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                cellWidth: width / 2
                cellHeight: root.dp(84)
                model: root.unlistedDevices
                visible: count > 0

                delegate: Item {
                    width: moreDevicesGrid.cellWidth
                    height: root.dp(84)

                    Loader {
                        anchors.centerIn: parent
                        width: parent.width - root.dp(12)
                        height: root.dp(72)
                        sourceComponent: actionComponent

                        property var actionData: modelData
                    }
                }
            }

            // 当所有设备均已在侧边栏展示时的空状态提示
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                visible: moreDevicesGrid.count === 0
                spacing: root.dp(12)

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: root.dp(60)
                    Layout.preferredHeight: root.dp(60)
                    radius: root.dp(30)
                    color: Qt.rgba(1, 1, 1, 0.05)
                    border.color: Qt.rgba(1, 1, 1, 0.10)
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(28)
                        height: root.dp(28)
                        source: "qrc:/icons/check.svg"
                        sourceSize: Qt.size(width, height)
                        smooth: true
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("常用设备已全部在侧边栏展示")
                    color: "#ffffff"
                    font.pixelSize: root.fs(16)
                    font.bold: true
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("如需控制更多设备，可在 config.yaml 中配置追加实体")
                    color: "#7b8ea0"
                    font.pixelSize: root.fs(12)
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    signal cookerRequested(var cookerData)
    function openCooker(model, name) {
        if (moreDevicesPopup.visible) {
            moreDevicesPopup.close()
        }
        var m = model
        if (!m && name) {
            var list = appController.haActionModels
            if (list) {
                for (var i = 0; i < list.length; ++i) {
                    if (list[i] && list[i].name === name) {
                        m = list[i]
                        break
                    }
                }
            }
        }
        if (!m) {
            m = { name: (name || "电饭煲"), isCooker: true, available: true }
        }
        root.cookerRequested(m)
    }

    signal washerRequested(var washerData)
    function openWasher(model, name) {
        if (moreDevicesPopup.visible) {
            moreDevicesPopup.close()
        }
        var m = model
        if (!m && name) {
            var list = appController.haActionModels
            if (list) {
                for (var i = 0; i < list.length; ++i) {
                    if (list[i] && list[i].name === name) {
                        m = list[i]
                        break
                    }
                }
            }
        }
        if (!m) {
            m = { name: (name || "滚筒洗衣机"), isWasher: true, available: true }
        }
        root.washerRequested(m)
    }

    signal steamerRequested(var steamerData)
    function openSteamer(model, name) {
        if (moreDevicesPopup.visible) {
            moreDevicesPopup.close()
        }
        var m = model
        if (!m && name) {
            var list = appController.haActionModels
            if (list) {
                for (var i = 0; i < list.length; ++i) {
                    if (list[i] && list[i].name === name) {
                        m = list[i]
                        break
                    }
                }
            }
        }
        if (!m) {
            m = { name: (name || "智能蒸煮"), isSteamer: true, available: true }
        }
        root.steamerRequested(m)
    }

    function closeMoreDevices() {
        if (moreDevicesPopup.opened) {
            moreDevicesPopup.close()
            return true
        }
        return false
    }
}
