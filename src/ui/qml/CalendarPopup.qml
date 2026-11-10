import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import "qrc:/qml/LunarCalendar.js" as LunarCalendar

Popup {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(22)
    property int cardRadius: dp(14)
    property int chipRadius: dp(10)
    property var calendarDisplayDate: new Date()
    property var calendarSelectedDate: new Date()
    property bool yearPickerMode: false
    property int yearPickerBase: calendarDisplayDate.getFullYear() - 5
    property var calendarCellModels: []
    property string selectedLunarText: ""

    readonly property var weekTitles: [qsTr("一"), qsTr("二"), qsTr("三"), qsTr("四"), qsTr("五"), qsTr("六"), qsTr("日")]
    readonly property var monthTitles: [
        qsTr("1月"), qsTr("2月"), qsTr("3月"), qsTr("4月"),
        qsTr("5月"), qsTr("6月"), qsTr("7月"), qsTr("8月"),
        qsTr("9月"), qsTr("10月"), qsTr("11月"), qsTr("12月")
    ]

    // 视图模式: 0 = 日历日期视图, 1 = 月份选择视图, 2 = 年份选择视图
    property int viewMode: 0

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }
    function normalizedDate(date) {
        return new Date(date.getFullYear(), date.getMonth(), date.getDate())
    }
    function sameDate(a, b) {
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate()
    }
    function monthTitle(date) {
        return date.getFullYear() + "年" + (date.getMonth() + 1) + "月"
    }
    function monthAnchor(date) {
        return new Date(date.getFullYear(), date.getMonth(), 1)
    }
    function shiftMonth(date, delta) {
        return new Date(date.getFullYear(), date.getMonth() + delta, 1)
    }
    function shiftYear(date, delta) {
        return new Date(date.getFullYear() + delta, date.getMonth(), 1)
    }
    function setDisplayYear(year) {
        calendarDisplayDate = new Date(year, calendarDisplayDate.getMonth(), 1)
        root.viewMode = 0
    }
    function setDisplayMonth(monthIndex) {
        calendarDisplayDate = new Date(calendarDisplayDate.getFullYear(), monthIndex, 1)
        root.viewMode = 0
    }
    function miniMonthCells(year, monthIndex) {
        var monthStart = new Date(year, monthIndex, 1)
        var firstWeekday = monthStart.getDay()
        var mondayBased = firstWeekday === 0 ? 6 : firstWeekday - 1
        var gridStart = new Date(year, monthIndex, 1 - mondayBased)
        var cells = []
        for (var i = 0; i < 42; ++i) {
            var date = new Date(gridStart.getFullYear(), gridStart.getMonth(), gridStart.getDate() + i)
            cells.push({
                day: date.getDate(),
                inMonth: date.getMonth() === monthIndex,
                isToday: root.sameDate(date, root.normalizedDate(new Date()))
            })
        }
        return cells
    }
    function isoDate(date) {
        var year = date.getFullYear()
        var month = ("0" + (date.getMonth() + 1)).slice(-2)
        var day = ("0" + date.getDate()).slice(-2)
        return year + "-" + month + "-" + day
    }
    function calendarCells() {
        var monthStart = monthAnchor(calendarDisplayDate)
        var firstWeekday = monthStart.getDay()
        var mondayBased = firstWeekday === 0 ? 6 : firstWeekday - 1
        var gridStart = new Date(monthStart.getFullYear(), monthStart.getMonth(), 1 - mondayBased)
        var today = normalizedDate(new Date())
        var cells = []
        for (var i = 0; i < 42; ++i) {
            var date = new Date(gridStart.getFullYear(), gridStart.getMonth(), gridStart.getDate() + i)
            var lunar = LunarCalendar.solarToLunar(date)
            cells.push({
                date: date,
                isoDate: isoDate(date),
                day: date.getDate(),
                inMonth: date.getMonth() === calendarDisplayDate.getMonth(),
                isToday: sameDate(date, today),
                isSelected: sameDate(date, calendarSelectedDate),
                lunarText: lunar.shortText,
                lunarFullText: lunar.fullText
            })
        }
        return cells
    }
    function refreshCalendarModel() {
        calendarCellModels = calendarCells()
        selectedLunarText = LunarCalendar.solarToLunar(calendarSelectedDate).fullText
    }
    function selectCalendarDate(date) {
        calendarSelectedDate = normalizedDate(date)
        if (calendarSelectedDate.getMonth() !== calendarDisplayDate.getMonth()
                || calendarSelectedDate.getFullYear() !== calendarDisplayDate.getFullYear()) {
            calendarDisplayDate = monthAnchor(calendarSelectedDate)
        }
    }
    function openForToday() {
        root.viewMode = 0
        calendarDisplayDate = monthAnchor(new Date())
        selectCalendarDate(new Date())
        open()
    }

    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height
    modal: true
    focus: true
    width: Math.min((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 800)) * 0.94, dp(isPortrait ? 390 : 580))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 480)) * (isPortrait ? 0.85 : 0.94), dp(isPortrait ? 490 : 430))
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 800)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 480)) - height) / 2) + Math.round(dragOffsetY)
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
        root.viewMode = 0
        root.dragOffsetY = 0
        root.isDraggingDown = false
    }

    Overlay.modal: Rectangle {
        color: Theme.colorOverlayModal
    }

    background: Rectangle {
        radius: root.panelRadius
        color: Qt.rgba(0.07, 0.12, 0.18, 0.88)
        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.16)
        border.width: 1
        clip: true

        // 左上冰蓝流体漫反射光晕
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: -root.dp(60)
            width: root.dp(240)
            height: root.dp(240)
            radius: width / 2
            color: Qt.rgba(0.20, 0.58, 0.95, 0.14)
        }

        // 右下紫晶微暖光晕
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: -root.dp(50)
            width: root.dp(200)
            height: root.dp(200)
            radius: width / 2
            color: Qt.rgba(0.42, 0.22, 0.68, 0.08)
        }
    }

    Component.onCompleted: refreshCalendarModel()
    onCalendarDisplayDateChanged: {
        appController.prepareHolidayYear(calendarDisplayDate.getFullYear())
        refreshCalendarModel()
    }
    onCalendarSelectedDateChanged: refreshCalendarModel()

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
        spacing: root.dp(10)

        // 顶部年月与切换栏
        RowLayout {
            Layout.fillWidth: true

            // 上一个月苹果微光玻璃胶囊
            Rectangle {
                implicitWidth: root.dp(36)
                implicitHeight: root.dp(34)
                radius: root.dp(10)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: calendarPrevArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.22) : Qt.rgba(1.0, 1.0, 1.0, 0.11)
                    }
                    GradientStop {
                        position: 1.0
                        color: calendarPrevArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                    }
                }
                border.color: calendarPrevArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.35) : Qt.rgba(1.0, 1.0, 1.0, 0.16)
                border.width: 1
                scale: calendarPrevArea.pressed ? 0.90 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 90 } }

                MouseArea {
                    id: calendarPrevArea
                    anchors.fill: parent
                    onClicked: {
                        if (root.viewMode === 2) {
                            root.yearPickerBase -= 12
                        } else if (root.viewMode === 1) {
                            root.calendarDisplayDate = root.shiftYear(root.calendarDisplayDate, -1)
                        } else {
                            root.calendarDisplayDate = root.shiftMonth(root.calendarDisplayDate, -1)
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "‹"
                    color: "#eef7fb"
                    font.pixelSize: root.fs(20)
                    font.bold: true
                }
            }

            Item { Layout.fillWidth: true }

            Column {
                Layout.fillWidth: true
                Layout.maximumWidth: root.dp(240)
                spacing: root.dp(3)

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: root.dp(8)

                    // 年份选择苹果微光胶囊
                    Rectangle {
                        id: yearTitle
                        width: yearText.implicitWidth + root.dp(18)
                        height: root.dp(32)
                        radius: root.dp(10)
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: root.viewMode === 2
                                       ? Qt.rgba(0.0, 0.48, 1.0, 0.70)
                                       : (yearTitleArea.pressed ? Qt.rgba(0.20, 0.60, 1.0, 0.28) : Qt.rgba(1.0, 1.0, 1.0, 0.09))
                            }
                            GradientStop {
                                position: 1.0
                                color: root.viewMode === 2
                                       ? Qt.rgba(0.0, 0.35, 0.85, 0.80)
                                       : (yearTitleArea.pressed ? Qt.rgba(0.10, 0.40, 0.80, 0.20) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                            }
                        }
                        border.color: root.viewMode === 2
                                      ? Qt.rgba(0.65, 0.88, 1.0, 0.90)
                                      : (yearTitleArea.pressed ? Qt.rgba(0.50, 0.80, 1.0, 0.55) : Qt.rgba(1.0, 1.0, 1.0, 0.12))
                        border.width: 1
                        scale: yearTitleArea.pressed ? 0.94 : 1.0
                        clip: true

                        Behavior on scale { NumberAnimation { duration: 90 } }

                        MouseArea {
                            id: yearTitleArea
                            anchors.fill: parent
                            onClicked: {
                                if (root.viewMode === 2) {
                                    root.viewMode = 0
                                } else {
                                    root.yearPickerBase = root.calendarDisplayDate.getFullYear() - 5
                                    root.viewMode = 2
                                }
                            }
                        }

                        Text {
                            id: yearText
                            anchors.centerIn: parent
                            text: root.calendarDisplayDate.getFullYear() + "年"
                            color: "#f4f8fb"
                            font.pixelSize: root.fs(18)
                            font.bold: true
                        }
                    }

                    // 月份选择苹果微光胶囊
                    Rectangle {
                        id: monthTitle
                        width: monthText.implicitWidth + root.dp(18)
                        height: root.dp(32)
                        radius: root.dp(10)
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: root.viewMode === 1
                                       ? Qt.rgba(0.0, 0.48, 1.0, 0.70)
                                       : (monthTitleArea.pressed ? Qt.rgba(0.20, 0.60, 1.0, 0.28) : Qt.rgba(1.0, 1.0, 1.0, 0.09))
                            }
                            GradientStop {
                                position: 1.0
                                color: root.viewMode === 1
                                       ? Qt.rgba(0.0, 0.35, 0.85, 0.80)
                                       : (monthTitleArea.pressed ? Qt.rgba(0.10, 0.40, 0.80, 0.20) : Qt.rgba(1.0, 1.0, 1.0, 0.03))
                            }
                        }
                        border.color: root.viewMode === 1
                                      ? Qt.rgba(0.65, 0.88, 1.0, 0.90)
                                      : (monthTitleArea.pressed ? Qt.rgba(0.50, 0.80, 1.0, 0.55) : Qt.rgba(1.0, 1.0, 1.0, 0.12))
                        border.width: 1
                        scale: monthTitleArea.pressed ? 0.94 : 1.0
                        clip: true

                        Behavior on scale { NumberAnimation { duration: 90 } }

                        MouseArea {
                            id: monthTitleArea
                            anchors.fill: parent
                            onClicked: {
                                root.viewMode = (root.viewMode === 1 ? 0 : 1)
                            }
                        }

                        Text {
                            id: monthText
                            anchors.centerIn: parent
                            text: (root.calendarDisplayDate.getMonth() + 1) + "月"
                            color: "#f4f8fb"
                            font.pixelSize: root.fs(18)
                            font.bold: true
                        }
                    }
                }

                Text {
                    text: root.selectedLunarText
                    color: "#85d8ff"
                    font.pixelSize: root.fs(11)
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }

            Item { Layout.fillWidth: true }

            // 下一个月苹果微光玻璃胶囊
            Rectangle {
                implicitWidth: root.dp(36)
                implicitHeight: root.dp(34)
                radius: root.dp(10)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: calendarNextArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.22) : Qt.rgba(1.0, 1.0, 1.0, 0.11)
                    }
                    GradientStop {
                        position: 1.0
                        color: calendarNextArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                    }
                }
                border.color: calendarNextArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.35) : Qt.rgba(1.0, 1.0, 1.0, 0.16)
                border.width: 1
                scale: calendarNextArea.pressed ? 0.90 : 1.0
                clip: true

                Behavior on scale { NumberAnimation { duration: 90 } }

                MouseArea {
                    id: calendarNextArea
                    anchors.fill: parent
                    onClicked: {
                        if (root.viewMode === 2) {
                            root.yearPickerBase += 12
                        } else if (root.viewMode === 1) {
                            root.calendarDisplayDate = root.shiftYear(root.calendarDisplayDate, 1)
                        } else {
                            root.calendarDisplayDate = root.shiftMonth(root.calendarDisplayDate, 1)
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "›"
                    color: "#eef7fb"
                    font.pixelSize: root.fs(20)
                    font.bold: true
                }
            }

            Item { width: root.dp(6) }

            CloseButton {
                implicitWidth: root.dp(36)
                implicitHeight: root.dp(36)
                iconSize: root.dp(16)
                onClicked: root.close()
            }
        }

        // 主体内容区域（根据 viewMode 切换日期网格、月份选择或年份选择）
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 0. 日历日期网格视图
            ColumnLayout {
                anchors.fill: parent
                visible: root.viewMode === 0
                spacing: root.dp(8)

                // 星期栏（精巧毛玻璃药丸行）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.dp(6)

                    Repeater {
                        model: root.weekTitles

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: root.dp(24)
                            radius: root.dp(8)
                            color: Qt.rgba(1.0, 1.0, 1.0, 0.04)
                            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.06)
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: index >= 5 ? "#79d2ff" : "#d0e4f0"
                                font.pixelSize: root.fs(11)
                                font.bold: true
                            }
                        }
                    }
                }

                // 42 天完整日历网格
                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 7
                    rowSpacing: root.dp(isPortrait ? 5 : 6)
                    columnSpacing: root.dp(isPortrait ? 4 : 6)

                    Repeater {
                        model: root.calendarCellModels

                        Rectangle {
                            id: cellRect
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: root.dp(isPortrait ? 38 : 34)
                            radius: root.dp(8)

                            // iOS 风格卡片状态背景
                            color: modelData.isSelected
                                   ? Qt.rgba(0.0, 0.48, 1.0, 0.70)
                                   : (modelData.isToday
                                      ? Qt.rgba(0.20, 0.55, 0.85, 0.22)
                                      : (cellArea.pressed
                                         ? Qt.rgba(1.0, 1.0, 1.0, 0.12)
                                         : (modelData.inMonth ? Qt.rgba(1.0, 1.0, 1.0, 0.05) : Qt.rgba(1.0, 1.0, 1.0, 0.02))))

                            border.color: modelData.isSelected
                                          ? Qt.rgba(0.65, 0.88, 1.0, 0.85)
                                          : (modelData.isToday
                                             ? Qt.rgba(0.40, 0.75, 1.0, 0.55)
                                             : (modelData.inMonth ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.03)))
                            border.width: 1
                            opacity: modelData.inMonth ? 1.0 : 0.40
                            scale: cellArea.pressed ? 0.94 : 1.0
                            clip: true

                            Behavior on scale { NumberAnimation { duration: 90 } }
                            Behavior on color { ColorAnimation { duration: 100 } }

                            MouseArea {
                                id: cellArea
                                anchors.fill: parent
                                onClicked: root.selectCalendarDate(modelData.date)
                            }

                            // 班/休小药丸徽章
                            Rectangle {
                                readonly property string holidayBadge: (typeof appController !== "undefined" && appController) ? appController.holidayBadgeForDate(modelData.isoDate) : ""
                                visible: holidayBadge.length > 0
                                anchors.top: parent.top
                                anchors.right: parent.right
                                anchors.topMargin: root.dp(3)
                                anchors.rightMargin: root.dp(3)
                                width: root.dp(14)
                                height: root.dp(14)
                                radius: root.dp(4)
                                color: holidayBadge === "班" ? Qt.rgba(0.18, 0.35, 0.58, 0.85) : Qt.rgba(0.85, 0.22, 0.28, 0.85)
                                border.color: holidayBadge === "班" ? Qt.rgba(0.55, 0.78, 1.0, 0.80) : Qt.rgba(1.0, 0.65, 0.70, 0.80)
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: parent.holidayBadge
                                    color: "#ffffff"
                                    font.pixelSize: root.fs(7.5)
                                    font.bold: true
                                }
                            }

                            // 公历日期数字
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.verticalCenterOffset: -root.dp(isPortrait ? 5 : 6)
                                text: modelData.day
                                color: modelData.isSelected ? "#ffffff" : (modelData.inMonth ? "#f0f6fa" : Qt.rgba(1.0, 1.0, 1.0, 0.28))
                                font.pixelSize: root.fs(isPortrait ? 15 : 16)
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }

                            // 农历文字
                            Text {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: root.dp(isPortrait ? 3 : 5)
                                text: modelData.lunarText
                                color: modelData.isSelected ? "#d9f4ff" : (modelData.inMonth ? "#8aaec0" : Qt.rgba(1.0, 1.0, 1.0, 0.20))
                                font.pixelSize: root.fs(isPortrait ? 8.5 : 9)
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.NoWrap
                            }
                        }
                    }
                }
            }

            // 1. 月份选择网格视图 (4 列 x 3 行)
            GridLayout {
                anchors.fill: parent
                visible: root.viewMode === 1
                columns: 4
                rowSpacing: root.dp(isPortrait ? 12 : 8)
                columnSpacing: root.dp(isPortrait ? 12 : 8)

                Repeater {
                    model: root.monthTitles

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: root.dp(12)
                        color: index === root.calendarDisplayDate.getMonth()
                               ? Qt.rgba(0.0, 0.48, 1.0, 0.70)
                               : (monthCardArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.06))
                        border.color: index === root.calendarDisplayDate.getMonth()
                                      ? Qt.rgba(0.65, 0.88, 1.0, 0.85)
                                      : (monthCardArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.25) : Qt.rgba(1.0, 1.0, 1.0, 0.10))
                        border.width: 1
                        scale: monthCardArea.pressed ? 0.95 : 1.0
                        clip: true

                        Behavior on scale { NumberAnimation { duration: 90 } }

                        MouseArea {
                            id: monthCardArea
                            anchors.fill: parent
                            onClicked: root.setDisplayMonth(index)
                        }

                        Column {
                            anchors.centerIn: parent
                            spacing: root.dp(4)

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData
                                color: "#f4f8fb"
                                font.pixelSize: root.fs(17)
                                font.bold: true
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"][index]
                                color: index === root.calendarDisplayDate.getMonth() ? "#d0e8ff" : "#7f9aac"
                                font.pixelSize: root.fs(10)
                                font.letterSpacing: 1
                            }
                        }
                    }
                }
            }

            // 2. 年份选择视图 (4 列 x 3 行 + 年段翻页)
            ColumnLayout {
                anchors.fill: parent
                visible: root.viewMode === 2
                spacing: root.dp(10)

                // 年段翻页器
                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.dp(8)

                    Rectangle {
                        implicitWidth: root.dp(36)
                        implicitHeight: root.dp(32)
                        radius: root.dp(9)
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: prevYearBlockArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.22) : Qt.rgba(1.0, 1.0, 1.0, 0.10)
                            }
                            GradientStop {
                                position: 1.0
                                color: prevYearBlockArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                            }
                        }
                        border.color: prevYearBlockArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.35) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                        border.width: 1
                        scale: prevYearBlockArea.pressed ? 0.90 : 1.0

                        Behavior on scale { NumberAnimation { duration: 90 } }

                        MouseArea {
                            id: prevYearBlockArea
                            anchors.fill: parent
                            onClicked: root.yearPickerBase -= 12
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "‹"
                            color: "#eef7fb"
                            font.pixelSize: root.fs(18)
                            font.bold: true
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.yearPickerBase + " - " + (root.yearPickerBase + 11)
                        color: "#f4f8fb"
                        font.pixelSize: root.fs(16)
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Rectangle {
                        implicitWidth: root.dp(36)
                        implicitHeight: root.dp(32)
                        radius: root.dp(9)
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: nextYearBlockArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.22) : Qt.rgba(1.0, 1.0, 1.0, 0.10)
                            }
                            GradientStop {
                                position: 1.0
                                color: nextYearBlockArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.08) : Qt.rgba(1.0, 1.0, 1.0, 0.04)
                            }
                        }
                        border.color: nextYearBlockArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.35) : Qt.rgba(1.0, 1.0, 1.0, 0.15)
                        border.width: 1
                        scale: nextYearBlockArea.pressed ? 0.90 : 1.0

                        Behavior on scale { NumberAnimation { duration: 90 } }

                        MouseArea {
                            id: nextYearBlockArea
                            anchors.fill: parent
                            onClicked: root.yearPickerBase += 12
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "›"
                            color: "#eef7fb"
                            font.pixelSize: root.fs(18)
                            font.bold: true
                        }
                    }
                }

                // 12 个年份卡片
                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: 4
                    rowSpacing: root.dp(isPortrait ? 10 : 8)
                    columnSpacing: root.dp(isPortrait ? 10 : 8)

                    Repeater {
                        model: 12

                        Rectangle {
                            readonly property int displayYear: root.yearPickerBase + index
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: root.dp(12)
                            color: displayYear === root.calendarDisplayDate.getFullYear()
                                   ? Qt.rgba(0.0, 0.48, 1.0, 0.70)
                                   : (yearCardArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.16) : Qt.rgba(1.0, 1.0, 1.0, 0.06))
                            border.color: displayYear === root.calendarDisplayDate.getFullYear()
                                          ? Qt.rgba(0.65, 0.88, 1.0, 0.85)
                                          : (yearCardArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.25) : Qt.rgba(1.0, 1.0, 1.0, 0.10))
                            border.width: 1
                            scale: yearCardArea.pressed ? 0.95 : 1.0

                            Behavior on scale { NumberAnimation { duration: 90 } }

                            MouseArea {
                                id: yearCardArea
                                anchors.fill: parent
                                onClicked: root.setDisplayYear(parent.displayYear)
                            }

                            Text {
                                anchors.centerIn: parent
                                text: parent.displayYear
                                color: "#f4f8fb"
                                font.pixelSize: root.fs(16)
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }

        // 底部详情说明行
        RowLayout {
            Layout.fillWidth: true
            spacing: root.dp(8)

            Text {
                text: root.calendarSelectedDate.getFullYear() + "年"
                      + (root.calendarSelectedDate.getMonth() + 1) + "月"
                      + root.calendarSelectedDate.getDate() + "日"
                color: "#e2eff8"
                font.pixelSize: root.fs(12)
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.selectedLunarText
                color: "#79d2ff"
                font.pixelSize: root.fs(11)
                elide: Text.ElideRight
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}
