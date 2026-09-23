import QtQuick

Item {
    id: splashRoot
    anchors.fill: parent
    z: 99999

    property bool finished: false

    // 背景深空渐变：与 Linux BootSplash 像素级对齐
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#123044" }
            GradientStop { position: 0.52; color: "#081018" }
            GradientStop { position: 1.0; color: "#030507" }
        }
    }

    // 缩放基准：基于 800x480 的比例系数
    readonly property real scaleRatio: Math.min(splashRoot.width / 800.0, splashRoot.height / 480.0)
    readonly property real logoSize: Math.max(72, Math.round(118 * scaleRatio))
    readonly property real centerY: splashRoot.height / 2 - Math.round(28 * scaleRatio)

    // 中心科技青色径向辉光
    Item {
        id: glowArea
        x: splashRoot.width / 2 - width / 2
        y: splashRoot.centerY - height / 2
        width: splashRoot.logoSize * 2.24
        height: width

        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                var radial = ctx.createRadialGradient(width / 2, height / 2, splashRoot.logoSize * 0.2, width / 2, height / 2, width / 2)
                radial.addColorStop(0.0, "rgba(76, 184, 210, 0.45)")
                radial.addColorStop(0.5, "rgba(76, 184, 210, 0.15)")
                radial.addColorStop(1.0, "rgba(76, 184, 210, 0.0)")
                ctx.fillStyle = radial
                ctx.beginPath()
                ctx.arc(width / 2, height / 2, width / 2, 0, Math.PI * 2)
                ctx.fill()
            }
        }
    }

    // 科技芯片徽章与内部脉冲折线
    Item {
        id: badgeItem
        x: splashRoot.width / 2 - splashRoot.logoSize / 2
        y: splashRoot.centerY - splashRoot.logoSize / 2
        width: splashRoot.logoSize
        height: splashRoot.logoSize

        // 芯片圆角外框
        Rectangle {
            anchors.fill: parent
            radius: splashRoot.logoSize * 0.24
            color: "#102433"
            border.color: "#5bc4d8"
            border.width: Math.max(2, Math.round(2 * splashRoot.scaleRatio))
        }

        // 心跳/脉冲折线（严格还原 Linux bootsplash.cpp 折线坐标）
        Canvas {
            id: lineCanvas
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.strokeStyle = "#f2fbff"
                ctx.lineWidth = Math.max(5, Math.round(7 * splashRoot.scaleRatio))
                ctx.lineCap = "round"
                ctx.lineJoin = "round"

                ctx.beginPath()
                ctx.moveTo(width * 0.28, height * 0.62)
                ctx.lineTo(width * 0.45, height * 0.38)
                ctx.lineTo(width * 0.64, height * 0.62)
                ctx.lineTo(width * 0.78, height * 0.42)
                ctx.stroke()
            }
        }
    }

    // 大标题 "HOME GUI"
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: splashRoot.centerY + splashRoot.logoSize / 2 + Math.round(22 * splashRoot.scaleRatio)
        text: "HOME GUI"
        color: "#eef8fb"
        font.pixelSize: Math.max(28, Math.round(38 * splashRoot.scaleRatio))
        font.bold: true
        font.letterSpacing: 2
    }

    // 副标题 "正在启动智能中控"
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: splashRoot.centerY + splashRoot.logoSize / 2 + Math.round(68 * splashRoot.scaleRatio)
        text: qsTr("正在启动智能中控")
        color: "#8aa4b3"
        font.pixelSize: Math.max(14, Math.round(17 * splashRoot.scaleRatio))
        font.letterSpacing: 1
    }

    // 点击可加速跳过启动屏
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (!fadeOutAnim.running && !splashRoot.finished) {
                dismissTimer.stop()
                fadeOutAnim.start()
            }
        }
    }

    // 与 Linux 端 minimumVisibleElapsed(900) 严格一致的展示时长
    Timer {
        id: dismissTimer
        interval: 880
        running: true
        repeat: false
        onTriggered: {
            fadeOutAnim.start()
        }
    }

    // 平滑淡出至主界面
    NumberAnimation {
        id: fadeOutAnim
        target: splashRoot
        property: "opacity"
        to: 0.0
        duration: 320
        easing.type: Easing.InOutQuad
        onFinished: {
            splashRoot.finished = true
            splashRoot.visible = false
        }
    }
}
