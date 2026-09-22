import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import HomeGui 1.0
import "CookerRecipes.js" as RecipesData

Popup {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)
    property int chipRadius: dp(10)
    property var actionModel: null

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    // 视图状态："main"（主控页，显示8常用+更多入口） | "more"（108道分类大厅）
    property string currentView: "main"
    property string selectedCategory: "all"

    // 菜谱做法详情浮层控制
    property bool showRecipeDetail: false
    property var activeRecipe: null // 当前查看做法的菜谱元数据对象

    // 用户是否在弹窗中主动点击选择了菜谱（防止后台旧状态覆盖）
    property bool hasUserSelection: false
    // 用户自定义或模式预设保压时间（>0 时优先于硬件上报保压时间）
    property double customPressureTime: -1

    // HA 官方食谱动态缓存 (以烹饪模式为 key 缓存 HA 实体上报的权威官方做法、食材与步骤)
    property var haRecipeCache: ({})

    function updateHaRecipeCache() {
        if (!liveModel) return
        var m = liveModel.cookerSelectedMode || liveModel.cookerCurrentMode || liveModel.cookerRunningMode
        if (!m || String(m).trim() === "") return
        var modeKey = String(m).trim()

        var hasIng = liveModel.cookerRecipeIngredients && liveModel.cookerRecipeIngredients.length > 0
        var hasSteps = (liveModel.cookerRecipeSteps && liveModel.cookerRecipeSteps.length > 0) ||
                       (liveModel.cookerRecipePracticeText && String(liveModel.cookerRecipePracticeText).trim() !== "")
        var hasDesc = liveModel.cookerRecipeDescription && String(liveModel.cookerRecipeDescription).trim() !== ""

        if (hasIng || hasSteps || hasDesc) {
            var stepsArr = []
            if (liveModel.cookerRecipeSteps && liveModel.cookerRecipeSteps.length > 0) {
                stepsArr = liveModel.cookerRecipeSteps
            } else if (liveModel.cookerRecipePracticeText) {
                var lines = String(liveModel.cookerRecipePracticeText).split("\n")
                for (var i = 0; i < lines.length; ++i) {
                    var l = lines[i].trim()
                    if (l.length > 0) stepsArr.push(l)
                }
            }

            var cacheObj = root.haRecipeCache
            cacheObj[modeKey] = {
                fromHa: true,
                desc: liveModel.cookerRecipeDescription || "",
                ingredients: liveModel.cookerRecipeIngredients || [],
                steps: stepsArr,
                tips: liveModel.cookerRecipeTips || "",
                practice: liveModel.cookerRecipePractice || "",
                holdingTime: liveModel.cookerHoldingDurationText || "",
                estimatedTime: liveModel.cookerEstimatedCookingTime || (root.allModesTime ? root.allModesTime[modeKey] : "")
            }
            root.haRecipeCache = cacheObj
        }
    }

    function openWithAction(model) {
        root.actionModel = model
        root.currentView = "main"
        root.selectedCategory = "all"
        root.showRecipeDetail = false
        root.hasUserSelection = false
        root.customPressureTime = -1
        root.updateHaRecipeCache()
        if (liveModel && liveModel.cookerCurrentMode && String(liveModel.cookerCurrentMode).trim() !== "") {
            root.selectedMode = String(liveModel.cookerCurrentMode).trim()
        } else {
            root.selectedMode = "大米饭"
        }
        var initRec = RecipesData.getRecipe(root.selectedMode)
        if (initRec) {
            root.activeRecipe = initRec
        }
        root.open()
    }

    // 用户在菜谱卡片或大厅列表中选中某道菜谱，主动同步至 HA 并获取官方云端食谱
    function selectRecipeAndSync(recipeObj) {
        if (!recipeObj) return
        root.activeRecipe = recipeObj
        if (!root.isStrictCooking) {
            root.hasUserSelection = true
            root.selectedMode = recipeObj.mode
            // 联动关键：下发切换模式指令给 Home Assistant，触发 HA 立即把该菜谱的做法、食材推送过来
            appController.setCookerMode(recipeObj.mode)
            if (recipeObj.pressureTime) {
                var matchDigits = String(recipeObj.pressureTime).match(/\d+/)
                if (matchDigits && matchDigits[0]) {
                    var pt = parseFloat(matchDigits[0])
                    if (!isNaN(pt) && pt >= root.pressureMin && pt <= root.pressureMax) {
                        root.customPressureTime = pt
                        appController.setCookerPressureTime(pt)
                    }
                }
            }
        }
    }

    // 仅查看菜谱做法详情
    function openRecipeDetail(recipeObj) {
        if (!recipeObj) return
        root.selectRecipeAndSync(recipeObj)
        root.showRecipeDetail = true
    }

    // 用户在详情浮层明确点击“选定并开始烹饪”时触发
    function applyAndCookRecipe(recipeObj) {
        if (!recipeObj) return
        root.selectedMode = recipeObj.mode
        appController.setCookerMode(recipeObj.mode)
        if (recipeObj.pressureTime) {
            var matchDigits = String(recipeObj.pressureTime).match(/\d+/)
            if (matchDigits && matchDigits[0]) {
                var pt = parseFloat(matchDigits[0])
                if (!isNaN(pt) && pt >= root.pressureMin && pt <= root.pressureMax) {
                    appController.setCookerPressureTime(pt)
                }
            }
        }
        appController.startCooker(recipeObj.mode, "")
        root.showRecipeDetail = false
    }

    readonly property var liveModel: {
        var list = appController.haActionModels
        if (!list || list.length === 0 || !root.actionModel) {
            return root.actionModel
        }
        for (var i = 0; i < list.length; ++i) {
            var item = list[i]
            if (item && (item.name === root.actionModel.name || item.entityId === root.actionModel.entityId)) {
                return item
            }
        }
        return root.actionModel
    }

    // 正在运行的烹饪模式（只读硬件上报）
    readonly property string runningModeText: {
        if (liveModel && liveModel.cookerRunningMode && String(liveModel.cookerRunningMode).trim() !== "") {
            return String(liveModel.cookerRunningMode).trim()
        }
        if (liveModel && liveModel.cookerCurrentMode && String(liveModel.cookerCurrentMode).trim() !== "") {
            return String(liveModel.cookerCurrentMode).trim()
        }
        return ""
    }

    // 状态判定与实时参数
    readonly property string statusText: {
        if (!liveModel) return qsTr("待机")
        if (liveModel.stateText && String(liveModel.stateText).trim() !== "") {
            return String(liveModel.stateText).trim()
        }
        if (liveModel.state && String(liveModel.state).trim() !== "") {
            return String(liveModel.state).trim()
        }
        return qsTr("待机")
    }

    // 是否处于保温状态
    readonly property bool isKeepWarm: {
        if (!liveModel) return false
        if (liveModel.cookerIsKeepWarm === true || liveModel.is_keep_warm === true) return true
        var st = statusText.toLowerCase()
        return st.indexOf("保温") !== -1 || st.indexOf("warm") !== -1
    }

    // 真正处于高压/加热烹饪锁定状态（排除了待机、空闲、保温）
    readonly property bool isStrictCooking: {
        if (!liveModel) return false
        if (root.isKeepWarm) return false // 保温状态下设备未上锁，支持选菜开启新烹饪
        if (typeof liveModel.cookerIsCooking === "boolean") {
            return liveModel.cookerIsCooking
        }
        var st = String(liveModel.stateText || liveModel.state || "").trim().toLowerCase()
        if (st === "" || st === "待机" || st === "空闲" || st === "关机" || st === "离线" ||
            st === "idle" || st === "standby" || st === "off" || st === "0" || st === "5" ||
            st === "unavailable" || st === "unknown" || st === "已完成" || st === "完成") {
            return false
        }
        return st.indexOf("烹饪") !== -1 || st.indexOf("煮") !== -1 || st.indexOf("加热") !== -1 ||
               st.indexOf("保压") !== -1 || st.indexOf("收汁") !== -1 || st.indexOf("cooking") !== -1 ||
               st.indexOf("pressure") !== -1
    }

    // 综合烹饪中状态（包括高压烹饪与保温，用于顶部剩余时间展示等）
    readonly property bool isCooking: root.isStrictCooking || root.isKeepWarm

    readonly property string leftTimeFormatted: {
        if (!liveModel || !root.isCooking) return "--"
        return RecipesData.formatCookerTime(liveModel)
    }
    readonly property string leftMinutes: leftTimeFormatted
    readonly property string currentTemp: (liveModel && liveModel.cookerTemperature) ? String(liveModel.cookerTemperature) : "--"
    readonly property string currentPressure: (liveModel && liveModel.cookerPressure) ? String(liveModel.cookerPressure) : "0"

    // 口感偏好
    readonly property var tasteOptions: (liveModel && liveModel.cookerTasteOptions && liveModel.cookerTasteOptions.length > 0)
                                        ? liveModel.cookerTasteOptions
                                        : ["软糯", "适中", "弹润"]
    readonly property string currentTaste: (liveModel && liveModel.cookerTaste) ? liveModel.cookerTaste : "适中"

    // 保压时间
    readonly property double pressureMin: (liveModel && typeof liveModel.cookerPressureMin === "number") ? liveModel.cookerPressureMin : 6.0
    readonly property double pressureMax: (liveModel && typeof liveModel.cookerPressureMax === "number") ? liveModel.cookerPressureMax : 25.0
    readonly property double currentPressureTime: (liveModel && typeof liveModel.cookerPressureTime === "number") ? liveModel.cookerPressureTime : 6.0

    // 生效保压时间（用户自定义/模式推荐 > 硬件上报）
    readonly property double effectivePressureTime: (root.customPressureTime > 0)
                                                    ? root.customPressureTime
                                                    : root.currentPressureTime

    // 当前保压时间可调范围（如 6~25 分钟）
    readonly property string holdingDurationRange: (liveModel && liveModel.cookerHoldingDurationRange) ? liveModel.cookerHoldingDurationRange : "6~25分钟"

    // 全部 108 种模式的预估时间字典（来自 select 实体属性 all_modes_estimated_time）
    readonly property var allModesTime: (liveModel && liveModel.cookerAllModesEstimatedTime) ? liveModel.cookerAllModesEstimatedTime : ({})

    // 当前选中的烹饪模式（若正在烹饪则优先显示正在运行的模式；待机时优先用户本地选定模式）
    property string selectedMode: "大米饭"

    onLiveModelChanged: {
        root.updateHaRecipeCache()
        // 关键防护：只要用户在当前弹窗中点击过菜谱，绝不允许后台轮询冲刷覆盖用户的选择！
        if (!root.hasUserSelection && !root.isStrictCooking) {
            if (liveModel && liveModel.cookerCurrentMode && String(liveModel.cookerCurrentMode).trim() !== "") {
                root.selectedMode = String(liveModel.cookerCurrentMode).trim()
            }
        }
    }

    // 预估总耗时（待机时展示：优先保压联动总时长，其次模式字典或预设总时长）
    readonly property string estimatedTimeText: {
        if (liveModel && liveModel.cookerEstimatedTotalTime) {
            return liveModel.cookerEstimatedTotalTime
        }
        if (root.selectedMode && root.allModesTime && root.allModesTime[root.selectedMode]) {
            return root.allModesTime[root.selectedMode]
        }
        if (liveModel && liveModel.cookerPresetEstimatedTime) {
            return liveModel.cookerPresetEstimatedTime
        }
        if (liveModel && liveModel.cookerEstimatedCookingTime) {
            return liveModel.cookerEstimatedCookingTime
        }
        var recipe = RecipesData.getRecipe(root.selectedMode)
        return recipe ? recipe.estimatedTime : "约 45 分钟"
    }

    // 锅盖与手柄安全状态
    readonly property string lidStatus: (liveModel && liveModel.cookerLidStatus) ? liveModel.cookerLidStatus : "已合盖到位"
    readonly property string lockStatus: (liveModel && liveModel.cookerLockStatus) ? liveModel.cookerLockStatus : "已旋转锁死"

    // 获取特定模式卡片的耗时与特色说明（100% 优先 HA 官方全量预估时间字典）
    function getRecipeEstimatedTime(modeName) {
        if (root.allModesTime && root.allModesTime[modeName]) {
            return root.allModesTime[modeName]
        }
        var recipe = RecipesData.getRecipe(modeName)
        return recipe ? recipe.estimatedTime : ""
    }

    // 8道常用菜谱列表
    readonly property var commonRecipes: RecipesData.getCommonRecipes()

    // 当前查看的做法元数据对象（来自本地精选大厨数据库）
    readonly property var currentRecipeData: RecipesData.getRecipeDetail(root.activeRecipe)

    // 判断当前查看的菜是否为电饭锅正在运行或已选定的模式
    readonly property bool isActiveRecipeCurrentDeviceMode: {
        if (!root.activeRecipe) return false
        var m = root.activeRecipe.mode || root.activeRecipe.name
        return m === root.selectedMode || m === root.runningModeText
    }

    // 判断当前查看的菜品做法是否直接来源于 Home Assistant 官方同步（实时上报或已缓存）
    readonly property bool isCurrentRecipeFromHa: {
        if (!root.activeRecipe) return false
        var m = root.activeRecipe.mode || root.activeRecipe.name
        if (root.isActiveRecipeCurrentDeviceMode && liveModel &&
            ((liveModel.cookerRecipeIngredients && liveModel.cookerRecipeIngredients.length > 0) ||
             (liveModel.cookerRecipeSteps && liveModel.cookerRecipeSteps.length > 0) ||
             (liveModel.cookerRecipeDescription && String(liveModel.cookerRecipeDescription).trim() !== ""))) {
            return true
        }
        return Boolean(root.haRecipeCache && root.haRecipeCache[m] && root.haRecipeCache[m].fromHa)
    }

    // 做法详情动态获取字段（优先 HA 实时推送与缓存，离线/未同步时使用大厨数据库兜底）
    readonly property string detailDescription: {
        if (!root.activeRecipe) return ""
        var m = root.activeRecipe.mode || root.activeRecipe.name
        if (root.isActiveRecipeCurrentDeviceMode && liveModel && liveModel.cookerRecipeDescription && String(liveModel.cookerRecipeDescription).trim() !== "") {
            return liveModel.cookerRecipeDescription
        }
        if (root.haRecipeCache && root.haRecipeCache[m] && root.haRecipeCache[m].desc) {
            return root.haRecipeCache[m].desc
        }
        if (root.currentRecipeData && root.currentRecipeData.desc) {
            return root.currentRecipeData.desc
        }
        return root.activeRecipe.desc || ""
    }

    readonly property var detailIngredients: {
        if (!root.activeRecipe) return []
        var m = root.activeRecipe.mode || root.activeRecipe.name
        if (root.isActiveRecipeCurrentDeviceMode && liveModel && liveModel.cookerRecipeIngredients && liveModel.cookerRecipeIngredients.length > 0) {
            return liveModel.cookerRecipeIngredients
        }
        if (root.haRecipeCache && root.haRecipeCache[m] && root.haRecipeCache[m].ingredients && root.haRecipeCache[m].ingredients.length > 0) {
            return root.haRecipeCache[m].ingredients
        }
        if (root.currentRecipeData && root.currentRecipeData.ingredients && root.currentRecipeData.ingredients.length > 0) {
            return root.currentRecipeData.ingredients
        }
        return []
    }

    readonly property var detailSteps: {
        if (!root.activeRecipe) return []
        var m = root.activeRecipe.mode || root.activeRecipe.name
        if (root.isActiveRecipeCurrentDeviceMode && liveModel) {
            if (liveModel.cookerRecipeSteps && liveModel.cookerRecipeSteps.length > 0) {
                return liveModel.cookerRecipeSteps
            }
            if (liveModel.cookerRecipePracticeText && String(liveModel.cookerRecipePracticeText).trim() !== "") {
                var lines = String(liveModel.cookerRecipePracticeText).split("\n")
                var res = []
                for (var i = 0; i < lines.length; ++i) {
                    var l = lines[i].trim()
                    if (l.length > 0) res.push(l)
                }
                if (res.length > 0) return res
            }
        }
        if (root.haRecipeCache && root.haRecipeCache[m] && root.haRecipeCache[m].steps && root.haRecipeCache[m].steps.length > 0) {
            return root.haRecipeCache[m].steps
        }
        if (root.currentRecipeData && root.currentRecipeData.steps && root.currentRecipeData.steps.length > 0) {
            return root.currentRecipeData.steps
        }
        return []
    }

    readonly property string detailTips: {
        if (!root.activeRecipe) return ""
        var m = root.activeRecipe.mode || root.activeRecipe.name
        if (root.isActiveRecipeCurrentDeviceMode && liveModel && liveModel.cookerRecipeTips && String(liveModel.cookerRecipeTips).trim() !== "") {
            return liveModel.cookerRecipeTips
        }
        if (root.haRecipeCache && root.haRecipeCache[m] && root.haRecipeCache[m].tips) {
            return root.haRecipeCache[m].tips
        }
        if (root.currentRecipeData && root.currentRecipeData.tips) {
            return root.currentRecipeData.tips
        }
        return ""
    }

    readonly property string detailPractice: {
        if (!root.activeRecipe) return ""
        var m = root.activeRecipe.mode || root.activeRecipe.name
        if (root.isActiveRecipeCurrentDeviceMode && liveModel && liveModel.cookerRecipePractice && String(liveModel.cookerRecipePractice).trim() !== "") {
            return liveModel.cookerRecipePractice
        }
        if (root.haRecipeCache && root.haRecipeCache[m] && root.haRecipeCache[m].practice) {
            return root.haRecipeCache[m].practice
        }
        if (root.currentRecipeData && root.currentRecipeData.practice) {
            return root.currentRecipeData.practice
        }
        return root.activeRecipe ? (root.activeRecipe.categoryName || "大厨专享") : "精选工艺"
    }

    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: Math.min((parent ? parent.width : 800) * 0.96, root.dp(parent && parent.width < parent.height ? 470 : 780))
    height: Math.min((parent ? parent.height : 480) * 0.95, root.dp(parent && parent.width < parent.height ? 760 : 460))
    modal: true
    focus: true
    clip: true
    padding: 0
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
        color: Theme.isAndroidPlatform ? Qt.rgba(0.01, 0.02, 0.04, 0.20) : Qt.rgba(0.01, 0.02, 0.04, 0.88)
    }

    background: Rectangle {
        radius: root.panelRadius
        color: Qt.rgba(0.08, 0.12, 0.18, 0.98)
        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.22)
        border.width: 1
        clip: true

        // 左上浅冰蓝漫反射微光
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: -root.dp(60)
            width: root.dp(240)
            height: root.dp(240)
            radius: width / 2
            color: Qt.rgba(0.20, 0.58, 0.95, 0.10)
        }

        // 右下紫晶微暖光晕
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: -root.dp(60)
            width: root.dp(220)
            height: root.dp(220)
            radius: width / 2
            color: Qt.rgba(0.42, 0.22, 0.68, 0.06)
        }

        // 顶部 1px 月白光折射线（避开两端大圆角）
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: root.panelRadius
            anchors.rightMargin: root.panelRadius
            height: 1
            color: "#ffffff"
            opacity: 0.35
        }
    }

    Item {
        anchors.fill: parent

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
                anchors.topMargin: root.dp(4)
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

        // 主容器：包含顶部状态、中间主内容区、底部控制栏
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.dp(12)
            spacing: root.dp(6)

            // ================= 1. 顶部状态栏 =================
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: root.dp(36)
                spacing: root.dp(8)

                // 如果在“更多菜谱”页面，左侧显示“返回主控”按钮（苹果微光玻璃胶囊）
                Rectangle {
                    visible: root.currentView === "more"
                    Layout.preferredWidth: backToMainBtn.implicitWidth + root.dp(22)
                    Layout.preferredHeight: root.dp(32)
                    radius: root.dp(16)
                    gradient: Gradient {
                        GradientStop {
                            position: 0.0
                            color: backArea.pressed ? Qt.rgba(0.28, 0.40, 0.54, 0.95) : Qt.rgba(0.18, 0.26, 0.36, 0.88)
                        }
                        GradientStop {
                            position: 1.0
                            color: backArea.pressed ? Qt.rgba(0.18, 0.28, 0.38, 0.95) : Qt.rgba(0.11, 0.16, 0.24, 0.88)
                        }
                    }
                    border.color: backArea.pressed ? Qt.rgba(0.60, 0.82, 1.0, 0.45) : Qt.rgba(1.0, 1.0, 1.0, 0.22)
                    border.width: 1
                    scale: backArea.pressed ? 0.94 : 1.0
                    clip: true

                    Behavior on scale { NumberAnimation { duration: 90 } }


                    RowLayout {
                        id: backToMainBtn
                        anchors.centerIn: parent
                        spacing: root.dp(4)
                        Text {
                            text: "‹"
                            color: "#ffffff"
                            font.pixelSize: root.fs(16)
                            font.bold: true
                        }
                        Text {
                            text: qsTr("返回主控")
                            color: "#ffffff"
                            font.pixelSize: root.fs(12)
                            font.bold: true
                        }
                    }

                    MouseArea {
                        id: backArea
                        anchors.fill: parent
                        onClicked: root.currentView = "main"
                    }
                }

                // 主控视图下的电饭煲图标徽章
                Rectangle {
                    visible: root.currentView === "main"
                    Layout.preferredWidth: root.dp(34)
                    Layout.preferredHeight: root.dp(34)
                    radius: root.dp(17)
                    color: Qt.rgba(0.95, 0.60, 0.15, 0.22)
                    border.color: Qt.rgba(0.95, 0.60, 0.15, 0.45)
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(18)
                        height: root.dp(18)
                        source: "qrc:/icons/cooker.svg"
                        sourceSize: Qt.size(width, height)
                        smooth: true
                    }
                }

                // 设备主标题与当前模式
                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: root.dp(1)

                    Text {
                        text: {
                            if (root.currentView === "more") return qsTr("108道大厨菜谱")
                            return (root.actionModel && root.actionModel.name) ? root.actionModel.name : qsTr("电饭煲")
                        }
                        color: "#ffffff"
                        font.pixelSize: root.fs(15)
                        font.bold: true
                    }

                    Text {
                        text: {
                            if (root.currentView === "more") return qsTr("分类点选 · 详尽做法")
                            if (root.isCooking) return qsTr("正在烹饪: %1").arg(root.runningModeText)
                            if (root.isKeepWarm) return qsTr("保温中: %1").arg(root.runningModeText)
                            return qsTr("当前模式: %1").arg(root.selectedMode)
                        }
                        color: root.isKeepWarm ? "#ffd28a" : (root.isCooking ? "#96f0b4" : "#8aa2b5")
                        font.pixelSize: root.fs(10)
                    }
                }

                // 状态指示徽章（磨砂实体药丸）
                Rectangle {
                    Layout.preferredHeight: root.dp(24)
                    radius: root.dp(12)
                    color: root.isKeepWarm
                           ? Qt.rgba(0.35, 0.22, 0.08, 0.85)
                           : (root.isCooking ? Qt.rgba(0.12, 0.32, 0.18, 0.85) : Qt.rgba(0.12, 0.18, 0.26, 0.85))
                    border.color: root.isKeepWarm
                                  ? Qt.rgba(0.95, 0.60, 0.20, 0.55)
                                  : (root.isCooking ? Qt.rgba(0.20, 0.75, 0.40, 0.55) : Qt.rgba(1.0, 1.0, 1.0, 0.16))
                    border.width: 1
                    implicitWidth: statusRow.implicitWidth + root.dp(12)

                    RowLayout {
                        id: statusRow
                        anchors.centerIn: parent
                        spacing: root.dp(4)

                        Rectangle {
                            width: root.dp(6)
                            height: root.dp(6)
                            radius: root.dp(3)
                            color: root.isKeepWarm ? "#ffb347" : (root.isCooking ? "#52e379" : "#a8c0d2")
                        }

                        Text {
                            text: root.statusText
                            color: root.isKeepWarm ? "#ffc266" : (root.isCooking ? "#52e379" : "#ffffff")
                            font.pixelSize: root.fs(11)
                            font.bold: true
                        }
                    }
                }

                // 实时压力徽章（磨砂实体药丸）
                Rectangle {
                    Layout.preferredHeight: root.dp(24)
                    radius: root.dp(12)
                    color: Qt.rgba(0.12, 0.18, 0.26, 0.85)
                    border.color: Qt.rgba(0.20, 0.60, 0.90, 0.30)
                    border.width: 1
                    implicitWidth: pressRow.implicitWidth + root.dp(12)

                    RowLayout {
                        id: pressRow
                        anchors.centerIn: parent
                        spacing: root.dp(3)

                        Text {
                            text: qsTr("压强")
                            color: "#38c2ff"
                            font.pixelSize: root.fs(9)
                            font.bold: true
                        }

                        Text {
                            text: root.currentPressure + " kPa"
                            color: "#c2efff"
                            font.pixelSize: root.fs(10)
                            font.bold: true
                        }
                    }
                }

                // 锅内温度徽章（磨砂实体药丸）
                Rectangle {
                    Layout.preferredHeight: root.dp(24)
                    radius: root.dp(12)
                    color: Qt.rgba(0.12, 0.18, 0.26, 0.85)
                    border.color: Qt.rgba(0.95, 0.50, 0.20, 0.30)
                    border.width: 1
                    implicitWidth: tempRow.implicitWidth + root.dp(12)

                    RowLayout {
                        id: tempRow
                        anchors.centerIn: parent
                        spacing: root.dp(3)

                        Text {
                            text: qsTr("温度")
                            color: "#ff9542"
                            font.pixelSize: root.fs(9)
                            font.bold: true
                        }

                        Text {
                            text: root.currentTemp !== "--" ? (root.currentTemp + "°C") : "--"
                            color: "#ffd4b2"
                            font.pixelSize: root.fs(10)
                            font.bold: true
                        }
                    }
                }

                // 倒计时 / 预估总耗时徽章（磨砂实体药丸）
                Rectangle {
                    Layout.preferredHeight: root.dp(24)
                    radius: root.dp(12)
                    color: root.isCooking ? Qt.rgba(0.35, 0.25, 0.08, 0.85) : Qt.rgba(0.12, 0.18, 0.26, 0.85)
                    border.color: root.isCooking ? Qt.rgba(1.0, 0.80, 0.20, 0.60) : Qt.rgba(1.0, 1.0, 1.0, 0.16)
                    border.width: 1
                    implicitWidth: timeRow.implicitWidth + root.dp(12)

                    RowLayout {
                        id: timeRow
                        anchors.centerIn: parent
                        spacing: root.dp(3)

                        Text {
                            text: qsTr("耗时")
                            color: root.isCooking ? "#ffd666" : "#a8c0d2"
                            font.pixelSize: root.fs(9)
                            font.bold: true
                        }

                        Text {
                            text: root.isCooking
                                  ? root.leftTimeFormatted
                                  : root.estimatedTimeText
                            color: root.isCooking ? "#ffd666" : "#ffffff"
                            font.pixelSize: root.fs(10)
                            font.bold: true
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                CloseButton {
                    Layout.preferredWidth: root.dp(36)
                    Layout.preferredHeight: root.dp(36)
                    iconSize: root.dp(16)
                    onClicked: root.close()
                }
            }

            // ================= 2. 视图切换区 =================
            // 视图 A: 主控视图（包含参数微调卡片 + 8常用+1更多入口）
            ColumnLayout {
                visible: root.currentView === "main"
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.dp(5)

                // 安全锁与合盖状态
                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.dp(6)

                    Text {
                        text: qsTr("安全状态: ") + root.lidStatus + " · " + root.lockStatus
                        color: "#8cb2cc"
                        font.pixelSize: root.fs(10)
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("轻触选中模式 (自动联动HA) · 点右下角开始烹饪")
                        color: "#7fa6be"
                        font.pixelSize: root.fs(10)
                    }
                }

                // 口感偏好 & 保压时间调节卡片（磨砂实体卡片）
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.dp(68)
                    radius: root.cardRadius
                    color: Qt.rgba(0.10, 0.16, 0.24, 0.85)
                    border.color: Qt.rgba(1.0, 1.0, 1.0, 0.15)
                    border.width: 1
                    clip: true

                    Rectangle {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: root.cardRadius
                        anchors.rightMargin: root.cardRadius
                        height: 1
                        color: "#ffffff"
                        opacity: 0.22
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(7)
                        spacing: root.dp(10)

                        // 左侧：口感偏好
                        ColumnLayout {
                            Layout.preferredWidth: parent.width * 0.44
                            Layout.fillHeight: true
                            spacing: root.dp(3)

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: qsTr("口感偏好")
                                    color: "#b0c7d8"
                                    font.pixelSize: root.fs(11)
                                    font.bold: true
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: root.currentTaste
                                    color: "#52e379"
                                    font.pixelSize: root.fs(11)
                                    font.bold: true
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: root.dp(6)

                                Repeater {
                                    model: root.tasteOptions

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: root.dp(6)
                                        readonly property bool isSelected: root.currentTaste === modelData
                                        color: isSelected
                                               ? Qt.rgba(0.20, 0.70, 0.40, 0.40)
                                               : (tasteArea.pressed ? Qt.rgba(0.20, 0.28, 0.38, 0.90) : Qt.rgba(0.14, 0.20, 0.28, 0.85))
                                        border.color: isSelected
                                                      ? Qt.rgba(0.40, 0.85, 0.55, 0.75)
                                                      : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                        border.width: 1
                                        scale: tasteArea.pressed ? 0.94 : 1.0

                                        Behavior on scale { NumberAnimation { duration: 90 } }
                                        Behavior on color { ColorAnimation { duration: 100 } }

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: parent.isSelected ? "#ffffff" : "#d0e4f0"
                                            font.pixelSize: root.fs(11)
                                            font.bold: parent.isSelected
                                        }

                                        MouseArea {
                                            id: tasteArea
                                            anchors.fill: parent
                                            onClicked: {
                                                appController.setCookerTaste(modelData)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // 垂直分割线
                        Rectangle {
                            Layout.fillHeight: true
                            width: 1
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                        }

                        // 右侧：保压时间
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: root.dp(2)

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: qsTr("保压时间")
                                    color: "#b0c7d8"
                                    font.pixelSize: root.fs(11)
                                    font.bold: true
                                }
                                Text {
                                    text: "(" + root.holdingDurationRange + ")"
                                    color: "#7fa6be"
                                    font.pixelSize: root.fs(9)
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: Math.round(root.effectivePressureTime) + " " + qsTr("分钟")
                                    color: "#ffd666"
                                    font.pixelSize: root.fs(11)
                                    font.bold: true
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: root.dp(6)

                                Rectangle {
                                    Layout.preferredWidth: root.dp(26)
                                    Layout.preferredHeight: root.dp(26)
                                    radius: root.dp(13)
                                    gradient: Gradient {
                                        GradientStop {
                                            position: 0.0
                                            color: minusArea.pressed ? Qt.rgba(0.32, 0.44, 0.58, 0.95) : Qt.rgba(0.20, 0.28, 0.38, 0.90)
                                        }
                                        GradientStop {
                                            position: 1.0
                                            color: minusArea.pressed ? Qt.rgba(0.20, 0.30, 0.42, 0.95) : Qt.rgba(0.12, 0.18, 0.26, 0.90)
                                        }
                                    }
                                    border.color: minusArea.pressed ? Qt.rgba(0.65, 0.85, 1.0, 0.50) : Qt.rgba(1.0, 1.0, 1.0, 0.18)
                                    border.width: 1
                                    scale: minusArea.pressed ? 0.90 : 1.0
                                    Behavior on scale { NumberAnimation { duration: 80 } }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "-"
                                        color: "#ffffff"
                                        font.pixelSize: root.fs(15)
                                        font.bold: true
                                    }

                                    MouseArea {
                                        id: minusArea
                                        anchors.fill: parent
                                        onClicked: {
                                            var val = Math.max(root.pressureMin, Math.round(root.effectivePressureTime - 1))
                                            root.customPressureTime = val
                                            if (!root.isStrictCooking) {
                                                appController.setCookerPressureTime(val)
                                            }
                                        }
                                    }
                                }

                                Slider {
                                    id: pressureSlider
                                    Layout.fillWidth: true
                                    from: root.pressureMin
                                    to: root.pressureMax
                                    stepSize: 1.0
                                    value: root.effectivePressureTime
                                    Binding on value {
                                        when: !pressureSlider.pressed
                                        value: root.effectivePressureTime
                                    }
                                    onMoved: {
                                        var val = Math.round(value)
                                        root.customPressureTime = val
                                        if (!root.isStrictCooking) {
                                            appController.setCookerPressureTime(val)
                                        }
                                    }

                                    background: Rectangle {
                                        x: pressureSlider.leftPadding
                                        y: pressureSlider.topPadding + pressureSlider.availableHeight / 2 - height / 2
                                        implicitHeight: root.dp(6)
                                        width: pressureSlider.availableWidth
                                        height: implicitHeight
                                        radius: height / 2
                                        color: Qt.rgba(0.05, 0.08, 0.13, 0.85)

                                        Rectangle {
                                            width: pressureSlider.visualPosition * parent.width
                                            height: parent.height
                                            radius: parent.radius
                                            color: Qt.rgba(1.0, 0.80, 0.20, 0.95)
                                        }
                                    }

                                    handle: Rectangle {
                                        x: pressureSlider.leftPadding + pressureSlider.visualPosition * (pressureSlider.availableWidth - width)
                                        y: pressureSlider.topPadding + pressureSlider.availableHeight / 2 - height / 2
                                        implicitWidth: root.dp(18)
                                        implicitHeight: root.dp(18)
                                        radius: width / 2
                                        color: "#ffffff"
                                        border.color: Qt.rgba(0.0, 0.0, 0.0, 0.20)
                                        border.width: 1
                                        scale: pressureSlider.pressed ? 1.15 : 1.0

                                        Behavior on scale { NumberAnimation { duration: 80 } }
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: root.dp(26)
                                    Layout.preferredHeight: root.dp(26)
                                    radius: root.dp(13)
                                    gradient: Gradient {
                                        GradientStop {
                                            position: 0.0
                                            color: plusArea.pressed ? Qt.rgba(0.32, 0.44, 0.58, 0.95) : Qt.rgba(0.20, 0.28, 0.38, 0.90)
                                        }
                                        GradientStop {
                                            position: 1.0
                                            color: plusArea.pressed ? Qt.rgba(0.20, 0.30, 0.42, 0.95) : Qt.rgba(0.12, 0.18, 0.26, 0.90)
                                        }
                                    }
                                    border.color: plusArea.pressed ? Qt.rgba(0.65, 0.85, 1.0, 0.50) : Qt.rgba(1.0, 1.0, 1.0, 0.18)
                                    border.width: 1
                                    scale: plusArea.pressed ? 0.90 : 1.0
                                    clip: true

                                    Behavior on scale { NumberAnimation { duration: 80 } }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "+"
                                        color: "#ffffff"
                                        font.pixelSize: root.fs(15)
                                        font.bold: true
                                    }

                                    MouseArea {
                                        id: plusArea
                                        anchors.fill: parent
                                        onClicked: {
                                            var val = Math.min(root.pressureMax, Math.round(root.effectivePressureTime + 1))
                                            root.customPressureTime = val
                                            if (!root.isStrictCooking) {
                                                appController.setCookerPressureTime(val)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 常用菜谱标题行
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("精选常用菜谱")
                        color: "#b0c7d8"
                        font.pixelSize: root.fs(11)
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: qsTr("共 108 道名厨菜谱 ›")
                        color: "#60d0ff"
                        font.pixelSize: root.fs(10)
                        font.bold: true
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentView = "more"
                        }
                    }
                }

                // 核心 3x3 布局网格：8 道常用菜谱 + 1 个【更多菜谱(108道)】
                GridLayout {
                    id: commonGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 3
                    columnSpacing: root.dp(6)
                    rowSpacing: root.dp(6)

                    // 1 ~ 8: 常用菜谱项
                    Repeater {
                        model: root.commonRecipes

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: root.chipRadius
                            readonly property bool isRunningThis: root.isStrictCooking && (root.runningModeText === modelData.mode || root.runningModeText === modelData.name)
                            readonly property bool isSelected: isRunningThis || (root.selectedMode === modelData.mode)
                            color: isRunningThis
                                   ? Qt.rgba(0.18, 0.75, 0.38, 0.35)
                                   : (isSelected
                                      ? Qt.rgba(0.0, 0.45, 0.95, 0.32)
                                      : (commonItemArea.pressed ? Qt.rgba(0.16, 0.24, 0.34, 0.90) : Qt.rgba(0.11, 0.17, 0.25, 0.85)))
                            border.color: isRunningThis
                                          ? Qt.rgba(0.40, 0.85, 0.55, 0.85)
                                          : (isSelected ? Qt.rgba(0.40, 0.80, 1.0, 0.85) : (commonItemArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.22) : Qt.rgba(1.0, 1.0, 1.0, 0.14)))
                            border.width: (isRunningThis || isSelected) ? 1.5 : 1
                            scale: commonItemArea.pressed ? 0.96 : 1.0
                            clip: true

                            Behavior on scale { NumberAnimation { duration: 90 } }
                            Behavior on color { ColorAnimation { duration: 100 } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: root.dp(7)
                                spacing: root.dp(6)

                                Rectangle {
                                    Layout.preferredWidth: root.dp(38)
                                    Layout.preferredHeight: root.dp(38)
                                    radius: root.dp(9)
                                    color: isRunningThis ? Qt.rgba(0.20, 0.70, 0.40, 0.25) : (isSelected ? Qt.rgba(0.20, 0.60, 0.90, 0.25) : Qt.rgba(0.96, 0.62, 0.15, 0.20))
                                    border.color: isRunningThis ? "#50e879" : (isSelected ? "#72d2ff" : Qt.rgba(0.96, 0.62, 0.15, 0.45))
                                    border.width: 1
                                    Layout.alignment: Qt.AlignVCenter

                                    Image {
                                        anchors.centerIn: parent
                                        width: root.dp(22)
                                        height: root.dp(22)
                                        sourceSize.width: root.dp(44)
                                        sourceSize.height: root.dp(44)
                                        source: RecipesData.getRecipeIcon(modelData.name, modelData.category)
                                        fillMode: Image.PreserveAspectFit
                                        smooth: true
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignVCenter
                                    spacing: 2

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            text: modelData.name
                                            color: isRunningThis ? "#50e879" : (isSelected ? "#72d2ff" : "#ffffff")
                                            font.pixelSize: root.fs(13)
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: isRunningThis ? qsTr("运行中") : qsTr("已选")
                                            color: isRunningThis ? "#ffd666" : "#72d2ff"
                                            font.pixelSize: root.fs(10)
                                            font.bold: true
                                            visible: isSelected
                                        }
                                    }

                                    Text {
                                        text: {
                                            var t = root.getRecipeEstimatedTime(modelData.mode)
                                            return t ? (modelData.desc.split("，")[0] + " · " + t) : modelData.desc
                                        }
                                        color: isSelected ? "#c2e9ff" : "#a5bed0"
                                        font.pixelSize: root.fs(10)
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            MouseArea {
                                id: commonItemArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.selectRecipeAndSync(modelData)
                                }
                            }
                        }
                    }

                    // 9. 【更多菜谱 (108道)】特别入口卡片（磨砂实体琥珀金）
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: root.chipRadius
                        color: moreCardArea.pressed ? Qt.rgba(0.32, 0.25, 0.12, 0.90) : Qt.rgba(0.24, 0.19, 0.10, 0.85)
                        border.color: moreCardArea.containsMouse ? Qt.rgba(1.0, 0.85, 0.35, 0.70) : Qt.rgba(1.0, 0.80, 0.25, 0.50)
                        border.width: 1.5
                        scale: moreCardArea.pressed ? 0.96 : 1.0
                        clip: true

                        Behavior on scale { NumberAnimation { duration: 90 } }
                        Behavior on color { ColorAnimation { duration: 100 } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: root.dp(7)
                            spacing: root.dp(6)

                            Rectangle {
                                Layout.preferredWidth: root.dp(34)
                                Layout.preferredHeight: root.dp(34)
                                radius: root.dp(8)
                                color: Qt.rgba(1.0, 0.80, 0.20, 0.30)
                                border.color: Qt.rgba(1.0, 0.80, 0.20, 0.55)
                                border.width: 1

                                Image {
                                    anchors.centerIn: parent
                                    width: root.dp(18)
                                    height: root.dp(18)
                                    sourceSize.width: root.dp(18)
                                    sourceSize.height: root.dp(18)
                                    source: "qrc:/icons/cooker-chef.svg"
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 2

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: qsTr("更多菜谱")
                                        color: "#ffdf7a"
                                        font.pixelSize: root.fs(13)
                                        font.bold: true
                                    }
                                    Rectangle {
                                        Layout.preferredHeight: root.dp(18)
                                        radius: root.dp(9)
                                        color: Qt.rgba(1.0, 0.80, 0.20, 0.35)
                                        border.color: Qt.rgba(1.0, 0.80, 0.20, 0.60)
                                        border.width: 1
                                        implicitWidth: countText.implicitWidth + root.dp(8)

                                        Text {
                                            id: countText
                                            anchors.centerIn: parent
                                            text: "108道"
                                            color: "#fff3c4"
                                            font.pixelSize: root.fs(10)
                                            font.bold: true
                                        }
                                    }
                                }

                                Text {
                                    text: qsTr("六大类名厨菜肴 · 做法详解")
                                    color: "#ded2af"
                                    font.pixelSize: root.fs(10)
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Text {
                                text: "›"
                                color: "#ffdf7a"
                                font.pixelSize: root.fs(18)
                                font.bold: true
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        MouseArea {
                            id: moreCardArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.currentView = "more"
                                var list = RecipesData.getRecipesByCategory(root.selectedCategory, "")
                                if (list && list.length > 0) {
                                    root.activeRecipe = list[0]
                                }
                            }
                        }
                    }
                }
            }

            // 视图 B: “更多菜谱”分类大厅视图（磨砂实体质感：左侧选菜列表，右侧实时大厨详尽做法与 HA 官方食谱同步）
            ColumnLayout {
                visible: root.currentView === "more"
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.dp(6)

                // 水平滚动分类标签栏（磨砂实体药丸轨道）
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.dp(30)
                    color: Qt.rgba(0.08, 0.13, 0.19, 0.90)
                    radius: root.dp(8)
                    border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                    border.width: 1

                    Flickable {
                        anchors.fill: parent
                        anchors.margins: root.dp(3)
                        contentWidth: categoryRow.implicitWidth + root.dp(8)
                        contentHeight: height
                        boundsBehavior: Flickable.StopAtBounds
                        flickableDirection: Flickable.HorizontalFlick

                        RowLayout {
                            id: categoryRow
                            height: parent.height
                            spacing: root.dp(4)

                            Repeater {
                                model: RecipesData.categories

                                Rectangle {
                                    Layout.preferredHeight: parent.height
                                    radius: root.dp(6)
                                    readonly property bool isCatActive: root.selectedCategory === modelData.id
                                    color: isCatActive
                                           ? Qt.rgba(0.20, 0.70, 0.40, 0.45)
                                           : (catArea.pressed ? Qt.rgba(0.18, 0.26, 0.36, 0.90) : Qt.rgba(0.12, 0.18, 0.25, 0.75))
                                    border.color: isCatActive ? Qt.rgba(0.40, 0.85, 0.55, 0.85) : Qt.rgba(1.0, 1.0, 1.0, 0.10)
                                    border.width: 1
                                    implicitWidth: catRow.implicitWidth + root.dp(12)
                                    scale: catArea.pressed ? 0.94 : 1.0

                                    Behavior on scale { NumberAnimation { duration: 90 } }
                                    Behavior on color { ColorAnimation { duration: 100 } }

                                    RowLayout {
                                        id: catRow
                                        anchors.centerIn: parent
                                        spacing: root.dp(4)

                                        Image {
                                            width: root.dp(12)
                                            height: root.dp(12)
                                            sourceSize.width: root.dp(12)
                                            sourceSize.height: root.dp(12)
                                            source: modelData.icon || "qrc:/icons/cooker-chef.svg"
                                            fillMode: Image.PreserveAspectFit
                                            smooth: true
                                        }

                                        Text {
                                            text: modelData.name + " (" + modelData.count + ")"
                                            color: isCatActive ? "#ffffff" : "#a6bed0"
                                            font.pixelSize: root.fs(10)
                                            font.bold: isCatActive
                                        }
                                    }

                                    MouseArea {
                                        id: catArea
                                        anchors.fill: parent
                                        onClicked: {
                                            root.selectedCategory = modelData.id
                                            var list = RecipesData.getRecipesByCategory(modelData.id, "")
                                            if (list && list.length > 0) {
                                                root.selectRecipeAndSync(list[0])
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 主体区域：左右联动双栏（左侧当前分类菜品，右侧实时做法详解）
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: root.dp(6)

                    // 1. 左侧：当前分类菜品选择列表（磨砂实体深底微容器）
                    Rectangle {
                        Layout.preferredWidth: root.dp(230)
                        Layout.fillHeight: true
                        radius: root.dp(10)
                        color: Qt.rgba(0.09, 0.14, 0.21, 0.85)
                        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.14)
                        border.width: 1
                        clip: true

                        Flickable {
                            id: allRecipesFlickable
                            anchors.fill: parent
                            anchors.margins: root.dp(4)
                            contentWidth: width
                            contentHeight: allListCol.implicitHeight + root.dp(8)
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.VerticalFlick

                            ScrollBar.vertical: ScrollBar {
                                anchors.right: parent.right
                                anchors.rightMargin: -root.dp(2)
                                policy: ScrollBar.AsNeeded
                                width: root.dp(4)
                            }

                            ColumnLayout {
                                id: allListCol
                                width: parent.width - root.dp(6)
                                spacing: root.dp(4)

                                Repeater {
                                    model: RecipesData.getRecipesByCategory(root.selectedCategory, "")

                                    Rectangle {
                                        Layout.fillWidth: true
                                        implicitHeight: root.dp(46)
                                        radius: root.dp(6)
                                        readonly property bool isCurrentViewRecipe: root.activeRecipe && (root.activeRecipe.name === modelData.name || root.activeRecipe.mode === modelData.mode)
                                        readonly property bool isRunningThis: root.isCooking && (root.runningModeText === modelData.mode || root.runningModeText === modelData.name)

                                        color: isRunningThis
                                               ? Qt.rgba(0.18, 0.75, 0.38, 0.35)
                                               : (isCurrentViewRecipe
                                                  ? Qt.rgba(0.0, 0.45, 0.95, 0.32)
                                                  : (itemArea.pressed ? Qt.rgba(0.18, 0.26, 0.36, 0.90) : Qt.rgba(0.12, 0.18, 0.26, 0.85)))
                                        border.color: isRunningThis
                                                      ? Qt.rgba(0.40, 0.85, 0.55, 0.85)
                                                      : (isCurrentViewRecipe ? Qt.rgba(0.40, 0.80, 1.0, 0.85) : (itemArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.20) : Qt.rgba(1.0, 1.0, 1.0, 0.12)))
                                        border.width: (isRunningThis || isCurrentViewRecipe) ? 1.5 : 1
                                        scale: itemArea.pressed ? 0.96 : 1.0

                                        Behavior on scale { NumberAnimation { duration: 90 } }
                                        Behavior on color { ColorAnimation { duration: 100 } }

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: root.dp(4)
                                            spacing: root.dp(4)

                                            Rectangle {
                                                Layout.preferredWidth: root.dp(26)
                                                Layout.preferredHeight: root.dp(26)
                                                radius: root.dp(6)
                                                color: isRunningThis ? Qt.rgba(0.20, 0.70, 0.40, 0.25) : (isCurrentViewRecipe ? Qt.rgba(0.20, 0.60, 0.90, 0.25) : Qt.rgba(0.96, 0.62, 0.15, 0.20))
                                                border.color: isRunningThis ? "#50e879" : (isCurrentViewRecipe ? "#72d2ff" : Qt.rgba(0.96, 0.62, 0.15, 0.40))
                                                border.width: 1
                                                Layout.alignment: Qt.AlignVCenter

                                                Image {
                                                    anchors.centerIn: parent
                                                    width: root.dp(18)
                                                    height: root.dp(18)
                                                    sourceSize.width: root.dp(36)
                                                    sourceSize.height: root.dp(36)
                                                    source: RecipesData.getRecipeIcon(modelData.name, modelData.category)
                                                    fillMode: Image.PreserveAspectFit
                                                    smooth: true
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                Layout.alignment: Qt.AlignVCenter
                                                spacing: 1

                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    Text {
                                                        text: modelData.name
                                                        color: isRunningThis ? "#50e879" : (isCurrentViewRecipe ? "#72d2ff" : "#ffffff")
                                                        font.pixelSize: root.fs(11)
                                                        font.bold: isCurrentViewRecipe
                                                        elide: Text.ElideRight
                                                        Layout.fillWidth: true
                                                    }
                                                    Text {
                                                        text: isRunningThis ? qsTr("运行中") : (isCurrentViewRecipe ? qsTr("查看中") : "")
                                                        color: isRunningThis ? "#ffd666" : "#72d2ff"
                                                        font.pixelSize: root.fs(9)
                                                        font.bold: true
                                                        visible: isCurrentViewRecipe || isRunningThis
                                                    }
                                                }

                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: root.dp(3)
                                                    Text {
                                                        text: (root.getRecipeEstimatedTime(modelData.mode) || modelData.estimatedTime)
                                                        color: isCurrentViewRecipe ? "#c2e9ff" : "#ffd666"
                                                        font.pixelSize: root.fs(8)
                                                        font.bold: true
                                                    }
                                                    Text {
                                                        text: "· 保压" + modelData.pressureTime
                                                        color: "#9db3c4"
                                                        font.pixelSize: root.fs(8)
                                                    }
                                                }
                                            }
                                        }

                                        MouseArea {
                                            id: itemArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onClicked: {
                                                root.selectRecipeAndSync(modelData)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 2. 右侧：当前选中菜品大厨详尽做法面板 (实时展示，支持 HA 官方食谱与本地名厨做法)
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: root.dp(10)
                        color: Qt.rgba(0.09, 0.14, 0.21, 0.85)
                        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.14)
                        border.width: 1
                        clip: true

                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: root.dp(10)
                            anchors.rightMargin: root.dp(10)
                            height: 1
                            color: "#ffffff"
                            opacity: 0.18
                        }

                        Flickable {
                            id: moreDetailFlickable
                            anchors.fill: parent
                            anchors.margins: root.dp(8)
                            contentWidth: width
                            contentHeight: moreDetailCol.implicitHeight + root.dp(16)
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.VerticalFlick

                            ScrollBar.vertical: ScrollBar {
                                anchors.right: parent.right
                                anchors.rightMargin: -root.dp(2)
                                policy: ScrollBar.AsNeeded
                                width: root.dp(4)
                            }

                            ColumnLayout {
                                id: moreDetailCol
                                width: parent.width - root.dp(6)
                                spacing: root.dp(8)

                                // (1) 菜品头部标题栏与数据源指示
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: root.dp(6)

                                    Rectangle {
                                        Layout.preferredWidth: root.dp(30)
                                        Layout.preferredHeight: root.dp(30)
                                        radius: root.dp(6)
                                        color: Qt.rgba(0.96, 0.62, 0.15, 0.20)
                                        border.color: Qt.rgba(0.96, 0.62, 0.15, 0.40)
                                        border.width: 1
                                        Layout.alignment: Qt.AlignVCenter

                                        Image {
                                            anchors.centerIn: parent
                                            width: root.dp(20)
                                            height: root.dp(20)
                                            sourceSize.width: root.dp(40)
                                            sourceSize.height: root.dp(40)
                                            source: RecipesData.getRecipeIcon(root.activeRecipe ? root.activeRecipe.name : "", root.activeRecipe ? root.activeRecipe.category : "")
                                            fillMode: Image.PreserveAspectFit
                                            smooth: true
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text {
                                                text: root.activeRecipe ? root.activeRecipe.name : ""
                                                color: "#ffffff"
                                                font.pixelSize: root.fs(14)
                                                font.bold: true
                                                elide: Text.ElideRight
                                            }

                                            // 工艺标签
                                            Rectangle {
                                                visible: root.detailPractice !== ""
                                                radius: root.dp(3)
                                                color: Qt.rgba(1.0, 0.80, 0.20, 0.25)
                                                border.color: Qt.rgba(1.0, 0.80, 0.20, 0.50)
                                                border.width: 1
                                                implicitWidth: detailPracticeLabel.implicitWidth + root.dp(6)
                                                implicitHeight: root.dp(15)

                                                Text {
                                                    id: detailPracticeLabel
                                                    anchors.centerIn: parent
                                                    text: root.detailPractice
                                                    color: "#ffd666"
                                                    font.pixelSize: root.fs(8)
                                                    font.bold: true
                                                }
                                            }

                                            // 数据源标识（绿色：HA 官方实时同步；蓝色：名厨做法）
                                            Rectangle {
                                                radius: root.dp(3)
                                                color: root.isCurrentRecipeFromHa ? Qt.rgba(0.15, 0.65, 0.35, 0.30) : Qt.rgba(0.15, 0.40, 0.70, 0.30)
                                                border.color: root.isCurrentRecipeFromHa ? Qt.rgba(0.40, 0.85, 0.55, 0.65) : Qt.rgba(0.35, 0.65, 0.95, 0.55)
                                                border.width: 1
                                                implicitWidth: sourceRow.implicitWidth + root.dp(8)
                                                implicitHeight: root.dp(15)

                                                RowLayout {
                                                    id: sourceRow
                                                    anchors.centerIn: parent
                                                    spacing: root.dp(3)

                                                    Rectangle {
                                                        width: root.dp(5)
                                                        height: root.dp(5)
                                                        radius: root.dp(2.5)
                                                        color: root.isCurrentRecipeFromHa ? "#52e379" : "#80d0ff"
                                                        Layout.alignment: Qt.AlignVCenter
                                                    }
                                                    Text {
                                                        text: root.isCurrentRecipeFromHa ? qsTr("HA 官方同步") : qsTr("名厨做法")
                                                        color: root.isCurrentRecipeFromHa ? "#52e379" : "#80d0ff"
                                                        font.pixelSize: root.fs(8)
                                                        font.bold: true
                                                    }
                                                }
                                            }

                                            Item { Layout.fillWidth: true }
                                        }

                                        // 耗时与保压
                                        RowLayout {
                                            spacing: root.dp(6)
                                            Text {
                                                text: "耗时: " + (root.activeRecipe ? (root.getRecipeEstimatedTime(root.activeRecipe.mode) || root.activeRecipe.estimatedTime) : "")
                                                color: "#ffd666"
                                                font.pixelSize: root.fs(9)
                                                font.bold: true
                                            }
                                            Text {
                                                text: "· 保压: " + (root.activeRecipe ? root.activeRecipe.pressureTime : "")
                                                color: "#72d2ff"
                                                font.pixelSize: root.fs(9)
                                            }
                                        }
                                    }
                                }

                                // (2) 菜品特色简介
                                Rectangle {
                                    Layout.fillWidth: true
                                    radius: root.dp(6)
                                    color: Qt.rgba(0.12, 0.18, 0.26, 0.85)
                                    border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                    border.width: 1
                                    implicitHeight: moreDescText.implicitHeight + root.dp(12)

                                    Text {
                                        id: moreDescText
                                        x: root.dp(6)
                                        y: root.dp(6)
                                        width: Math.max(10, parent.width - root.dp(12))
                                        text: root.detailDescription
                                        color: "#c8dceb"
                                        font.pixelSize: root.fs(10)
                                        wrapMode: Text.WordWrap
                                        lineHeight: 1.15
                                    }
                                }

                                // (3) 所需食材清单
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: root.dp(4)

                                    RowLayout {
                                        spacing: root.dp(4)
                                        Text {
                                            text: qsTr("所需食材清单")
                                            color: "#ffffff"
                                            font.pixelSize: root.fs(11)
                                            font.bold: true
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            visible: root.detailIngredients.length > 0
                                            text: "共 " + root.detailIngredients.length + " 样"
                                            color: "#8cb2cc"
                                            font.pixelSize: root.fs(8)
                                        }
                                    }

                                    Flow {
                                        Layout.fillWidth: true
                                        spacing: root.dp(4)

                                        Repeater {
                                            model: root.detailIngredients

                                            Rectangle {
                                                radius: root.dp(4)
                                                color: Qt.rgba(0.16, 0.23, 0.32, 0.90)
                                                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.16)
                                                border.width: 1
                                                implicitHeight: root.dp(20)
                                                implicitWidth: moreIngText.implicitWidth + root.dp(10)

                                                Text {
                                                    id: moreIngText
                                                    anchors.centerIn: parent
                                                    text: modelData
                                                    color: "#e4f1fb"
                                                    font.pixelSize: root.fs(9)
                                                }
                                            }
                                        }
                                    }
                                }

                                // (4) 制作步骤
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: root.dp(4)

                                    RowLayout {
                                        spacing: root.dp(4)
                                        Text {
                                            text: qsTr("大厨烹饪步骤")
                                            color: "#ffffff"
                                            font.pixelSize: root.fs(11)
                                            font.bold: true
                                        }
                                    }

                                    Repeater {
                                        model: root.detailSteps

                                        Rectangle {
                                            Layout.fillWidth: true
                                            radius: root.dp(6)
                                            color: Qt.rgba(0.13, 0.19, 0.27, 0.85)
                                            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                            border.width: 1
                                            implicitHeight: Math.max(root.dp(26), moreStepTxt.implicitHeight + root.dp(10))

                                            Rectangle {
                                                id: moreStepNum
                                                x: root.dp(5)
                                                y: root.dp(5)
                                                width: root.dp(16)
                                                height: root.dp(16)
                                                radius: root.dp(8)
                                                color: Qt.rgba(0.20, 0.70, 0.40, 0.35)
                                                border.color: Qt.rgba(0.40, 0.85, 0.55, 0.75)
                                                border.width: 1

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: String(index + 1)
                                                    color: "#52e379"
                                                    font.pixelSize: root.fs(8)
                                                    font.bold: true
                                                }
                                            }

                                            Text {
                                                id: moreStepTxt
                                                x: root.dp(26)
                                                y: root.dp(5)
                                                width: Math.max(10, parent.width - root.dp(32))
                                                text: modelData
                                                color: "#e4f1fb"
                                                font.pixelSize: root.fs(10)
                                                wrapMode: Text.WordWrap
                                                lineHeight: 1.15
                                            }
                                        }
                                    }
                                }

                                // (5) 大厨贴士
                                Rectangle {
                                    visible: root.detailTips !== ""
                                    Layout.fillWidth: true
                                    radius: root.dp(6)
                                    color: Qt.rgba(0.24, 0.20, 0.10, 0.85)
                                    border.color: Qt.rgba(1.0, 0.80, 0.20, 0.45)
                                    border.width: 1
                                    implicitHeight: moreTipsTitleRow.implicitHeight + moreTipsTxt.implicitHeight + root.dp(14)

                                    RowLayout {
                                        id: moreTipsTitleRow
                                        x: root.dp(6)
                                        y: root.dp(5)
                                        spacing: root.dp(4)
                                        Text {
                                            text: qsTr("大厨私房贴士")
                                            color: "#ffd666"
                                            font.pixelSize: root.fs(10)
                                            font.bold: true
                                        }
                                    }

                                    Text {
                                        id: moreTipsTxt
                                        x: root.dp(6)
                                        y: moreTipsTitleRow.y + moreTipsTitleRow.implicitHeight + root.dp(3)
                                        width: Math.max(10, parent.width - root.dp(12))
                                        text: root.detailTips
                                        color: "#fff3d4"
                                        font.pixelSize: root.fs(9)
                                        wrapMode: Text.WordWrap
                                        lineHeight: 1.15
                                    }
                                }

                                // (6) 底部烹饪动作条（苹果微光玻璃胶囊按钮）
                                GlassButton {
                                    id: moreStartRecipeGlassBtn
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: root.dp(32)
                                    scaleUnit: root.scaleUnit
                                    readonly property bool isThisRunning: root.isCooking && root.activeRecipe && (root.runningModeText === root.activeRecipe.mode || root.runningModeText === root.activeRecipe.name)
                                    readonly property bool isOtherRunning: root.isCooking && !isThisRunning
                                    type: isThisRunning ? "accent" : (isOtherRunning ? "secondary" : "primary")
                                    enabled: !isOtherRunning && !isThisRunning
                                    text: {
                                        if (isThisRunning) return qsTr("当前正在烹饪此菜品")
                                        if (isOtherRunning) return qsTr("设备正在运行其他程序 (已安全锁止)")
                                        return qsTr("以此菜谱开始烹饪")
                                    }
                                    onClicked: {
                                        if (root.activeRecipe) {
                                            root.applyAndCookRecipe(root.activeRecipe)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ================= 3. 底部固定控制栏 (仅在主界面显示，毛玻璃实体底座) =================
            Rectangle {
                visible: root.currentView === "main"
                Layout.fillWidth: true
                Layout.preferredHeight: root.dp(42)
                radius: root.chipRadius
                color: root.isStrictCooking ? Qt.rgba(0.70, 0.15, 0.15, 0.35) : Qt.rgba(0.10, 0.16, 0.24, 0.92)
                border.color: root.isStrictCooking ? Qt.rgba(0.95, 0.35, 0.35, 0.60) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                border.width: 1
                clip: true

                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root.chipRadius
                    anchors.rightMargin: root.chipRadius
                    height: 1
                    color: "#ffffff"
                    opacity: root.isStrictCooking ? 0.35 : 0.20
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: root.dp(7)
                    spacing: root.dp(8)

                    Rectangle {
                        width: root.dp(8)
                        height: root.dp(8)
                        radius: root.dp(4)
                        color: root.isStrictCooking ? "#ff5252" : (root.isKeepWarm ? "#ffb347" : "#50e879")
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        Layout.fillWidth: true
                        text: {
                            if (root.isStrictCooking) {
                                return qsTr("正在运行: %1 · 剩余 %2").arg(root.runningModeText !== "" ? root.runningModeText : root.selectedMode).arg(root.leftTimeFormatted)
                            }
                            if (root.isKeepWarm) {
                                return qsTr("保温中 (%1) · 已选待煮: %2 · 保压 %3分钟").arg(root.leftTimeFormatted).arg(root.selectedMode).arg(Math.round(root.effectivePressureTime))
                            }
                            return qsTr("已选模式: %1 (%2) · %3 · 保压 %4分钟").arg(root.selectedMode).arg(root.estimatedTimeText).arg(root.currentTaste).arg(Math.round(root.effectivePressureTime))
                        }
                        color: root.isStrictCooking ? "#ffd4d4" : (root.isKeepWarm ? "#ffe0b3" : "#e0eef7")
                        font.pixelSize: root.fs(11)
                        font.bold: root.isStrictCooking
                        elide: Text.ElideRight
                    }

                    // 右下角：开始 / 停止烹饪核心操作按钮（苹果微光玻璃胶囊）
                    GlassButton {
                        Layout.preferredWidth: root.isStrictCooking ? root.dp(92) : root.dp(108)
                        Layout.preferredHeight: root.dp(30)
                        scaleUnit: root.scaleUnit
                        styleType: root.isStrictCooking ? "danger" : "primary"
                        text: root.isStrictCooking ? qsTr("停止烹饪") : qsTr("开始烹饪")
                        textPixelSize: root.fs(11)
                        onClicked: {
                            if (root.isStrictCooking) {
                                appController.cancelCooker()
                            } else {
                                if (root.customPressureTime > 0) {
                                    appController.setCookerPressureTime(root.customPressureTime)
                                }
                                appController.startCooker(root.selectedMode, "")
                            }
                        }
                    }
                }
            }
        }

        // ================= 4. 菜谱做法详情全屏浮层 (Recipe Detail Overlay - iOS 高对比液态磨砂) =================
        Rectangle {
            id: recipeDetailOverlay
            z: 100
            anchors.fill: parent
            visible: root.showRecipeDetail && root.activeRecipe !== null
            color: Qt.rgba(0.08, 0.12, 0.18, 0.98)
            radius: root.panelRadius
            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.22)
            border.width: 1
            clip: true

            // 左上浅冰蓝次表面漫反射微光
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: -root.dp(50)
                width: root.dp(220)
                height: root.dp(220)
                radius: width / 2
                color: Qt.rgba(0.20, 0.58, 0.95, 0.10)
            }

            // 右下紫晶微暖光晕
            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: -root.dp(50)
                width: root.dp(200)
                height: root.dp(200)
                radius: width / 2
                color: Qt.rgba(0.42, 0.22, 0.68, 0.06)
            }

            // 顶部 1px 月白光折射线
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root.panelRadius
                anchors.rightMargin: root.panelRadius
                height: 1
                color: "#ffffff"
                opacity: 0.30
            }

            // 阻断底层点击穿透
            MouseArea {
                anchors.fill: parent
                onClicked: {}
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: root.dp(14)
                spacing: root.dp(8)

                // 详情头部
                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.dp(8)

                    Rectangle {
                        Layout.preferredWidth: root.dp(34)
                        Layout.preferredHeight: root.dp(34)
                        radius: root.dp(8)
                        color: Qt.rgba(0.96, 0.62, 0.15, 0.22)
                        border.color: Qt.rgba(0.96, 0.62, 0.15, 0.45)
                        border.width: 1
                        Layout.alignment: Qt.AlignVCenter

                        Image {
                            anchors.centerIn: parent
                            width: root.dp(22)
                            height: root.dp(22)
                            sourceSize.width: root.dp(44)
                            sourceSize.height: root.dp(44)
                            source: RecipesData.getRecipeIcon(root.activeRecipe ? root.activeRecipe.name : "", root.activeRecipe ? root.activeRecipe.category : "")
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: root.activeRecipe ? root.activeRecipe.name : ""
                                color: "#ffffff"
                                font.pixelSize: root.fs(16)
                                font.bold: true
                            }

                            Rectangle {
                                radius: root.dp(4)
                                color: Qt.rgba(0.20, 0.70, 0.40, 0.25)
                                border.color: Qt.rgba(0.40, 0.85, 0.55, 0.65)
                                border.width: 1
                                implicitWidth: catLabel.implicitWidth + root.dp(8)
                                implicitHeight: root.dp(16)

                                Text {
                                    id: catLabel
                                    anchors.centerIn: parent
                                    text: root.activeRecipe ? root.activeRecipe.categoryName : ""
                                    color: "#52e379"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }

                            // 烹饪工艺标签（如 "红烧"、"煲汤" 等）
                            Rectangle {
                                visible: root.detailPractice !== ""
                                radius: root.dp(4)
                                color: Qt.rgba(1.0, 0.80, 0.20, 0.25)
                                border.color: Qt.rgba(1.0, 0.80, 0.20, 0.50)
                                border.width: 1
                                implicitWidth: practiceLabel.implicitWidth + root.dp(8)
                                implicitHeight: root.dp(16)

                                Text {
                                    id: practiceLabel
                                    anchors.centerIn: parent
                                    text: root.detailPractice
                                    color: "#ffd666"
                                    font.pixelSize: root.fs(9)
                                    font.bold: true
                                }
                            }

                            // 数据源指示胶囊
                            Rectangle {
                                radius: root.dp(4)
                                color: root.isCurrentRecipeFromHa ? Qt.rgba(0.15, 0.65, 0.35, 0.30) : Qt.rgba(0.15, 0.40, 0.70, 0.30)
                                border.color: root.isCurrentRecipeFromHa ? Qt.rgba(0.40, 0.85, 0.55, 0.65) : Qt.rgba(0.35, 0.65, 0.95, 0.55)
                                border.width: 1
                                implicitWidth: overlaySourceRow.implicitWidth + root.dp(8)
                                implicitHeight: root.dp(16)

                                RowLayout {
                                    id: overlaySourceRow
                                    anchors.centerIn: parent
                                    spacing: root.dp(3)

                                    Rectangle {
                                        width: root.dp(5)
                                        height: root.dp(5)
                                        radius: root.dp(2.5)
                                        color: root.isCurrentRecipeFromHa ? "#52e379" : "#80d0ff"
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    Text {
                                        text: root.isCurrentRecipeFromHa ? qsTr("HA 官方同步") : qsTr("名厨做法")
                                        color: root.isCurrentRecipeFromHa ? "#52e379" : "#80d0ff"
                                        font.pixelSize: root.fs(8)
                                        font.bold: true
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }

                        // 属性标签组
                        RowLayout {
                            spacing: root.dp(8)
                            Text {
                                text: "预估总耗时: " + ((liveModel && liveModel.cookerEstimatedCookingTime) ? liveModel.cookerEstimatedCookingTime : (root.getRecipeEstimatedTime(root.activeRecipe ? root.activeRecipe.mode : "") || (root.activeRecipe ? root.activeRecipe.estimatedTime : "")))
                                color: "#ffd666"
                                font.pixelSize: root.fs(10)
                                font.bold: true
                            }
                            Text {
                                text: "· 建议保压: " + ((liveModel && liveModel.cookerHoldingDurationText) ? liveModel.cookerHoldingDurationText : (root.activeRecipe ? root.activeRecipe.pressureTime : ""))
                                color: "#72d2ff"
                                font.pixelSize: root.fs(10)
                            }
                            Text {
                                visible: Boolean(liveModel && liveModel.cookerTaste)
                                text: "· 口感: " + (liveModel ? liveModel.cookerTaste : "")
                                color: "#52e379"
                                font.pixelSize: root.fs(10)
                            }
                        }
                    }

                    CloseButton {
                        Layout.preferredWidth: root.dp(36)
                        Layout.preferredHeight: root.dp(36)
                        iconSize: root.dp(16)
                        onClicked: root.showRecipeDetail = false
                    }
                }

                // 水平分割线
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                }

                // 可滚动做法详情内容
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Flickable {
                        id: detailFlickable
                        anchors.fill: parent
                        contentWidth: width
                        contentHeight: detailCol.implicitHeight + root.dp(16)
                        boundsBehavior: Flickable.StopAtBounds
                        flickableDirection: Flickable.VerticalFlick

                        ScrollBar.vertical: ScrollBar {
                            anchors.right: parent.right
                            anchors.rightMargin: -root.dp(2)
                            policy: ScrollBar.AsNeeded
                            width: root.dp(4)
                        }

                        ColumnLayout {
                            id: detailCol
                            width: parent.width - root.dp(10)
                            spacing: root.dp(10)

                            // 1. 菜品简介卡片
                            Rectangle {
                                Layout.fillWidth: true
                                radius: root.dp(8)
                                color: Qt.rgba(0.12, 0.18, 0.26, 0.85)
                                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                border.width: 1
                                implicitHeight: Math.max(root.dp(30), descText.implicitHeight + root.dp(16))

                                Text {
                                    id: descText
                                    x: root.dp(10)
                                    y: root.dp(8)
                                    width: Math.max(10, parent.width - root.dp(20))
                                    text: root.detailDescription
                                    color: "#c8dceb"
                                    font.pixelSize: root.fs(11)
                                    wrapMode: Text.WordWrap
                                    lineHeight: 1.2
                                }
                            }

                            // 2. 所需食材
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: root.dp(5)

                                RowLayout {
                                    spacing: root.dp(4)
                                    Text {
                                        text: qsTr("所需食材清单")
                                        color: "#ffffff"
                                        font.pixelSize: root.fs(12)
                                        font.bold: true
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        visible: root.detailIngredients.length > 0
                                        text: "共 " + root.detailIngredients.length + " 样"
                                        color: "#8cb2cc"
                                        font.pixelSize: root.fs(9)
                                    }
                                }

                                // 食材标签 Flow 流式布局
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: root.dp(6)
                                    visible: root.detailIngredients.length > 0

                                    Repeater {
                                        model: root.detailIngredients

                                        Rectangle {
                                            radius: root.dp(6)
                                            color: Qt.rgba(0.16, 0.23, 0.32, 0.90)
                                            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.16)
                                            border.width: 1
                                            implicitHeight: root.dp(24)
                                            implicitWidth: ingText.implicitWidth + root.dp(14)

                                            Text {
                                                id: ingText
                                                anchors.centerIn: parent
                                                text: modelData
                                                color: "#e4f1fb"
                                                font.pixelSize: root.fs(10)
                                            }
                                        }
                                    }
                                }

                                // 默认食材备选提示
                                Text {
                                    visible: root.detailIngredients.length === 0
                                    text: qsTr("根据食材分量切块，肉类焯水去腥，加入葱姜与适量调味料。")
                                    color: "#8cb2cc"
                                    font.pixelSize: root.fs(10)
                                }
                            }

                            // 3. 制作步骤
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: root.dp(6)

                                RowLayout {
                                    spacing: root.dp(4)
                                    Text {
                                        text: qsTr("烹饪步骤指引")
                                        color: "#ffffff"
                                        font.pixelSize: root.fs(12)
                                        font.bold: true
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: root.dp(5)
                                    visible: root.detailSteps.length > 0

                                    Repeater {
                                        model: root.detailSteps

                                        Rectangle {
                                            Layout.fillWidth: true
                                            radius: root.dp(6)
                                            color: Qt.rgba(0.13, 0.19, 0.27, 0.85)
                                            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                            border.width: 1
                                            implicitHeight: Math.max(root.dp(28), overlayStepTxt.implicitHeight + root.dp(12))

                                            Rectangle {
                                                id: overlayStepBadge
                                                x: root.dp(6)
                                                y: root.dp(6)
                                                width: root.dp(18)
                                                height: root.dp(18)
                                                radius: root.dp(9)
                                                color: Qt.rgba(0.20, 0.70, 0.40, 0.35)
                                                border.color: Qt.rgba(0.40, 0.85, 0.55, 0.75)
                                                border.width: 1

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: String(index + 1)
                                                    color: "#52e379"
                                                    font.pixelSize: root.fs(9)
                                                    font.bold: true
                                                }
                                            }

                                            Text {
                                                id: overlayStepTxt
                                                x: root.dp(30)
                                                y: root.dp(6)
                                                width: Math.max(10, parent.width - root.dp(38))
                                                text: modelData
                                                color: "#e4f1fb"
                                                font.pixelSize: root.fs(11)
                                                wrapMode: Text.WordWrap
                                                lineHeight: 1.15
                                            }
                                        }
                                    }
                                }

                                // 默认烹饪步骤
                                Text {
                                    visible: root.detailSteps.length === 0
                                    text: qsTr("1. 将处理好的食材与适量清水/料汁加入电压力锅内胆；\n2. 旋转合盖并确保手柄锁止；\n3. 选择当前菜谱程序，点击下方按钮开始烹饪；\n4. 烹饪完成自动泄压后开盖享用。")
                                    color: "#8cb2cc"
                                    font.pixelSize: root.fs(10)
                                    lineHeight: 1.2
                                }
                            }

                            // 4. 大厨贴士
                            Rectangle {
                                visible: root.detailTips !== ""
                                Layout.fillWidth: true
                                radius: root.dp(8)
                                color: Qt.rgba(0.24, 0.20, 0.10, 0.85)
                                border.color: Qt.rgba(1.0, 0.80, 0.20, 0.45)
                                border.width: 1
                                implicitHeight: overlayTipsTitleRow.implicitHeight + overlayTipsTxt.implicitHeight + root.dp(18)

                                RowLayout {
                                    id: overlayTipsTitleRow
                                    x: root.dp(8)
                                    y: root.dp(6)
                                    spacing: root.dp(4)
                                    Text {
                                        text: qsTr("大厨私房贴士")
                                        color: "#ffd666"
                                        font.pixelSize: root.fs(11)
                                        font.bold: true
                                    }
                                }

                                Text {
                                    id: overlayTipsTxt
                                    x: root.dp(8)
                                    y: overlayTipsTitleRow.y + overlayTipsTitleRow.implicitHeight + root.dp(4)
                                    width: Math.max(10, parent.width - root.dp(16))
                                    text: root.detailTips
                                    color: "#fff3d4"
                                    font.pixelSize: root.fs(10)
                                    wrapMode: Text.WordWrap
                                    lineHeight: 1.15
                                }
                            }
                        }
                    }
                }

                // 底部操作按钮栏
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.dp(36)
                    spacing: root.dp(8)

                    GlassButton {
                        Layout.preferredWidth: root.dp(114)
                        Layout.fillHeight: true
                        scaleUnit: root.scaleUnit
                        styleType: "neutral"
                        text: qsTr("返回菜谱列表")
                        textPixelSize: root.fs(11)
                        onClicked: root.showRecipeDetail = false
                    }

                    // 右侧动作按钮：根据运行状态展示为“正在烹饪中”或“开始烹饪”
                    GlassButton {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        scaleUnit: root.scaleUnit
                        readonly property bool isThisRunning: root.isCooking && root.activeRecipe && (root.runningModeText === root.activeRecipe.mode || root.runningModeText === root.activeRecipe.name)
                        readonly property bool isOtherRunning: root.isCooking && !isThisRunning
                        disabled: isThisRunning || isOtherRunning
                        styleType: isThisRunning ? "accent" : (isOtherRunning ? "neutral" : "primary")
                        text: {
                            if (isThisRunning) {
                                return qsTr("当前正在执行此菜谱烹饪程序 (%1)").arg(root.activeRecipe ? root.activeRecipe.name : "")
                            }
                            if (isOtherRunning) {
                                return qsTr("电压力锅工作中 · 当前正在运行: %1").arg(root.runningModeText !== "" ? root.runningModeText : root.statusText)
                            }
                            return qsTr("以此菜谱开始烹饪 (%1)").arg(root.activeRecipe ? root.activeRecipe.name : "")
                        }
                        textPixelSize: root.fs(12)
                        onClicked: {
                            if (root.activeRecipe) {
                                root.applyAndCookRecipe(root.activeRecipe)
                            }
                        }
                    }
                }
            }
        }
    }
}
