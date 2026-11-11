import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HomeGui 1.0
import HomeGui.LiquidGlass 1.0

GlassPopup {
    id: root
    title: qsTr("小米通行证登录")

    property real scaleUnit: Theme.scaleUnit
    property int cardRadius: Theme.radiusCard
    property int chipRadius: Theme.radiusChip
    readonly property bool isPortrait: Overlay.overlay && Overlay.overlay.width < Overlay.overlay.height

    width: Math.min((Overlay.overlay ? Overlay.overlay.width : 800) * 0.94, Theme.dp(isPortrait ? 380 : 540))
    height: Math.min((Overlay.overlay ? Overlay.overlay.height : 480) * 0.92, Theme.dp(isPortrait ? 520 : 380))

    function dp(val) { return Theme.dp(val) }
    function fs(val) { return Theme.fs(val) }

    onOpened: {
        console.log("[IntercomLoginPopup] Opened! Calling startIntercomQrLogin...")
        if (typeof appController !== "undefined") {
            appController.startIntercomQrLogin()
        }
    }

    onClosed: {
        console.log("[IntercomLoginPopup] Closed! Calling cancelIntercomQrLogin...")
        if (typeof appController !== "undefined") {
            appController.cancelIntercomQrLogin()
        }
    }

    Connections {
        target: (typeof appController !== "undefined") ? appController : null
        function onIntercomLoginResult(success, message) {
            if (success) {
                closeTimer.start()
            }
        }
    }

    Timer {
        id: closeTimer
        interval: 1200
        repeat: false
        onTriggered: root.close()
    }

    // 弹窗主内容区
    Item {
        anchors.fill: parent
        anchors.margins: root.dp(16)

        // 桌面横屏布局 (左右双栏)
        RowLayout {
            anchors.fill: parent
            spacing: root.dp(16)
            visible: !root.isPortrait

            // 左侧：二维码卡片
            Rectangle {
                Layout.preferredWidth: root.dp(180)
                Layout.fillHeight: true
                radius: root.cardRadius
                color: Qt.rgba(1.0, 1.0, 1.0, 0.04)
                border.color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.dp(12)
                    spacing: root.dp(8)

                    Item { Layout.fillHeight: true }

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: root.dp(136)
                        height: root.dp(136)
                        radius: root.dp(10)
                        color: "#ffffff"
                        clip: true

                        Image {
                            id: landscapeQrImg
                            anchors.fill: parent
                            anchors.margins: root.dp(6)
                            fillMode: Image.PreserveAspectFit
                            source: (typeof appController !== "undefined") ? appController.intercomQrImageUrl : ""
                            visible: status === Image.Ready
                        }

                        Column {
                            anchors.centerIn: parent
                            spacing: root.dp(6)
                            visible: landscapeQrImg.status !== Image.Ready

                            BusyIndicator {
                                anchors.horizontalCenter: parent.horizontalCenter
                                running: true
                                implicitWidth: root.dp(32)
                                implicitHeight: root.dp(32)
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: qsTr("生成二维码中...")
                                color: "#64748b"
                                font.pixelSize: root.fs(10)
                            }
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("小米手机 / 米家 App 扫码")
                        color: "#94a3b8"
                        font.pixelSize: root.fs(11)
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // 右侧：浏览器登录与提示卡片
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.dp(10)

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: root.cardRadius
                    color: Qt.rgba(0.20, 0.60, 1.0, 0.08)
                    border.color: Qt.rgba(0.40, 0.75, 1.0, 0.20)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(14)
                        spacing: root.dp(10)

                        RowLayout {
                            spacing: root.dp(8)
                            Rectangle {
                                width: root.dp(24)
                                height: root.dp(24)
                                radius: root.dp(6)
                                color: Qt.rgba(0.22, 0.65, 1.0, 0.25)
                                Text {
                                    anchors.centerIn: parent
                                    text: "🌐"
                                    font.pixelSize: root.fs(12)
                                }
                            }
                            Text {
                                text: qsTr("如遇图形滑块或两步验证")
                                color: "#f0f6fa"
                                font.pixelSize: root.fs(14)
                                font.bold: true
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("点击下方按钮直接在系统浏览器中打开小米通行证页面，支持完整的图形滑块验证、短信验证码或一键快捷授权。登录成功后本应用将自动检测并获取凭证。")
                            color: "#94a3b8"
                            font.pixelSize: root.fs(11)
                            wrapMode: Text.WordWrap
                        }

                        GlassButton {
                            Layout.fillWidth: true
                            scaleUnit: root.scaleUnit
                            styleType: "primary"
                            implicitHeight: root.dp(38)
                            text: qsTr("🌐 在浏览器中打开登录 (推荐)")
                            textPixelSize: root.fs(12)
                            onClicked: {
                                if (typeof appController !== "undefined" && appController.intercomQrWebUrl) {
                                    Qt.openUrlExternally(appController.intercomQrWebUrl)
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }

                        // 状态显示
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: root.dp(30)
                            radius: root.dp(6)
                            color: Qt.rgba(0.0, 0.0, 0.0, 0.25)

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: root.dp(10)
                                anchors.rightMargin: root.dp(10)
                                spacing: root.dp(8)

                                Rectangle {
                                    width: root.dp(8)
                                    height: root.dp(8)
                                    radius: 4
                                    color: (typeof appController !== "undefined" && appController.intercomQrLoggingIn) ? "#38bdf8" : "#4ade80"
                                    SequentialAnimation on opacity {
                                        running: (typeof appController !== "undefined" && appController.intercomQrLoggingIn)
                                        loops: Animation.Infinite
                                        NumberAnimation { to: 0.3; duration: 600 }
                                        NumberAnimation { to: 1.0; duration: 600 }
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: (typeof appController !== "undefined" && appController.intercomQrLoginStatus)
                                          ? appController.intercomQrLoginStatus
                                          : qsTr("等待扫码或浏览器授权...")
                                    color: "#f8fafc"
                                    font.pixelSize: root.fs(11)
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                // 底部操作按钮
                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.dp(8)

                    Item { Layout.fillWidth: true }

                    GlassButton {
                        scaleUnit: root.scaleUnit
                        styleType: "neutral"
                        implicitWidth: root.dp(90)
                        implicitHeight: root.dp(32)
                        text: qsTr("刷新二维码")
                        textPixelSize: root.fs(11)
                        onClicked: {
                            if (typeof appController !== "undefined") {
                                appController.startIntercomQrLogin()
                            }
                        }
                    }

                    GlassButton {
                        scaleUnit: root.scaleUnit
                        styleType: "neutral"
                        implicitWidth: root.dp(70)
                        implicitHeight: root.dp(32)
                        text: qsTr("取消")
                        textPixelSize: root.fs(11)
                        onClicked: root.close()
                    }
                }
            }
        }

        // 移动端竖屏布局 (垂直单栏)
        Flickable {
            anchors.fill: parent
            visible: root.isPortrait
            contentWidth: width
            contentHeight: portraitCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.DragAndOvershootBounds

            ColumnLayout {
                id: portraitCol
                width: parent.width
                spacing: root.dp(12)

                // 顶部二维码卡片
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: root.dp(176)
                    radius: root.cardRadius
                    color: Qt.rgba(1.0, 1.0, 1.0, 0.04)
                    border.color: Qt.rgba(1.0, 1.0, 1.0, 0.10)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: root.dp(10)
                        spacing: root.dp(6)

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: root.dp(120)
                            height: root.dp(120)
                            radius: root.dp(10)
                            color: "#ffffff"
                            clip: true

                            Image {
                                id: portraitQrImg
                                anchors.fill: parent
                                anchors.margins: root.dp(6)
                                fillMode: Image.PreserveAspectFit
                                source: (typeof appController !== "undefined") ? appController.intercomQrImageUrl : ""
                                visible: status === Image.Ready
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: root.dp(4)
                                visible: portraitQrImg.status !== Image.Ready

                                BusyIndicator {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    running: true
                                    implicitWidth: root.dp(28)
                                    implicitHeight: root.dp(28)
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: qsTr("生成二维码中...")
                                    color: "#64748b"
                                    font.pixelSize: root.fs(10)
                                }
                            }
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("小米手机 / 米家 App 扫码")
                            color: "#94a3b8"
                            font.pixelSize: root.fs(11)
                        }
                    }
                }

                // 浏览器登录卡片
                Rectangle {
                    Layout.fillWidth: true
                    radius: root.cardRadius
                    color: Qt.rgba(0.20, 0.60, 1.0, 0.08)
                    border.color: Qt.rgba(0.40, 0.75, 1.0, 0.20)
                    border.width: 1
                    implicitHeight: portraitBrowserCol.implicitHeight + root.dp(24)

                    ColumnLayout {
                        id: portraitBrowserCol
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: root.dp(12)
                        spacing: root.dp(8)

                        Text {
                            text: qsTr("遇到图形滑块或两步验证？")
                            color: "#f0f6fa"
                            font.pixelSize: root.fs(13)
                            font.bold: true
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("点击下方按钮在系统浏览器中完成图形滑块或短信验证登录，完成后应用将自动识别凭证。")
                            color: "#94a3b8"
                            font.pixelSize: root.fs(11)
                            wrapMode: Text.WordWrap
                        }

                        GlassButton {
                            Layout.fillWidth: true
                            scaleUnit: root.scaleUnit
                            styleType: "primary"
                            implicitHeight: root.dp(38)
                            text: qsTr("🌐 在浏览器中打开登录 (推荐)")
                            textPixelSize: root.fs(12)
                            onClicked: {
                                if (typeof appController !== "undefined" && appController.intercomQrWebUrl) {
                                    Qt.openUrlExternally(appController.intercomQrWebUrl)
                                }
                            }
                        }
                    }
                }

                // 状态条
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: root.dp(32)
                    radius: root.dp(6)
                    color: Qt.rgba(0.0, 0.0, 0.0, 0.25)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: root.dp(10)
                        anchors.rightMargin: root.dp(10)
                        spacing: root.dp(8)

                        Rectangle {
                            width: root.dp(8)
                            height: root.dp(8)
                            radius: 4
                            color: (typeof appController !== "undefined" && appController.intercomQrLoggingIn) ? "#38bdf8" : "#4ade80"
                            SequentialAnimation on opacity {
                                running: (typeof appController !== "undefined" && appController.intercomQrLoggingIn)
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.3; duration: 600 }
                                NumberAnimation { to: 1.0; duration: 600 }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: (typeof appController !== "undefined" && appController.intercomQrLoginStatus)
                                  ? appController.intercomQrLoginStatus
                                  : qsTr("等待扫码或浏览器授权...")
                            color: "#f8fafc"
                            font.pixelSize: root.fs(11)
                            elide: Text.ElideRight
                        }
                    }
                }

                // 底部操作按钮
                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.dp(8)

                    GlassButton {
                        Layout.fillWidth: true
                        scaleUnit: root.scaleUnit
                        styleType: "neutral"
                        implicitHeight: root.dp(34)
                        text: qsTr("刷新二维码")
                        textPixelSize: root.fs(11)
                        onClicked: {
                            if (typeof appController !== "undefined") {
                                appController.startIntercomQrLogin()
                            }
                        }
                    }

                    GlassButton {
                        Layout.fillWidth: true
                        scaleUnit: root.scaleUnit
                        styleType: "neutral"
                        implicitHeight: root.dp(34)
                        text: qsTr("取消")
                        textPixelSize: root.fs(11)
                        onClicked: root.close()
                    }
                }
            }
        }
    }
}
