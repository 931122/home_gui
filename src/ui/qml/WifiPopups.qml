import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0

Item {
    id: root

    property real scaleUnit: 1.0
    property int panelRadius: dp(22)
    property int cardRadius: dp(16)
    property int chipRadius: dp(10)

    property string pendingWifiSsid: ""
    property bool keyboardShift: false
    property bool keyboardSymbolMode: false
    property bool anyPopupOpen: wifiPopup.opened || wifiConnectPopup.opened || keyboardPopup.opened
    readonly property bool anyPopupOpened: anyPopupOpen

    function dp(value) { return Theme.dp(value) }
    function fs(value) { return Theme.fs(value) }

    function openWifiPopup() {
        wifiPopup.open()
    }

    function closeTopPopup() {
        if (keyboardPopup.opened) {
            keyboardPopup.close()
            return true
        }
        if (wifiConnectPopup.opened) {
            wifiConnectPopup.close()
            return true
        }
        if (wifiPopup.opened) {
            wifiPopup.close()
            return true
        }
        return false
    }

    function keyboardRows() {
        if (keyboardSymbolMode) {
            return [
                ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
                ["@", "#", "$", "%", "&", "-", "_", "+", "(", ")"],
                ["*", "\"", "'", ":", ";", "!", "?", "/", "\\", "="],
                [".", ",", "[", "]", "{", "}", "<", ">", "|", "~"]
            ]
        }

        return [
            ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
            keyboardShift ? ["Q","W","E","R","T","Y","U","I","O","P"] : ["q","w","e","r","t","y","u","i","o","p"],
            keyboardShift ? ["A","S","D","F","G","H","J","K","L"] : ["a","s","d","f","g","h","j","k","l"],
            keyboardShift ? ["Z","X","C","V","B","N","M"] : ["z","x","c","v","b","n","m"]
        ]
    }

    function appendPasswordKey(value) {
        wifiPasswordField.insert(wifiPasswordField.length, value)
        if (keyboardShift && !keyboardSymbolMode) {
            keyboardShift = false
        }
    }

    // 1. Wi-Fi 主弹窗
    Popup {
        id: wifiPopup
        modal: true
        focus: true
        width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.95, root.dp(620))
        height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.94, root.dp(396))
        anchors.centerIn: Overlay.overlay
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: {
            if (appController.wifiAvailable && !appController.wifiScanning) {
                appController.scanWifi()
            }
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

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: -root.dp(60)
                width: root.dp(240)
                height: root.dp(240)
                radius: width / 2
                color: Qt.rgba(0.20, 0.58, 0.95, 0.14)
            }

            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: -root.dp(60)
                width: root.dp(220)
                height: root.dp(220)
                radius: width / 2
                color: Qt.rgba(0.42, 0.22, 0.68, 0.08)
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.dp(16)
            spacing: root.dp(12)

            // 顶部栏
            RowLayout {
                Layout.fillWidth: true

                Column {
                    spacing: root.dp(2)

                    Text {
                        text: "Wi-Fi 网络"
                        color: "#f4f8fb"
                        font.pixelSize: root.fs(22)
                        font.bold: true
                    }

                    Text {
                        text: appController.wifiStatus
                        color: "#85d8ff"
                        font.pixelSize: root.fs(11)
                    }
                }

                Item { Layout.fillWidth: true }

                // 扫描按钮（磨砂玻璃胶囊）
                GlassButton {
                    implicitWidth: root.dp(116)
                    implicitHeight: root.dp(34)
                    scaleUnit: root.scaleUnit
                    styleType: "accent"
                    text: appController.wifiScanning ? "扫描中..." : "扫描网络"
                    textPixelSize: root.fs(12)
                    disabled: !appController.wifiAvailable || appController.wifiScanning
                    onClicked: appController.scanWifi()
                }

                CloseButton {
                    implicitWidth: root.dp(36)
                    implicitHeight: root.dp(36)
                    iconSize: root.dp(16)
                    onClicked: wifiPopup.close()
                }
            }

            // 列表容器
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: root.cardRadius
                color: Qt.rgba(1.0, 1.0, 1.0, 0.04)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                border.width: 1
                clip: true

                ListView {
                    anchors.fill: parent
                    anchors.margins: root.dp(8)
                    spacing: root.dp(6)
                    clip: true
                    model: appController.wifiNetworks

                    delegate: Rectangle {
                        id: wifiItemRect
                        width: ListView.view.width
                        height: root.dp(54)
                        radius: root.chipRadius
                        color: modelData.connected
                               ? Qt.rgba(0.0, 0.48, 1.0, 0.22)
                               : (wifiItemMouseArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.12) : Qt.rgba(1.0, 1.0, 1.0, 0.04))
                        border.color: modelData.connected
                                      ? Qt.rgba(0.40, 0.78, 1.0, 0.65)
                                      : Qt.rgba(1.0, 1.0, 1.0, 0.07)
                        border.width: 1
                        scale: wifiItemMouseArea.pressed ? 0.98 : 1.0
                        clip: true

                        Behavior on scale { NumberAnimation { duration: 90 } }
                        Behavior on color { ColorAnimation { duration: 100 } }

                        MouseArea {
                            id: wifiItemMouseArea
                            anchors.fill: parent
                            onClicked: {
                                root.pendingWifiSsid = modelData.ssid
                                wifiPasswordField.text = ""
                                wifiConnectPopup.open()
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: root.dp(12)
                            spacing: root.dp(8)

                            // Wi-Fi 徽标
                            Rectangle {
                                width: root.dp(20)
                                height: root.dp(20)
                                radius: root.dp(4)
                                color: modelData.connected ? Qt.rgba(0.2, 0.7, 1.0, 0.3) : Qt.rgba(1, 1, 1, 0.1)
                                border.color: modelData.connected ? "#72d2ff" : Qt.rgba(1, 1, 1, 0.2)
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "W"
                                    color: modelData.connected ? "#72d2ff" : "#ffffff"
                                    font.pixelSize: root.fs(10)
                                    font.bold: true
                                }
                            }

                            Column {
                                spacing: root.dp(2)

                                Text {
                                    text: modelData.ssid
                                    color: "#f4f8fb"
                                    font.pixelSize: root.fs(14)
                                    font.bold: true
                                }

                                Row {
                                    spacing: root.dp(6)

                                    Text {
                                        text: (modelData.flags ? modelData.flags + "  " : "") + "RSSI " + modelData.signal
                                        color: "#8ca8b8"
                                        font.pixelSize: root.fs(10)
                                    }

                                    // 已保存药丸
                                    Rectangle {
                                        visible: modelData.saved
                                        width: savedLabel.implicitWidth + root.dp(8)
                                        height: root.dp(14)
                                        radius: height / 2
                                        color: Qt.rgba(0.20, 0.75, 0.40, 0.20)
                                        border.color: Qt.rgba(0.30, 0.85, 0.50, 0.40)
                                        border.width: 0.5

                                        Text {
                                            id: savedLabel
                                            anchors.centerIn: parent
                                            text: "已保存"
                                            color: "#85f0a8"
                                            font.pixelSize: root.fs(8)
                                            font.bold: true
                                        }
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            HeaderActionButton {
                                visible: modelData.saved && !modelData.connected
                                scaleUnit: root.scaleUnit
                                cornerRadius: root.dp(8)
                                label: "忘记"
                                activeColor: Qt.rgba(0.85, 0.22, 0.28, 0.18)
                                pressedColor: Qt.rgba(0.85, 0.22, 0.28, 0.35)
                                activeBorderColor: Qt.rgba(0.95, 0.45, 0.50, 0.40)
                                pressedBorderColor: Qt.rgba(1.0, 0.65, 0.70, 0.70)
                                activeTextColor: "#ffdce0"
                                pressedTextColor: "#ffffff"
                                implicitWidth: root.dp(68)
                                implicitHeight: root.dp(28)
                                onClicked: {
                                    appController.forgetWifi(modelData.ssid)
                                }
                            }

                            Text {
                                text: modelData.connected ? "已连接" : "连接"
                                color: modelData.connected ? "#72e59e" : "#85d8ff"
                                font.pixelSize: root.fs(11)
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
    }

    // 2. 密码输入连接子弹窗
    Popup {
        id: wifiConnectPopup
        parent: Overlay.overlay
        modal: true
        focus: true
        width: Math.min((parent ? parent.width : 800) * 0.92, root.dp(520))
        height: Math.min((parent ? parent.height : 480) * 0.92, root.dp(246))
        x: parent ? (parent.width - width) / 2 : 0
        y: keyboardPopup.visible ? root.dp(34) : (parent ? (parent.height - height) / 2 : 0)
        padding: 0
        closePolicy: Popup.NoAutoClose
        onOpened: {
            root.keyboardShift = false
            root.keyboardSymbolMode = false
            keyboardPopup.open()
        }
        onClosed: keyboardPopup.close()

        Overlay.modal: Rectangle {
            color: Theme.colorOverlayModal
        }

        Behavior on y {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        background: Rectangle {
            radius: root.panelRadius
            color: Qt.rgba(0.07, 0.12, 0.18, 0.92)
            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.18)
            border.width: 1
            clip: true
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.dp(16)
            spacing: root.dp(12)

            Text {
                text: "连接 " + root.pendingWifiSsid
                color: "#f4f8fb"
                font.pixelSize: root.fs(20)
                font.bold: true
            }

            // iOS 磨砂文本输入框
            TextField {
                id: wifiPasswordField
                Layout.fillWidth: true
                placeholderText: "请输入 Wi-Fi 密码"
                color: "#f4f8fb"
                placeholderTextColor: "#8099a8"
                font.pixelSize: root.fs(14)
                echoMode: showPassword ? TextInput.Normal : TextInput.Password
                selectByMouse: true
                rightPadding: root.dp(40)

                property bool showPassword: false

                background: Rectangle {
                    radius: root.chipRadius
                    color: Qt.rgba(1.0, 1.0, 1.0, 0.06)
                    border.color: wifiPasswordField.activeFocus ? Qt.rgba(0.35, 0.75, 1.0, 0.65) : Qt.rgba(1.0, 1.0, 1.0, 0.10)
                    border.width: 1
                }

                onPressed: {
                    if (!keyboardPopup.visible) {
                        keyboardPopup.open()
                    }
                }
                onActiveFocusChanged: {
                    if (activeFocus && !keyboardPopup.visible) {
                        keyboardPopup.open()
                    }
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: root.dp(8)
                    width: root.dp(28)
                    height: root.dp(28)
                    radius: 14
                    color: passwordVisibilityMouseArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 0.15) : "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: root.dp(18)
                        height: root.dp(18)
                        source: wifiPasswordField.showPassword ? "qrc:/icons/lock-unlocked.svg" : "qrc:/icons/lock-locked.svg"
                        opacity: 0.7
                        sourceSize: Qt.size(width, height)
                    }

                    MouseArea {
                        id: passwordVisibilityMouseArea
                        anchors.fill: parent
                        onClicked: wifiPasswordField.showPassword = !wifiPasswordField.showPassword
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: root.dp(10)

                Item { Layout.fillWidth: true }

                GlassButton {
                    scaleUnit: root.scaleUnit
                    type: "secondary"
                    text: "取消"
                    implicitWidth: root.dp(96)
                    implicitHeight: root.dp(36)
                    onClicked: {
                        keyboardPopup.close()
                        wifiConnectPopup.close()
                    }
                }

                GlassButton {
                    scaleUnit: root.scaleUnit
                    type: "accent"
                    text: "连接"
                    implicitWidth: root.dp(112)
                    implicitHeight: root.dp(36)
                    onClicked: {
                        appController.connectWifi(root.pendingWifiSsid, wifiPasswordField.text)
                        keyboardPopup.close()
                        wifiConnectPopup.close()
                    }
                }
            }
        }
    }

    // 3. 虚拟键盘子弹窗
    Popup {
        id: keyboardPopup
        parent: Overlay.overlay
        modal: false
        focus: true
        width: Math.min((parent ? parent.width : 800) * 0.92, root.dp(520))
        height: Math.min((parent ? parent.height : 480) * 0.92, root.dp(248))
        x: parent ? (parent.width - width) / 2 : 0
        y: (parent ? parent.height : height) - height - root.dp(8)
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: root.panelRadius
            color: Qt.rgba(0.08, 0.13, 0.18, 0.94)
            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.16)
            border.width: 1
            clip: true
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.dp(10)
            spacing: root.dp(6)

            Repeater {
                model: root.keyboardRows()

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: root.dp(6)

                    Repeater {
                        model: modelData

                        Rectangle {
                            implicitWidth: root.keyboardSymbolMode ? root.dp(52) : root.dp(50)
                            implicitHeight: root.dp(40)
                            radius: root.dp(9)
                            color: keyboardKeyArea.pressed
                                   ? Qt.rgba(1.0, 1.0, 1.0, 0.26)
                                   : Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.color: keyboardKeyArea.pressed
                                          ? Qt.rgba(1.0, 1.0, 1.0, 0.40)
                                          : Qt.rgba(1.0, 1.0, 1.0, 0.08)
                            border.width: 1
                            scale: keyboardKeyArea.pressed ? 0.92 : 1.0

                            Behavior on scale { NumberAnimation { duration: 80 } }
                            Behavior on color { ColorAnimation { duration: 90 } }

                            MouseArea {
                                id: keyboardKeyArea
                                anchors.fill: parent
                                onClicked: root.appendPasswordKey(modelData)
                            }

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: "#f4f8fb"
                                font.pixelSize: root.fs(15)
                                font.bold: true
                            }
                        }
                    }
                }
            }

            // 键盘底部功能按键行
            RowLayout {
                Layout.fillWidth: true
                spacing: root.dp(8)

                HeaderActionButton {
                    scaleUnit: root.scaleUnit
                    cornerRadius: root.dp(9)
                    label: root.keyboardSymbolMode ? "ABC" : "#+="
                    activeColor: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                    pressedColor: Qt.rgba(1.0, 1.0, 1.0, 0.22)
                    activeBorderColor: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                    pressedBorderColor: Qt.rgba(1.0, 1.0, 1.0, 0.28)
                    activeTextColor: "#f0f6fa"
                    pressedTextColor: "#ffffff"
                    implicitWidth: root.dp(92)
                    onClicked: {
                        root.keyboardSymbolMode = !root.keyboardSymbolMode
                        root.keyboardShift = false
                    }
                }

                HeaderActionButton {
                    scaleUnit: root.scaleUnit
                    cornerRadius: root.dp(9)
                    label: "大写"
                    activeColor: root.keyboardShift ? Qt.rgba(0.0, 0.48, 1.0, 0.70) : Qt.rgba(1.0, 1.0, 1.0, 0.10)
                    pressedColor: Qt.rgba(0.15, 0.58, 1.0, 0.85)
                    activeBorderColor: root.keyboardShift ? Qt.rgba(0.65, 0.88, 1.0, 0.85) : Qt.rgba(1.0, 1.0, 1.0, 0.12)
                    pressedBorderColor: "#ffffff"
                    activeTextColor: root.keyboardSymbolMode ? "#6a8898" : "#ffffff"
                    pressedTextColor: "#ffffff"
                    implicitWidth: root.dp(92)
                    enabled: !root.keyboardSymbolMode
                    onClicked: root.keyboardShift = !root.keyboardShift
                }

                HeaderActionButton {
                    scaleUnit: root.scaleUnit
                    cornerRadius: root.dp(9)
                    label: "空格"
                    activeColor: Qt.rgba(1.0, 1.0, 1.0, 0.08)
                    pressedColor: Qt.rgba(1.0, 1.0, 1.0, 0.20)
                    activeBorderColor: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                    pressedBorderColor: Qt.rgba(1.0, 1.0, 1.0, 0.25)
                    activeTextColor: "#ebf7fd"
                    pressedTextColor: "#ffffff"
                    implicitWidth: root.dp(214)
                    onClicked: root.appendPasswordKey(" ")
                }

                HeaderActionButton {
                    scaleUnit: root.scaleUnit
                    cornerRadius: root.dp(9)
                    label: "退格"
                    activeColor: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                    pressedColor: Qt.rgba(1.0, 1.0, 1.0, 0.22)
                    activeBorderColor: Qt.rgba(1.0, 1.0, 1.0, 0.12)
                    pressedBorderColor: Qt.rgba(1.0, 1.0, 1.0, 0.28)
                    activeTextColor: "#ffd9df"
                    pressedTextColor: "#ffffff"
                    implicitWidth: root.dp(92)
                    onClicked: {
                        if (wifiPasswordField.length > 0) {
                            wifiPasswordField.remove(wifiPasswordField.length - 1, wifiPasswordField.length)
                        }
                    }
                }

                HeaderActionButton {
                    scaleUnit: root.scaleUnit
                    cornerRadius: root.dp(9)
                    label: "完成"
                    activeColor: Qt.rgba(0.20, 0.70, 0.35, 0.45)
                    pressedColor: Qt.rgba(0.25, 0.80, 0.40, 0.75)
                    activeBorderColor: Qt.rgba(0.40, 0.85, 0.50, 0.65)
                    pressedBorderColor: Qt.rgba(0.60, 1.0, 0.70, 0.85)
                    activeTextColor: "#f0fff4"
                    pressedTextColor: "#ffffff"
                    implicitWidth: root.dp(92)
                    onClicked: keyboardPopup.close()
                }
            }
        }
    }
}
