import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import HomeGui 1.0

Rectangle {
    id: cardRoot

    property real scaleUnit: 1.0
    property int panelRadius: dp(18)
    property int cardRadius: dp(14)

    signal fullscreenRequested()

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    radius: cardRadius
    color: Qt.rgba(0.12, 0.18, 0.26, 0.52)
    border.color: Qt.rgba(1, 1, 1, 0.15)
    border.width: 1
    clip: true

    // 顶部微晶流光折射高光线
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: cardRoot.cardRadius
        anchors.rightMargin: cardRoot.cardRadius
        height: 1
        color: "#ffffff"
        opacity: 0.30
    }

    // 智能提取全屋所有“灯光”相关实体
    readonly property var allHaActions: globalState.haActionStates || []

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

    // 浴霸状态：优先从 HA 查找，找不到时提供本地交互模拟状态
    property string bathHeaterMode: "standby" // "heat" (取暖), "vent" (换气), "standby" (关)
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

    // 空气净化器状态：优先从 HA 查找，找不到时提供本地交互模拟状态
    property string airPurifierMode: "auto" // "auto" (智能), "boost" (强劲), "off" (关)
    property int pm25Value: 12 // 默认优级空气指标
    readonly property var airPurifierEntity: {
        for (var i = 0; i < allHaActions.length; ++i) {
            var it = allHaActions[i]
            if (!it) continue
            var n = String(it.name || "")
            var e = String(it.entityId || "")
            if (n.indexOf("净化器") !== -1 || n.indexOf("新风") !== -1 || e.indexOf("fan.air_purifier") !== -1 || e.indexOf("purifier") !== -1) {
                return it
            }
        }
        return null
    }

    // =========================================================================
    // 三大核心全屋态势区（一体化微晶中控坞布局，消除粗暴的多层重叠边框）
    // =========================================================================
    Row {
        anchors.fill: parent
        anchors.margins: cardRoot.dp(8)
        spacing: 0

        // =====================================================================
        // 1. 全屋灯光态势与快捷控制区 (占 42% 宽度)
        // =====================================================================
        Item {
            id: lightZone
            width: Math.floor(parent.width * 0.42)
            height: parent.height

            Column {
                anchors.fill: parent
                anchors.leftMargin: cardRoot.dp(4)
                anchors.rightMargin: cardRoot.dp(10)
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
                                            appController.triggerHaAction(lightActions[i].name)
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

                // 底部：主要灯光快捷开关胶囊（单排紧凑，不换行，绝不溢出）
                Row {
                    width: parent.width
                    spacing: cardRoot.dp(5)

                    Repeater {
                        model: lightActions.slice(0, 5)

                        Rectangle {
                            height: cardRoot.dp(22)
                            width: Math.min((parent.width - (Math.min(lightActions.length, 5) - 1) * cardRoot.dp(5)) / Math.max(1, Math.min(lightActions.length, 5)), chipText.implicitWidth + cardRoot.dp(16))
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
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: modelData.active ? "#ffe14d" : "#55697d"
                                }

                                Text {
                                    id: chipText
                                    text: modelData.name || ""
                                    font.pixelSize: cardRoot.fs(9)
                                    font.bold: modelData.active
                                    color: modelData.active ? "#ffffff" : "#98abbd"
                                    anchors.verticalCenter: parent.verticalCenter
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (modelData.name) {
                                        appController.triggerHaAction(modelData.name)
                                    }
                                }
                            }
                        }
                    }

                    // 没有实体时的备用占位提示
                    Text {
                        visible: lightActions.length === 0
                        text: qsTr("客厅灯 · 餐厅灯 · 卧室灯")
                        color: "#556a7d"
                        font.pixelSize: cardRoot.fs(9)
                    }
                }
            }
        }

        // 垂直微光分割线 1
        Rectangle {
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
        // 2. 智能浴霸 / 浴室温控区 (占 29% 宽度)
        // =====================================================================
        Item {
            id: bathHeaterZone
            width: Math.floor(parent.width * 0.29)
            height: parent.height

            Column {
                anchors.fill: parent
                anchors.leftMargin: cardRoot.dp(10)
                anchors.rightMargin: cardRoot.dp(10)
                spacing: cardRoot.dp(3)

                // 顶栏：图标 + 名称 + 运行呼吸点
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
                            color: bathHeaterMode === "heat"
                                   ? Qt.rgba(1.0, 0.50, 0.15, 0.25)
                                   : (bathHeaterMode === "vent" ? Qt.rgba(0.20, 0.75, 1.0, 0.25) : Qt.rgba(1, 1, 1, 0.08))

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
                            text: bathHeaterEntity ? (bathHeaterEntity.name || qsTr("智能浴霸")) : qsTr("浴室暖风")
                            color: "#ffffff"
                            font.pixelSize: cardRoot.fs(12)
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    // 运行状态指示徽章
                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: cardRoot.dp(16)
                        width: heaterStatusBadge.implicitWidth + cardRoot.dp(8)
                        radius: cardRoot.dp(8)
                        color: bathHeaterMode === "heat" ? Qt.rgba(1.0, 0.55, 0.20, 0.20) : (bathHeaterMode === "vent" ? Qt.rgba(0.20, 0.75, 1.0, 0.20) : Qt.rgba(1, 1, 1, 0.06))

                        Text {
                            id: heaterStatusBadge
                            anchors.centerIn: parent
                            text: bathHeaterMode === "heat" ? qsTr("加热") : (bathHeaterMode === "vent" ? qsTr("换气") : qsTr("待机"))
                            font.pixelSize: cardRoot.fs(8)
                            font.bold: true
                            color: bathHeaterMode === "heat" ? "#ff9a44" : (bathHeaterMode === "vent" ? "#50cbff" : "#7c92a5")
                        }
                    }
                }

                // 中间：当前温控副文本
                Text {
                    width: parent.width
                    text: bathHeaterMode === "heat" ? qsTr("🔥 强暖速热 · 32°C") : (bathHeaterMode === "vent" ? qsTr("🌀 双向排气 · 极速") : qsTr("待机休眠 · 24°C"))
                    font.pixelSize: cardRoot.fs(10)
                    color: bathHeaterMode === "heat" ? "#ffaa5e" : (bathHeaterMode === "vent" ? "#62d6ff" : "#6c8295")
                    elide: Text.ElideRight
                }

                // 底部：现代化一体式分段控制器 [ 取暖 | 换气 | 关 ]
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
                                { mode: "heat", label: qsTr("取暖") },
                                { mode: "vent", label: qsTr("换气") },
                                { mode: "standby", label: qsTr("关") }
                            ]

                            Rectangle {
                                width: Math.floor(parent.width / 3)
                                height: parent.height
                                radius: cardRoot.dp(10)
                                color: bathHeaterMode === modelData.mode
                                       ? (modelData.mode === "heat" ? "#e05818" : (modelData.mode === "vent" ? "#0088cc" : Qt.rgba(1, 1, 1, 0.20)))
                                       : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    font.pixelSize: cardRoot.fs(9)
                                    font.bold: bathHeaterMode === modelData.mode
                                    color: bathHeaterMode === modelData.mode ? "#ffffff" : "#8ea2b4"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        cardRoot.bathHeaterMode = modelData.mode
                                        if (cardRoot.bathHeaterEntity) {
                                            appController.triggerHaAction(cardRoot.bathHeaterEntity.name)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 垂直微光分割线 2
        Rectangle {
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
        // 3. 室内空气与新风净化区 (占 29% 宽度)
        // =====================================================================
        Item {
            id: purifierZone
            width: parent.width - lightZone.width - bathHeaterZone.width - 2
            height: parent.height

            Column {
                anchors.fill: parent
                anchors.leftMargin: cardRoot.dp(10)
                anchors.rightMargin: cardRoot.dp(4)
                spacing: cardRoot.dp(3)

                // 顶栏：图标 + 名称 + PM2.5 晶莹胶囊
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
                            color: airPurifierMode !== "off" ? Qt.rgba(0.20, 0.85, 0.45, 0.25) : Qt.rgba(1, 1, 1, 0.08)

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
                            text: airPurifierEntity ? (airPurifierEntity.name || qsTr("空气净化器")) : qsTr("空气净化")
                            color: "#ffffff"
                            font.pixelSize: cardRoot.fs(12)
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    // PM2.5 晶莹胶囊
                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: cardRoot.dp(17)
                        width: pm25Text.implicitWidth + cardRoot.dp(10)
                        radius: cardRoot.dp(8)
                        color: Qt.rgba(0.20, 0.85, 0.45, 0.22)
                        border.color: Qt.rgba(0.35, 0.95, 0.55, 0.50)

                        Text {
                            id: pm25Text
                            anchors.centerIn: parent
                            text: qsTr("PM2.5 %1 优").arg(cardRoot.pm25Value)
                            font.pixelSize: cardRoot.fs(8)
                            font.bold: true
                            color: "#46ff94"
                        }
                    }
                }

                // 中间：运行状态副文本
                Text {
                    width: parent.width
                    text: airPurifierMode === "auto"
                          ? qsTr("🍃 智能净味 · 运行中")
                          : (airPurifierMode === "boost" ? qsTr("💨 极速净化 · 强劲") : qsTr("待机休眠 · 滤网良好"))
                    font.pixelSize: cardRoot.fs(10)
                    color: airPurifierMode !== "off" ? "#54f29e" : "#6c8295"
                    elide: Text.ElideRight
                }

                // 底部：一体式分段控制器 [ 智能 | 强劲 | 关 ]
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
                                { mode: "auto", label: qsTr("智能") },
                                { mode: "boost", label: qsTr("强劲") },
                                { mode: "off", label: qsTr("关") }
                            ]

                            Rectangle {
                                width: Math.floor(parent.width / 3)
                                height: parent.height
                                radius: cardRoot.dp(10)
                                color: airPurifierMode === modelData.mode
                                       ? (modelData.mode !== "off" ? "#1ca756" : Qt.rgba(1, 1, 1, 0.20))
                                       : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    font.pixelSize: cardRoot.fs(9)
                                    font.bold: airPurifierMode === modelData.mode
                                    color: airPurifierMode === modelData.mode ? "#ffffff" : "#8ea2b4"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        cardRoot.airPurifierMode = modelData.mode
                                        if (cardRoot.airPurifierEntity) {
                                            appController.triggerHaAction(cardRoot.airPurifierEntity.name)
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
}
