import QtQuick
import QtQuick.Controls

Item {
    id: root

    // ============================================================
    // 公开属性 (Public API - LiquidGlassTabBar)
    // ============================================================
    property var model: []
    property int currentIndex: 0
    property Item backgroundSource: null
    property color accentColor: "#38bdf8"
    function dp(v) { return (typeof Theme !== "undefined" && Theme) ? Theme.dp(v) : v }
    function fs(v) { return (typeof Theme !== "undefined" && Theme) ? Theme.fs(v) : v }

    property real cornerRadius: root.dp(16)
    property int materialVariant: LiquidGlassSurface.MaterialVariant.Regular

    signal tabSelected(int index, string title)

    implicitWidth: tabRow.implicitWidth + root.dp(16)
    implicitHeight: root.dp(44)

    // 1. 底层液态微晶长条轨道 (Liquid Glass Base Surface)
    LiquidGlassSurface {
        id: baseTrack
        anchors.fill: parent
        backgroundSource: root.backgroundSource
        cornerRadius: root.cornerRadius
        materialVariant: root.materialVariant
        tintColor: Qt.rgba(0.06, 0.12, 0.20, 0.35)
        baseOpacity: 0.38
        highlightIntensity: 0.65
        edgeFresnelPower: 2.4

        // 🫧 次级形状 smin 平滑黏连融合 (将当前 Tab 滑动指示器作为次级形状传入基底着色器)
        secondaryPos: Qt.vector2d(
            indicator.x + indicator.width / 2 - baseTrack.width / 2,
            indicator.y + indicator.height / 2 - baseTrack.height / 2
        )
        secondarySize: Qt.vector2d(indicator.width * 0.46, indicator.height * 0.46)
        secondaryRadius: indicator.radius
        secondaryActive: isAnimating ? 0.90 : 0.60
        sminFactor: isAnimating ? 26.0 : 16.0
    }

    property bool isAnimating: indicatorXAnim.running

    // 2. 滑动液态指示器胶囊 (Liquid Indicator Capsule)
    Rectangle {
        id: indicator
        y: root.dp(4)
        height: parent.height - root.dp(8)
        radius: root.cornerRadius - root.dp(3)
        z: 2

        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.45) }
            GradientStop { position: 1.0; color: Qt.rgba(root.accentColor.r * 0.8, root.accentColor.g * 0.8, root.accentColor.b * 0.8, 0.25) }
        }

        border.width: 1
        border.color: Qt.rgba(1.0, 1.0, 1.0, 0.35)

        // 柔和微光发丝
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            height: Math.max(1, Math.round(parent.height * 0.4))
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.35) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Behavior on x {
            NumberAnimation {
                id: indicatorXAnim
                duration: 260
                easing.type: Easing.OutBack
                easing.overshoot: 1.15
            }
        }

        Behavior on width {
            NumberAnimation {
                duration: 220
                easing.type: Easing.OutCubic
            }
        }
    }

    // 3. Tab 项水平布局 (Tab Items Row)
    Row {
        id: tabRow
        anchors.centerIn: parent
        spacing: root.dp(4)
        z: 10

        Repeater {
            id: tabRepeater
            model: root.model

            delegate: Item {
                id: tabItem
                property bool isSelected: root.currentIndex === index
                implicitWidth: tabLabel.implicitWidth + root.dp(24)
                implicitHeight: root.height - root.dp(8)

                // 首次加载或索引变化时同步指示器位置
                Component.onCompleted: {
                    if (index === root.currentIndex) {
                        updateIndicator(false)
                    }
                }

                Connections {
                    target: root
                    function onCurrentIndexChanged() {
                        if (index === root.currentIndex) {
                            updateIndicator(true)
                        }
                    }
                }

                function updateIndicator(animate) {
                    var targetX = tabItem.x + tabRow.x
                    var targetW = tabItem.width
                    if (!animate) {
                        indicator.x = targetX
                        indicator.width = targetW
                    } else {
                        indicator.x = targetX
                        indicator.width = targetW
                    }
                }

                Text {
                    id: tabLabel
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: root.fs(13)
                    font.bold: tabItem.isSelected
                    color: tabItem.isSelected ? "#ffffff" : "#94a3b8"

                    Behavior on color {
                        ColorAnimation { duration: 160 }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.currentIndex = index
                        root.tabSelected(index, modelData)
                    }
                }
            }
        }
    }
}
