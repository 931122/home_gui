import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

Popup {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(22)
    property int cardRadius: dp(16)
    property int chipRadius: dp(12)

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height
    modal: true
    focus: true
    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.96, dp(isPortrait ? 460 : 720))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.94, dp(isPortrait ? 660 : 430))
    anchors.centerIn: Overlay.overlay
    padding: 0
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

    // 苹果柔和暗夜微光遮罩（透出底层内容轮廓）
    Overlay.modal: Rectangle {
        color: Theme.colorOverlayModal
    }

    // ================= 苹果 iOS 液态毛玻璃背景（Liquid Frosted Shell） =================
    background: Rectangle {
        radius: root.panelRadius
        color: Qt.rgba(0.07, 0.12, 0.18, 0.85)
        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.16)
        border.width: 1
        clip: true

        // 1. 左上方环境蓝光漫反射（模拟磨砂玻璃次表面漫散射光）
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: -root.dp(40)
            anchors.topMargin: -root.dp(40)
            width: root.dp(360)
            height: root.dp(240)
            radius: width / 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.28, 0.62, 0.96, 0.14) }
                GradientStop { position: 0.6; color: Qt.rgba(0.12, 0.35, 0.65, 0.04) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        // 2. 右下方微紫暗光漫反射（苹果经典双光源光影纵深）
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: -root.dp(50)
            anchors.bottomMargin: -root.dp(50)
            width: root.dp(320)
            height: root.dp(200)
            radius: width / 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.42, 0.28, 0.65, 0.08) }
                GradientStop { position: 0.7; color: Qt.rgba(0.20, 0.15, 0.40, 0.02) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        // 3. 顶部月白玻璃边缘微折射高光（Specular Rim）
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
    }

    // 实况快捷数据源
    readonly property var cur: appController.weatherCurrent || ({})
    readonly property string curTemp: (cur.temp && cur.temp !== "") ? cur.temp : "--"
    readonly property string curWeather: cur.weather || "多云"
    readonly property string curEmoji: cur.emoji || ""
    readonly property string curHumidity: cur.humidity || "--"
    readonly property string curWindDir: cur.windDirection || "微风"
    readonly property string curWindPower: cur.windPower || ""
    readonly property string curAqi: cur.aqi || ""
    readonly property string curAqiText: cur.aqiText || (cur.aqi ? "良好" : "优")
    readonly property string curAqiColor: cur.aqiColor || "#38bdf8"
    readonly property string curLifeHint: cur.lifeHint || ""
    readonly property string curLifeTitle: cur.lifeTitle || "舒适度"
    readonly property string curReportTime: cur.reportTime || ""
    readonly property string curLocation: (appController.weatherLocation && appController.weatherLocation !== "") 
                                          ? appController.weatherLocation : qsTr("北京 · 朝阳")

    // 预报数据响应式属性绑定（强监听 appController 变化）
    readonly property var forecastList: appController.weatherForecast

    // 备用兜底数据
    readonly property var fallbackForecast: [
        { label: "今天", week: "今天", emoji: "", icon: "qrc:/icons/weather-cloudy.svg", weather: "多云", lowTemp: "20", highTemp: "31", tempRange: "20~31°C", dayWind: "北风 <3级" },
        { label: "明天", week: "明天", emoji: "", icon: "qrc:/icons/weather-cloudy.svg", weather: "多云", lowTemp: "21", highTemp: "32", tempRange: "21~32°C", dayWind: "南风 <3级" },
        { label: "后天", week: "后天", emoji: "", icon: "qrc:/icons/weather-cloudy.svg", weather: "多云", lowTemp: "21", highTemp: "31", tempRange: "21~31°C", dayWind: "北风 <3级" }
    ]

    function getWeatherIcon(weatherText) {
        return "qrc:/icons/weather-cloudy.svg"
    }

    function getLowTemp(modelData) {
        if (!modelData) return "--"
        if (modelData.lowTemp !== undefined && modelData.lowTemp !== "" && modelData.lowTemp !== "--") {
            var val = String(modelData.lowTemp).replace(/[\u2103°C\s]/g, "").trim()
            return val ? (val + "°C") : "--"
        }
        if (modelData.tempRange) {
            var parts = String(modelData.tempRange).replace(/[\u2103°C\s]/g, "").split("~")
            if (parts.length >= 1 && parts[0].trim() !== "") {
                return parts[0].trim() + "°C"
            }
        }
        return "--"
    }

    function getHighTemp(modelData) {
        if (!modelData) return "--"
        if (modelData.highTemp !== undefined && modelData.highTemp !== "" && modelData.highTemp !== "--") {
            var val = String(modelData.highTemp).replace(/[\u2103°C\s]/g, "").trim()
            return val ? (val + "°C") : "--"
        }
        if (modelData.tempRange) {
            var parts = String(modelData.tempRange).replace(/[\u2103°C\s]/g, "").split("~")
            if (parts.length >= 2 && parts[1].trim() !== "") {
                return parts[1].trim() + "°C"
            }
        }
        return "--"
    }

    function getWeatherText(modelData) {
        if (!modelData) return "多云"
        if (modelData.weather && modelData.weather !== "") return modelData.weather
        if (modelData.dayWeather) return modelData.dayWeather
        return "多云"
    }

    function getWindText(modelData) {
        if (!modelData) return "微风"
        if (modelData.dayWind && modelData.dayWind !== "") return modelData.dayWind
        if (modelData.wind && modelData.wind !== "") {
            var splitted = modelData.wind.split("  ")
            return splitted[0] ? splitted[0] : modelData.wind
        }
        return "微风"
    }

    onOpened: {
        if (!root.forecastList || root.forecastList.length === 0) {
            appController.refreshWeather()
        }
    }

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
        spacing: root.dp(10)

        // ================= 1. 顶部 Header 栏（半透明磨砂浮岛控制栏） =================
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.dp(34)
            spacing: root.dp(10)

            // 地理位置磨砂胶囊徽标
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                implicitHeight: root.dp(32)
                implicitWidth: locationRow.width + root.dp(22)
                radius: root.dp(16)
                color: Qt.rgba(1, 1, 1, 0.08)
                border.color: Qt.rgba(1, 1, 1, 0.15)
                border.width: 1

                Row {
                    id: locationRow
                    anchors.centerIn: parent
                    spacing: root.dp(6)

                    Rectangle {
                        width: root.dp(8)
                        height: root.dp(8)
                        radius: root.dp(4)
                        color: "#38bdf8"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.curLocation
                        color: "#f0f6fc"
                        font.pixelSize: root.fs(13)
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // 更新状态磨砂指示胶囊
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                implicitHeight: root.dp(32)
                implicitWidth: statusRow.width + root.dp(18)
                radius: root.dp(16)
                color: Qt.rgba(1, 1, 1, 0.05)
                border.color: Qt.rgba(1, 1, 1, 0.10)
                border.width: 1

                Row {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: root.dp(6)

                    Rectangle {
                        width: root.dp(6)
                        height: root.dp(6)
                        radius: root.dp(3)
                        color: "#34d399"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.curReportTime !== "" ? (qsTr("更新于 ") + root.curReportTime) : qsTr("气象台实况")
                        color: "#8ca2b4"
                        font.pixelSize: root.fs(10)
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // 手动刷新苹果液态微晶纽扣按钮
            Rectangle {
                id: refreshBtn
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: root.dp(34)
                implicitHeight: root.dp(34)
                radius: root.dp(17)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: refreshArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.24) : (refreshArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.10))
                    }
                    GradientStop {
                        position: 1.0
                        color: refreshArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.10) : (refreshArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.06) : Qt.rgba(1.0, 1.0, 1.0, 0.04))
                    }
                }
                border.color: refreshArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.35) : (refreshArea.containsMouse ? Qt.rgba(1.0, 1.0, 1.0, 0.24) : Qt.rgba(1.0, 1.0, 1.0, 0.15))
                border.width: 1
                scale: refreshArea.pressed ? 0.90 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 90 } }

                Image {
                    id: refreshIcon
                    anchors.centerIn: parent
                    source: "qrc:/icons/refresh.svg"
                    width: root.dp(16)
                    height: root.dp(16)
                    sourceSize.width: root.dp(16)
                    sourceSize.height: root.dp(16)
                    smooth: true
                    mipmap: true
                    fillMode: Image.PreserveAspectFit

                    transformOrigin: Item.Center
                    RotationAnimation on rotation {
                        id: refreshAnim
                        running: false
                        from: 0
                        to: 360
                        duration: 600
                        loops: 1
                    }
                }

                MouseArea {
                    id: refreshArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        refreshAnim.restart()
                        appController.refreshWeather()
                    }
                }
            }

            Item { Layout.fillWidth: true }

            CloseButton {
                id: closeBtn
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: root.dp(36)
                implicitHeight: root.dp(36)
                iconSize: root.dp(16)
                onClicked: root.close()
            }
        }

        // ================= 2. 今日实况区域（透光磨砂实况卡片） =================
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.dp(140)
            spacing: root.dp(10)

            // 左侧：实时天气核心 Hero 毛玻璃大卡片
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                radius: root.cardRadius
                color: Qt.rgba(1, 1, 1, 0.055)
                border.color: Qt.rgba(1, 1, 1, 0.14)
                border.width: 1
                clip: true

                // 顶部微白玻璃反射线
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
                    anchors.margins: root.dp(12)
                    spacing: root.dp(6)

                    // 上半部：大温度与天气现象、AQI
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: root.dp(10)

                        // 超大天气图标：优先高保真彩色矢量 SVG 图标
                        Image {
                            source: (root.cur && root.cur.icon) ? root.cur.icon : root.getWeatherIcon(root.curWeather)
                            sourceSize.width: root.dp(52)
                            sourceSize.height: root.dp(52)
                            Layout.preferredWidth: root.dp(52)
                            Layout.preferredHeight: root.dp(52)
                            Layout.alignment: Qt.AlignVCenter
                            fillMode: Image.PreserveAspectFit
                        }

                        // 温度数值与摄氏度符号
                        Row {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: root.dp(2)

                            Text {
                                text: root.curTemp
                                color: "#ffffff"
                                font.pixelSize: root.fs(38)
                                font.bold: true
                                verticalAlignment: Text.AlignBottom
                            }

                            Text {
                                text: "°C"
                                color: "#9ec3d8"
                                font.pixelSize: root.fs(18)
                                font.bold: true
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: root.dp(5)
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // 天气状况与 AQI 胶囊
                        ColumnLayout {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                            spacing: root.dp(4)

                            Text {
                                text: root.curWeather
                                color: "#f0f6fc"
                                font.pixelSize: root.fs(19)
                                font.bold: true
                                Layout.alignment: Qt.AlignRight
                            }

                            // 苹果健康风格 AQI 微胶囊
                            Rectangle {
                                visible: root.curAqi !== ""
                                Layout.alignment: Qt.AlignRight
                                implicitHeight: root.dp(22)
                                implicitWidth: aqiRow.width + root.dp(14)
                                radius: root.dp(11)
                                color: Qt.rgba(0, 0, 0, 0.25)
                                border.color: root.curAqiColor
                                border.width: 1

                                Row {
                                    id: aqiRow
                                    anchors.centerIn: parent
                                    spacing: root.dp(4)

                                    Rectangle {
                                        width: root.dp(6)
                                        height: root.dp(6)
                                        radius: root.dp(3)
                                        color: root.curAqiColor
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    Text {
                                        text: "AQI " + root.curAqi + " · " + root.curAqiText
                                        color: root.curAqiColor
                                        font.pixelSize: root.fs(11)
                                        font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }

                    // 下半部：生活小贴士条 (半透明磨砂气泡)
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.dp(28)
                        radius: root.dp(8)
                        color: Qt.rgba(0, 0, 0, 0.22)
                        border.color: Qt.rgba(1, 1, 1, 0.07)
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: root.dp(10)
                            anchors.rightMargin: root.dp(10)
                            spacing: root.dp(6)

                            Text {
                                text: root.curLifeHint !== "" 
                                      ? (root.curLifeTitle + "提示: " + root.curLifeHint)
                                      : ("舒适度良好，适宜进行各类户外活动")
                                color: "#b0ccdf"
                                font.pixelSize: root.fs(11)
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            // 右侧：4 宫格实况指标微卡片 (2x2 Frosted Grid)
            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                columns: 2
                rowSpacing: root.dp(6)
                columnSpacing: root.dp(6)

                // 指标 1：相对湿度
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: root.chipRadius
                    color: Qt.rgba(1, 1, 1, 0.045)
                    border.color: Qt.rgba(1, 1, 1, 0.09)
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: root.dp(2)

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: root.dp(4)
                            Text { text: "相对湿度"; color: "#8aa9be"; font.pixelSize: root.fs(11); anchors.verticalCenter: parent.verticalCenter }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.curHumidity
                            color: "#f0f6fc"
                            font.pixelSize: root.fs(15)
                            font.bold: true
                        }
                    }
                }

                // 指标 2：风向风力
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: root.chipRadius
                    color: Qt.rgba(1, 1, 1, 0.045)
                    border.color: Qt.rgba(1, 1, 1, 0.09)
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: root.dp(2)

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: root.dp(4)
                            Text { text: "实时风况"; color: "#8aa9be"; font.pixelSize: root.fs(11); anchors.verticalCenter: parent.verticalCenter }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: (root.curWindDir + " " + root.curWindPower).trim()
                            color: "#f0f6fc"
                            font.pixelSize: root.fs(13)
                            font.bold: true
                            elide: Text.ElideRight
                        }
                    }
                }

                // 指标 3：体感温度
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: root.chipRadius
                    color: Qt.rgba(1, 1, 1, 0.045)
                    border.color: Qt.rgba(1, 1, 1, 0.09)
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: root.dp(2)

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: root.dp(4)
                            Text { text: "体感温度"; color: "#8aa9be"; font.pixelSize: root.fs(11); anchors.verticalCenter: parent.verticalCenter }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.curTemp !== "--" ? (root.curTemp + "°C") : "--"
                            color: "#f0f6fc"
                            font.pixelSize: root.fs(15)
                            font.bold: true
                        }
                    }
                }

                // 指标 4：空气质量
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: root.chipRadius
                    color: Qt.rgba(1, 1, 1, 0.045)
                    border.color: Qt.rgba(1, 1, 1, 0.09)
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: root.dp(2)

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: root.dp(4)
                            Text { text: "空气质量"; color: "#8aa9be"; font.pixelSize: root.fs(11); anchors.verticalCenter: parent.verticalCenter }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.curAqi !== "" ? (root.curAqi + " " + root.curAqiText) : "优良"
                            color: root.curAqiColor
                            font.pixelSize: root.fs(14)
                            font.bold: true
                        }
                    }
                }
            }
        }

        // ================= 3. 未来 3 天天气预报（苹果 iOS 磨砂悬浮甲板） =================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.dp(6)

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "未来 3 天天气趋势预报"
                    color: "#85a7bd"
                    font.pixelSize: root.fs(12)
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "中国天气网权威数据"
                    color: "#5c7c91"
                    font.pixelSize: root.fs(11)
                }
            }

            Row {
                id: forecastRow
                Layout.fillWidth: true
                Layout.preferredHeight: root.dp(156)
                spacing: root.dp(10)

                Repeater {
                    model: (root.forecastList && root.forecastList.length > 0) ? root.forecastList : root.fallbackForecast

                    Rectangle {
                        width: Math.max(root.dp(60), Math.floor((forecastRow.width - root.dp(20)) / 3))
                        height: root.dp(156)
                        clip: true
                        radius: root.cardRadius
                        color: index === 0 ? Qt.rgba(0.13, 0.30, 0.46, 0.55) : Qt.rgba(1, 1, 1, 0.04)
                        border.color: index === 0 ? Qt.rgba(0.35, 0.72, 1.0, 0.45) : Qt.rgba(1, 1, 1, 0.08)
                        border.width: index === 0 ? 1.5 : 1

                        // 卡片顶部月白/冰蓝高光线
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.leftMargin: root.cardRadius
                            anchors.rightMargin: root.cardRadius
                            height: 1
                            color: index === 0 ? "#80d0ff" : "#ffffff"
                            opacity: index === 0 ? 0.45 : 0.15
                        }

                        Column {
                            anchors.fill: parent
                            anchors.topMargin: root.dp(10)
                            anchors.bottomMargin: root.dp(10)
                            anchors.leftMargin: root.dp(10)
                            anchors.rightMargin: root.dp(10)
                            spacing: root.dp(6)

                            // 1. 日期与星期栏（高度固定 22dp）
                            Item {
                                width: parent.width
                                height: root.dp(22)

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    height: root.dp(20)
                                    width: labelTxt.width + root.dp(14)
                                    radius: root.dp(10)
                                    color: index === 0 ? Qt.rgba(0.20, 0.60, 0.95, 0.45) : Qt.rgba(1, 1, 1, 0.08)
                                    border.color: index === 0 ? Qt.rgba(0.40, 0.80, 1.0, 0.6) : Qt.rgba(1, 1, 1, 0.12)
                                    border.width: 1

                                    Text {
                                        id: labelTxt
                                        anchors.centerIn: parent
                                        text: modelData.label || (index === 0 ? "今天" : (index === 1 ? "明天" : "后天"))
                                        color: index === 0 ? "#ffffff" : "#b0c8d8"
                                        font.pixelSize: root.fs(11)
                                        font.bold: true
                                    }
                                }

                                Text {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.week || ""
                                    color: index === 0 ? "#a8d8f8" : "#7294aa"
                                    font.pixelSize: root.fs(12)
                                    font.bold: index === 0
                                }
                            }

                            // 2. 天气图标与天气现象（高度固定 36dp）
                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                height: root.dp(36)
                                spacing: root.dp(6)

                                Image {
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: (modelData && modelData.icon) ? modelData.icon : root.getWeatherIcon(modelData ? modelData.weather : "")
                                    sourceSize.width: root.dp(26)
                                    sourceSize.height: root.dp(26)
                                    width: root.dp(26)
                                    height: root.dp(26)
                                    fillMode: Image.PreserveAspectFit
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.getWeatherText(modelData)
                                    color: "#f0f6fc"
                                    font.pixelSize: root.fs(14)
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                            }

                            // 3. 高低气温直观对比（高度固定 28dp）
                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                height: root.dp(28)
                                spacing: root.dp(6)

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.getLowTemp(modelData)
                                    color: "#54a0ff"
                                    font.pixelSize: root.fs(16)
                                    font.bold: true
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "~"
                                    color: "#6c8da3"
                                    font.pixelSize: root.fs(14)
                                    font.bold: true
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.getHighTemp(modelData)
                                    color: "#ff9f43"
                                    font.pixelSize: root.fs(16)
                                    font.bold: true
                                }
                            }

                            // 4. 底部风向风力小胶囊（高度固定 22dp，稳居卡片内部）
                            Rectangle {
                                width: parent.width
                                height: root.dp(22)
                                radius: root.dp(6)
                                color: Qt.rgba(0, 0, 0, 0.28)
                                border.color: Qt.rgba(1, 1, 1, 0.06)
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: root.getWindText(modelData)
                                    color: "#8aaec2"
                                    font.pixelSize: root.fs(10)
                                    elide: Text.ElideRight
                                    width: parent.width - root.dp(8)
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
