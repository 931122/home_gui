import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

Rectangle {
    id: cardRoot

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)

    signal fullscreenRequested()
    signal settingsRequested()

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    radius: cardRadius
    color: Qt.rgba(0.12, 0.18, 0.26, 0.52)
    border.color: Qt.rgba(1, 1, 1, 0.15)
    border.width: 1
    clip: true

    // 智能提取全屋所有 HA 实体
    readonly property var allHaActions: (typeof globalState !== "undefined" && globalState.haActionStates) ? globalState.haActionStates : []

    // 提取所有“灯光”相关实体
    readonly property var lightActions: {
        var res = []
        for (var i = 0; i < allHaActions.length; ++i) {
            var item = allHaActions[i]
            if (!item) continue
            var name = String(item.name || "")
            var eid = String(item.entityId || "")
            var domain = String(item.domain || "")
            var isLight = domain === "light" ||
                          name.indexOf("灯") !== -1 ||
                          name.indexOf("洗漱台") !== -1 ||
                          eid.indexOf("light.") !== -1
            if (isLight && name !== "全屋总控") {
                res.push(item)
            }
        }
        return res
    }

    // 统计开启的灯数量
    readonly property int activeLightCount: {
        var count = 0
        for (var i = 0; i < lightActions.length; ++i) {
            if (lightActions[i] && lightActions[i].active) {
                count++
            }
        }
        return count
    }

    // 开启中的灯名称简写拼接（例如“客厅灯 · 餐厅灯”）
    readonly property string activeLightNamesSummary: {
        var names = []
        for (var i = 0; i < lightActions.length; ++i) {
            if (lightActions[i] && lightActions[i].active) {
                names.push(lightActions[i].name || "")
            }
        }
        if (names.length === 0) return qsTr("全屋节能中 · 夜间熄灯")
        return names.slice(0, 3).join(" · ") + (names.length > 3 ? "..." : "")
    }

    // 浴霸实体：仅在 HA 明确存在相应实体时启用（绝不无中生有硬编码假设备）
    readonly property var bathHeaterEntity: {
        for (var i = 0; i < allHaActions.length; ++i) {
            var it = allHaActions[i]
            if (!it) continue
            var n = String(it.name || "")
            var e = String(it.entityId || "")
            if (n.indexOf("浴霸") !== -1 || n.indexOf("浴室暖") !== -1 || e.indexOf("bath_heater") !== -1 || e.indexOf("yuba") !== -1) {
                return it
            }
        }
        return null
    }

    // 空气净化器实体：仅在 HA 明确存在相应实体时启用（绝不无中生有硬编码假设备）
    readonly property var airPurifierEntity: {
        for (var j = 0; j < allHaActions.length; ++j) {
            var item = allHaActions[j]
            if (!item) continue
            var n2 = String(item.name || "")
            var e2 = String(item.entityId || "")
            if (n2.indexOf("净化器") !== -1 || n2.indexOf("新风") !== -1 || e2.indexOf("fan.air_purifier") !== -1 || e2.indexOf("purifier") !== -1) {
                return item
            }
        }
        return null
    }

    // 其他常用实体（非灯光、非浴霸、非空净、非总控，例如插座、厨电等）
    readonly property var otherActions: {
        var res = []
        for (var k = 0; k < allHaActions.length; ++k) {
            var dev = allHaActions[k]
            if (!dev) continue
            if (dev === bathHeaterEntity || dev === airPurifierEntity) continue
            var nameStr = String(dev.name || "")
            var eidStr = String(dev.entityId || "")
            var domStr = String(dev.domain || "")
            var isL = domStr === "light" || nameStr.indexOf("灯") !== -1 || nameStr.indexOf("洗漱台") !== -1 || eidStr.indexOf("light.") !== -1
            if (!isL && nameStr !== "全屋总控") {
                res.push(dev)
            }
        }
        return res
    }

    // 开启中的其他设备数量
    readonly property int activeOtherCount: {
        var c = 0
        for (var m = 0; m < otherActions.length; ++m) {
            if (otherActions[m] && otherActions[m].active) c++
        }
        return c
    }

    readonly property string activeOtherNamesSummary: {
        var names = []
        for (var n = 0; n < otherActions.length; ++n) {
            if (otherActions[n] && otherActions[n].active) {
                names.push(otherActions[n].name || "")
            }
        }
        if (names.length === 0) return qsTr("全屋设备待机中")
        return names.slice(0, 3).join(" · ") + (names.length > 3 ? "..." : "")
    }

    readonly property bool hasCameras: typeof appController !== "undefined" && appController && appController.cameraPreviewModels && appController.cameraPreviewModels.length > 0
    readonly property bool hasAnyDevices: allHaActions.length > 0 || hasCameras

    // 判断各个子区域是否需要渲染（严格根据真实配置决定，无配置时不虚构）
    readonly property bool showLights: lightActions.length > 0
    readonly property bool showBathHeater: bathHeaterEntity !== null
    readonly property bool showAirPurifier: airPurifierEntity !== null
    readonly property bool showOtherDevices: (!showBathHeater || !showAirPurifier) && (otherActions.length > 0)
    readonly property bool showCameras: (!showBathHeater && !showAirPurifier && otherActions.length === 0 && hasCameras && appController.cameraPreviewModels.length > 1)

    // 计算活跃的列数（1 ~ 3）
    readonly property int activeColumnCount: {
        var count = 0
        if (showLights) count++
        if (showBathHeater) count++
        if (showAirPurifier) count++
        if (showOtherDevices) count++
        if (showCameras) count++
        return Math.max(1, count)
    }

    // 动态列宽度比例计算
    readonly property real availableWidth: Math.max(1, cardRoot.width - cardRoot.dp(16) - Math.max(0, activeColumnCount - 1) * cardRoot.dp(8))

    readonly property real lightZoneWidth: {
        if (!showLights) return 0
        if (activeColumnCount === 1) return availableWidth
        if (activeColumnCount === 2) return Math.floor(availableWidth * 0.52)
        return Math.floor(availableWidth * 0.40)
    }

    readonly property real secondColWidth: {
        var remainingW = availableWidth - (showLights ? lightZoneWidth : 0)
        var remainingCols = activeColumnCount - (showLights ? 1 : 0)
        if (remainingCols <= 0) return 0
        if (remainingCols === 1) return remainingW
        return Math.floor(remainingW * 0.49)
    }

    readonly property real thirdColWidth: {
        var remainingW = availableWidth - (showLights ? lightZoneWidth : 0) - secondColWidth
        return Math.max(0, remainingW)
    }

    // =========================================================================
    // 0. 未加载配置 / 清除配置后的空状态占位提示（绝不虚构设备）
    // =========================================================================
    Item {
        anchors.fill: parent
        visible: !cardRoot.hasAnyDevices

        RowLayout {
            anchors.centerIn: parent
            spacing: cardRoot.dp(16)

            Rectangle {
                width: cardRoot.dp(38)
                height: cardRoot.dp(38)
                radius: cardRoot.dp(19)
                color: Qt.rgba(0.20, 0.55, 0.95, 0.15)
                border.color: Qt.rgba(0.40, 0.75, 1.0, 0.35)
                border.width: 1

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/icons/settings.svg"
                    width: cardRoot.dp(20)
                    height: cardRoot.dp(20)
                    sourceSize: Qt.size(width, height)
                    smooth: true
                }
            }

            ColumnLayout {
                spacing: cardRoot.dp(2)

                Text {
                    text: qsTr("未加载智能家居配置")
                    color: "#f0f6fa"
                    font.pixelSize: cardRoot.fs(13)
                    font.bold: true
                }

                Text {
                    text: qsTr("当前配置文件已清除或无生效实体，可在系统设置中选择 YAML 文件")
                    color: "#94a3b8"
                    font.pixelSize: cardRoot.fs(10)
                }
            }

            Item {
                width: cardRoot.dp(6)
                height: 1
            }

            GlassButton {
                scaleUnit: cardRoot.scaleUnit
                styleType: "accent"
                implicitWidth: cardRoot.dp(88)
                implicitHeight: cardRoot.dp(30)
                text: qsTr("打开设置")
                textPixelSize: cardRoot.fs(11)
                onClicked: cardRoot.settingsRequested()
            }
        }
    }

    // =========================================================================
    // 真实设备动态态势区（根据真实配置动态伸缩呈现）
    // =========================================================================
    Row {
        anchors.fill: parent
        anchors.margins: cardRoot.dp(8)
        spacing: cardRoot.dp(4)
        visible: cardRoot.hasAnyDevices

        // =====================================================================
        // 1. 全屋灯光态势与快捷控制区（真实存在灯光实体时渲染）
        // =====================================================================
        Item {
            id: lightZone
            visible: cardRoot.showLights
            width: cardRoot.lightZoneWidth
            height: parent.height

            Column {
                anchors.fill: parent
                anchors.leftMargin: cardRoot.dp(4)
                anchors.rightMargin: cardRoot.dp(6)
                spacing: cardRoot.dp(3)

                // 顶栏：圆形灯泡徽章 + 标题 + 状态小标签 + 一键全关按钮
                Item {
                    width: parent.width
                    height: cardRoot.dp(24)

                    Row {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: cardRoot.dp(6)

                        // 呼吸发光灯球
                        Rectangle {
                            width: cardRoot.dp(20)
                            height: cardRoot.dp(20)
                            radius: cardRoot.dp(10)
                            anchors.verticalCenter: parent.verticalCenter
                            color: activeLightCount > 0 ? Qt.rgba(1.0, 0.78, 0.16, 0.25) : Qt.rgba(1, 1, 1, 0.08)
                            border.color: activeLightCount > 0 ? Qt.rgba(1.0, 0.85, 0.30, 0.60) : "transparent"

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/icons/light.svg"
                                width: cardRoot.dp(12)
                                height: cardRoot.dp(12)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                        }

                        Text {
                            text: qsTr("全屋灯光")
                            color: "#ffffff"
                            font.pixelSize: cardRoot.fs(12)
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        // 状态胶囊
                        Rectangle {
                            height: cardRoot.dp(17)
                            width: lightBadgeText.implicitWidth + cardRoot.dp(10)
                            radius: cardRoot.dp(8)
                            color: activeLightCount > 0 ? Qt.rgba(1.0, 0.78, 0.15, 0.20) : Qt.rgba(1, 1, 1, 0.08)
                            border.color: activeLightCount > 0 ? Qt.rgba(1.0, 0.85, 0.30, 0.40) : "transparent"
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                id: lightBadgeText
                                anchors.centerIn: parent
                                text: activeLightCount > 0 ? qsTr("%1 开启").arg(activeLightCount) : qsTr("全部熄灭")
                                font.pixelSize: cardRoot.fs(9)
                                font.bold: true
                                color: activeLightCount > 0 ? "#ffd644" : "#8ea3b5"
                            }
                        }
                    }

                    // 一键全关 / 开启常用按钮
                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: cardRoot.dp(21)
                        width: lightMasterBtnText.implicitWidth + cardRoot.dp(14)
                        radius: cardRoot.dp(10)
                        color: activeLightCount > 0
                               ? (lightMasterMouse.pressed ? Qt.rgba(0.9, 0.28, 0.28, 0.85) : Qt.rgba(0.50, 0.16, 0.16, 0.75))
                               : (lightMasterMouse.pressed ? Qt.rgba(0.2, 0.55, 0.85, 0.85) : Qt.rgba(0.12, 0.24, 0.36, 0.75))
                        border.color: activeLightCount > 0 ? Qt.rgba(1.0, 0.45, 0.45, 0.50) : Qt.rgba(0.40, 0.75, 1.0, 0.40)

                        Text {
                            id: lightMasterBtnText
                            anchors.centerIn: parent
                            text: activeLightCount > 0 ? qsTr("一键熄灯") : qsTr("开启常用")
                            font.pixelSize: cardRoot.fs(9)
                            font.bold: true
                            color: activeLightCount > 0 ? "#ffa8a8" : "#88cdff"
                        }

                        MouseArea {
                            id: lightMasterMouse
                            anchors.fill: parent
                            onClicked: {
                                if (activeLightCount > 0) {
                                    for (var i = 0; i < lightActions.length; ++i) {
                                        if (lightActions[i] && lightActions[i].active) {
                                            appController.callHaActionService(lightActions[i].name, "turn_off")
                                        }
                                    }
                                } else {
                                    for (var j = 0; j < Math.min(2, lightActions.length); ++j) {
                                        if (lightActions[j]) {
                                            appController.triggerHaAction(lightActions[j].name)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 中间：当前开灯概要副文
                Text {
                    width: parent.width
                    text: activeLightNamesSummary
                    font.pixelSize: cardRoot.fs(10)
                    color: activeLightCount > 0 ? "#ffe073" : "#6c8295"
                    elide: Text.ElideRight
                }

                // 底部：主要灯光快捷开关胶囊
                Row {
                    width: parent.width
                    spacing: cardRoot.dp(5)

                    Repeater {
                        model: lightActions.slice(0, cardRoot.activeColumnCount === 1 ? 8 : 5)

                        Rectangle {
                            readonly property int totalChips: Math.min(lightActions.length, cardRoot.activeColumnCount === 1 ? 8 : 5)
                            height: cardRoot.dp(22)
                            width: Math.min((parent.width - (totalChips - 1) * cardRoot.dp(5)) / Math.max(1, totalChips), chipText.implicitWidth + cardRoot.dp(16))
                            radius: cardRoot.dp(11)
                            color: modelData.active ? Qt.rgba(1.0, 0.80, 0.20, 0.28) : Qt.rgba(1, 1, 1, 0.07)
                            border.color: modelData.active ? Qt.rgba(1.0, 0.85, 0.35, 0.60) : Qt.rgba(1, 1, 1, 0.08)

                            Row {
                                anchors.centerIn: parent
                                spacing: cardRoot.dp(4)

                                Rectangle {
                                    width: cardRoot.dp(5)
                                    height: cardRoot.dp(5)
                                    radius: cardRoot.dp(2.5)
                                    color: modelData.active ? "#ffcf33" : "#526573"
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    id: chipText
                                    text: modelData.name || modelData.entityId || ""
                                    color: modelData.active ? "#ffffff" : "#8ea3b5"
                                    font.pixelSize: cardRoot.fs(9)
                                    font.bold: modelData.active
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: appController.triggerHaAction(modelData.name)
                            }
                        }
                    }
                }
            }
        }

        // 垂直微光分割线 1
        Rectangle {
            visible: cardRoot.showLights && cardRoot.activeColumnCount > 1
            width: 1
            height: parent.height - cardRoot.dp(10)
            anchors.verticalCenter: parent.verticalCenter
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.15) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        // =====================================================================
        // 2. 智能浴霸 / 浴室温控区（仅在 HA 存在浴霸实体时渲染）
        // =====================================================================
        Item {
            id: bathHeaterZone
            visible: cardRoot.showBathHeater
            width: cardRoot.secondColWidth
            height: parent.height

            Column {
                anchors.fill: parent
                anchors.leftMargin: cardRoot.dp(6)
                anchors.rightMargin: cardRoot.dp(6)
                spacing: cardRoot.dp(3)

                Item {
                    width: parent.width
                    height: cardRoot.dp(24)

                    Row {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: cardRoot.dp(5)

                        Rectangle {
                            width: cardRoot.dp(20)
                            height: cardRoot.dp(20)
                            radius: cardRoot.dp(10)
                            anchors.verticalCenter: parent.verticalCenter
                            color: (bathHeaterEntity && bathHeaterEntity.active)
                                   ? Qt.rgba(1.0, 0.50, 0.15, 0.25)
                                   : Qt.rgba(1, 1, 1, 0.08)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/icons/heater.svg"
                                width: cardRoot.dp(12)
                                height: cardRoot.dp(12)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                        }

                        Text {
                            text: bathHeaterEntity ? (bathHeaterEntity.name || qsTr("智能浴霸")) : ""
                            color: "#ffffff"
                            font.pixelSize: cardRoot.fs(12)
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: cardRoot.dp(16)
                        width: heaterStatusBadge.implicitWidth + cardRoot.dp(8)
                        radius: cardRoot.dp(8)
                        color: (bathHeaterEntity && bathHeaterEntity.active) ? Qt.rgba(1.0, 0.55, 0.20, 0.20) : Qt.rgba(1, 1, 1, 0.06)

                        Text {
                            id: heaterStatusBadge
                            anchors.centerIn: parent
                            text: (bathHeaterEntity && bathHeaterEntity.active) ? qsTr("加热中") : qsTr("待机")
                            font.pixelSize: cardRoot.fs(8)
                            font.bold: true
                            color: (bathHeaterEntity && bathHeaterEntity.active) ? "#ff9a44" : "#7c92a5"
                        }
                    }
                }

                Text {
                    width: parent.width
                    text: (bathHeaterEntity && bathHeaterEntity.active)
                          ? (bathHeaterEntity.stateText || qsTr("🔥 温暖运行中"))
                          : qsTr("待机休眠")
                    font.pixelSize: cardRoot.fs(10)
                    color: (bathHeaterEntity && bathHeaterEntity.active) ? "#ffaa5e" : "#6c8295"
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: parent.width
                    height: cardRoot.dp(22)
                    radius: cardRoot.dp(11)
                    color: Qt.rgba(0.04, 0.08, 0.12, 0.75)
                    border.color: Qt.rgba(1, 1, 1, 0.10)

                    Row {
                        anchors.fill: parent
                        anchors.margins: 1

                        Repeater {
                            model: [
                                { mode: "heat", label: qsTr("加热") },
                                { mode: "standby", label: qsTr("关闭") }
                            ]

                            Rectangle {
                                width: Math.floor(parent.width / 2)
                                height: parent.height
                                radius: cardRoot.dp(10)
                                readonly property bool isSelected: modelData.mode === "heat" ? (bathHeaterEntity && bathHeaterEntity.active) : (bathHeaterEntity && !bathHeaterEntity.active)
                                color: isSelected
                                       ? (modelData.mode === "heat" ? "#e05818" : Qt.rgba(1, 1, 1, 0.20))
                                       : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    font.pixelSize: cardRoot.fs(9)
                                    font.bold: parent.isSelected
                                    color: parent.isSelected ? "#ffffff" : "#8ea2b4"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (!bathHeaterEntity) return
                                        if (modelData.mode === "standby") {
                                            appController.callHaActionService(bathHeaterEntity.name, "turn_off")
                                        } else {
                                            appController.triggerHaAction(bathHeaterEntity.name)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // =====================================================================
        // 2B. 快捷生活设备区（无浴霸时呈现插座/厨电等常用设备）
        // =====================================================================
        Item {
            id: otherDevicesZone
            visible: cardRoot.showOtherDevices
            width: cardRoot.secondColWidth
            height: parent.height

            Column {
                anchors.fill: parent
                anchors.leftMargin: cardRoot.dp(6)
                anchors.rightMargin: cardRoot.dp(6)
                spacing: cardRoot.dp(3)

                Item {
                    width: parent.width
                    height: cardRoot.dp(24)

                    Row {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: cardRoot.dp(5)

                        Rectangle {
                            width: cardRoot.dp(20)
                            height: cardRoot.dp(20)
                            radius: cardRoot.dp(10)
                            anchors.verticalCenter: parent.verticalCenter
                            color: activeOtherCount > 0 ? Qt.rgba(0.20, 0.70, 1.0, 0.25) : Qt.rgba(1, 1, 1, 0.08)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/icons/switch.svg"
                                width: cardRoot.dp(12)
                                height: cardRoot.dp(12)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                        }

                        Text {
                            text: qsTr("生活设备")
                            color: "#ffffff"
                            font.pixelSize: cardRoot.fs(12)
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                            height: cardRoot.dp(16)
                            width: otherBadgeText.implicitWidth + cardRoot.dp(8)
                            radius: cardRoot.dp(8)
                            color: activeOtherCount > 0 ? Qt.rgba(0.20, 0.70, 1.0, 0.20) : Qt.rgba(1, 1, 1, 0.06)
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                id: otherBadgeText
                                anchors.centerIn: parent
                                text: activeOtherCount > 0 ? qsTr("%1 运行").arg(activeOtherCount) : qsTr("待机")
                                font.pixelSize: cardRoot.fs(8)
                                font.bold: true
                                color: activeOtherCount > 0 ? "#50cbff" : "#7c92a5"
                            }
                        }
                    }

                    // 全部关闭快捷按钮
                    Rectangle {
                        visible: activeOtherCount > 0
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: cardRoot.dp(18)
                        width: otherMasterText.implicitWidth + cardRoot.dp(10)
                        radius: cardRoot.dp(9)
                        color: Qt.rgba(0.40, 0.15, 0.15, 0.70)
                        border.color: Qt.rgba(1.0, 0.40, 0.40, 0.40)

                        Text {
                            id: otherMasterText
                            anchors.centerIn: parent
                            text: qsTr("全关")
                            font.pixelSize: cardRoot.fs(8)
                            font.bold: true
                            color: "#ffa8a8"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                for (var idx = 0; idx < otherActions.length; ++idx) {
                                    if (otherActions[idx] && otherActions[idx].active) {
                                        appController.callHaActionService(otherActions[idx].name, "turn_off")
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    width: parent.width
                    text: activeOtherNamesSummary
                    font.pixelSize: cardRoot.fs(10)
                    color: activeOtherCount > 0 ? "#62d6ff" : "#6c8295"
                    elide: Text.ElideRight
                }

                Row {
                    width: parent.width
                    spacing: cardRoot.dp(5)

                    Repeater {
                        model: otherActions.slice(0, 3)

                        Rectangle {
                            readonly property int totalChips: Math.min(otherActions.length, 3)
                            height: cardRoot.dp(22)
                            width: Math.min((parent.width - (totalChips - 1) * cardRoot.dp(5)) / Math.max(1, totalChips), chipTextOther.implicitWidth + cardRoot.dp(16))
                            radius: cardRoot.dp(11)
                            color: modelData.active ? Qt.rgba(0.20, 0.70, 1.0, 0.28) : Qt.rgba(1, 1, 1, 0.07)
                            border.color: modelData.active ? Qt.rgba(0.40, 0.80, 1.0, 0.60) : Qt.rgba(1, 1, 1, 0.08)

                            Row {
                                anchors.centerIn: parent
                                spacing: cardRoot.dp(4)

                                Rectangle {
                                    width: cardRoot.dp(5)
                                    height: cardRoot.dp(5)
                                    radius: cardRoot.dp(2.5)
                                    color: modelData.active ? "#38bdf8" : "#526573"
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    id: chipTextOther
                                    text: modelData.name || modelData.entityId || ""
                                    color: modelData.active ? "#ffffff" : "#8ea3b5"
                                    font.pixelSize: cardRoot.fs(9)
                                    font.bold: modelData.active
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: appController.triggerHaAction(modelData.name)
                            }
                        }
                    }
                }
            }
        }

        // 垂直微光分割线 2
        Rectangle {
            visible: (cardRoot.showBathHeater || cardRoot.showOtherDevices) && (cardRoot.showAirPurifier || cardRoot.showCameras)
            width: 1
            height: parent.height - cardRoot.dp(10)
            anchors.verticalCenter: parent.verticalCenter
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.15) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        // =====================================================================
        // 3. 室内空气与新风净化区（仅在 HA 存在空净实体时渲染）
        // =====================================================================
        Item {
            id: purifierZone
            visible: cardRoot.showAirPurifier
            width: cardRoot.thirdColWidth
            height: parent.height

            Column {
                anchors.fill: parent
                anchors.leftMargin: cardRoot.dp(6)
                anchors.rightMargin: cardRoot.dp(4)
                spacing: cardRoot.dp(3)

                Item {
                    width: parent.width
                    height: cardRoot.dp(24)

                    Row {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: cardRoot.dp(5)

                        Rectangle {
                            width: cardRoot.dp(20)
                            height: cardRoot.dp(20)
                            radius: cardRoot.dp(10)
                            anchors.verticalCenter: parent.verticalCenter
                            color: (airPurifierEntity && airPurifierEntity.active) ? Qt.rgba(0.20, 0.85, 0.45, 0.25) : Qt.rgba(1, 1, 1, 0.08)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/icons/purifier.svg"
                                width: cardRoot.dp(12)
                                height: cardRoot.dp(12)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                        }

                        Text {
                            text: airPurifierEntity ? (airPurifierEntity.name || qsTr("空气净化器")) : ""
                            color: "#ffffff"
                            font.pixelSize: cardRoot.fs(12)
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: cardRoot.dp(16)
                        width: purifierStatusBadge.implicitWidth + cardRoot.dp(8)
                        radius: cardRoot.dp(8)
                        color: (airPurifierEntity && airPurifierEntity.active) ? Qt.rgba(0.20, 0.85, 0.45, 0.20) : Qt.rgba(1, 1, 1, 0.06)

                        Text {
                            id: purifierStatusBadge
                            anchors.centerIn: parent
                            text: (airPurifierEntity && airPurifierEntity.active) ? qsTr("运行中") : qsTr("待机")
                            font.pixelSize: cardRoot.fs(8)
                            font.bold: true
                            color: (airPurifierEntity && airPurifierEntity.active) ? "#4ade80" : "#7c92a5"
                        }
                    }
                }

                Text {
                    width: parent.width
                    text: (airPurifierEntity && airPurifierEntity.active)
                          ? (airPurifierEntity.stateText || qsTr("🍃 智能净味中"))
                          : qsTr("待机休眠")
                    font.pixelSize: cardRoot.fs(10)
                    color: (airPurifierEntity && airPurifierEntity.active) ? "#54f29e" : "#6c8295"
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: parent.width
                    height: cardRoot.dp(22)
                    radius: cardRoot.dp(11)
                    color: Qt.rgba(0.04, 0.08, 0.12, 0.75)
                    border.color: Qt.rgba(1, 1, 1, 0.10)

                    Row {
                        anchors.fill: parent
                        anchors.margins: 1

                        Repeater {
                            model: [
                                { mode: "on", label: qsTr("开启") },
                                { mode: "off", label: qsTr("关闭") }
                            ]

                            Rectangle {
                                width: Math.floor(parent.width / 2)
                                height: parent.height
                                radius: cardRoot.dp(10)
                                readonly property bool isSelected: modelData.mode === "on" ? (airPurifierEntity && airPurifierEntity.active) : (airPurifierEntity && !airPurifierEntity.active)
                                color: isSelected
                                       ? (modelData.mode === "on" ? "#1ca756" : Qt.rgba(1, 1, 1, 0.20))
                                       : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    font.pixelSize: cardRoot.fs(9)
                                    font.bold: parent.isSelected
                                    color: parent.isSelected ? "#ffffff" : "#8ea2b4"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (!airPurifierEntity) return
                                        if (modelData.mode === "off") {
                                            appController.callHaActionService(airPurifierEntity.name, "turn_off")
                                        } else {
                                            appController.triggerHaAction(airPurifierEntity.name)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // =====================================================================
        // 3B. 监控通道快捷切换（空出列且存在多摄像头时渲染）
        // =====================================================================
        Item {
            id: cameraZone
            visible: cardRoot.showCameras
            width: cardRoot.thirdColWidth
            height: parent.height

            Column {
                anchors.fill: parent
                anchors.leftMargin: cardRoot.dp(6)
                anchors.rightMargin: cardRoot.dp(4)
                spacing: cardRoot.dp(3)

                Item {
                    width: parent.width
                    height: cardRoot.dp(24)

                    Row {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: cardRoot.dp(5)

                        Rectangle {
                            width: cardRoot.dp(20)
                            height: cardRoot.dp(20)
                            radius: cardRoot.dp(10)
                            anchors.verticalCenter: parent.verticalCenter
                            color: Qt.rgba(0.20, 0.55, 0.95, 0.25)

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/icons/camera.svg"
                                width: cardRoot.dp(12)
                                height: cardRoot.dp(12)
                                sourceSize: Qt.size(width, height)
                                smooth: true
                            }
                        }

                        Text {
                            text: qsTr("监控通道")
                            color: "#ffffff"
                            font.pixelSize: cardRoot.fs(12)
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                Text {
                    width: parent.width
                    text: qsTr("共 %1 路监控在线").arg(appController.cameraPreviewModels.length)
                    font.pixelSize: cardRoot.fs(10)
                    color: "#62d6ff"
                    elide: Text.ElideRight
                }

                Row {
                    width: parent.width
                    spacing: cardRoot.dp(5)

                    Repeater {
                        model: appController.cameraPreviewModels.slice(0, 3)

                        Rectangle {
                            readonly property bool isCurrent: appController.currentCameraIndex === (modelData.index !== undefined ? modelData.index : index)
                            height: cardRoot.dp(22)
                            width: Math.min((parent.width - (Math.min(appController.cameraPreviewModels.length, 3) - 1) * cardRoot.dp(5)) / Math.min(appController.cameraPreviewModels.length, 3), camText.implicitWidth + cardRoot.dp(16))
                            radius: cardRoot.dp(11)
                            color: isCurrent ? Qt.rgba(0.20, 0.60, 1.0, 0.35) : Qt.rgba(1, 1, 1, 0.07)
                            border.color: isCurrent ? Qt.rgba(0.40, 0.80, 1.0, 0.80) : Qt.rgba(1, 1, 1, 0.08)

                            Text {
                                id: camText
                                anchors.centerIn: parent
                                text: modelData.cameraName || qsTr("通道 %1").arg(index + 1)
                                color: parent.isCurrent ? "#ffffff" : "#8ea3b5"
                                font.pixelSize: cardRoot.fs(9)
                                font.bold: parent.isCurrent
                                elide: Text.ElideRight
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: appController.selectCamera(modelData.index !== undefined ? modelData.index : index)
                            }
                        }
                    }
                }
            }
        }
    }
}
