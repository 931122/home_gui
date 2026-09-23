import QtQuick
import HomeGui 1.0

Item {
    id: root

    // ============================================================
    // 公开属性 (Public API - LiquidGlassTabBar)
    // ============================================================
    property var model: []
    property int currentIndex: 0
    property bool scrollable: false
    property Item backgroundSource: null
    property color accentColor: "#38bdf8"
    function dp(v) { return (typeof Theme !== "undefined" && Theme) ? Theme.dp(v) : v }
    function fs(v) { return (typeof Theme !== "undefined" && Theme) ? Theme.fs(v) : v }

    property real cornerRadius: root.dp(16)
    property int materialVariant: LiquidGlassSurface.MaterialVariant.Regular
    property bool indicatorAnimation: true

    signal tabSelected(int index, string title)

    function updateIndicator(animate) {
        var tab = tabRepeater.itemAt(currentIndex)
        if (!tab) return
        indicatorAnimation = animate && (typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled)
        var point = tab.mapToItem(root, 0, 0)
        indicator.x = point.x
        indicator.width = tab.width
    }

    implicitWidth: scrollable ? Math.min(parent ? parent.width : tabRow.implicitWidth + root.dp(16), tabRow.implicitWidth + root.dp(16))
                              : tabRow.implicitWidth + root.dp(16)
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
        secondaryActive: root.isAnimating ? 0.90 : 0.60
        sminFactor: root.isAnimating ? 26.0 : 16.0
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
            enabled: root.indicatorAnimation
            NumberAnimation {
                id: indicatorXAnim
                duration: 260
                easing.type: Easing.OutBack
                easing.overshoot: 1.15
            }
        }

        Behavior on width {
            enabled: root.indicatorAnimation
            NumberAnimation {
                duration: 220
                easing.type: Easing.OutCubic
            }
        }
    }

    // 3. Tab 项水平布局 (Tab Items Row)
    Flickable {
        id: tabViewport
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: root.dp(8)
        anchors.rightMargin: root.dp(8)
        height: parent.height - root.dp(8)
        contentWidth: tabRow.implicitWidth
        contentHeight: height
        interactive: root.scrollable
        clip: root.scrollable
        boundsBehavior: Flickable.StopAtBounds
        z: 10

        Row {
            id: tabRow
            y: 0
            height: parent.height
            spacing: root.dp(4)

            Repeater {
                id: tabRepeater
                model: root.model

            delegate: Item {
                id: tabItem
                required property int index
                required property string modelData
                property bool isSelected: root.currentIndex === index
                implicitWidth: tabLabel.implicitWidth + root.dp(24)
                implicitHeight: root.height - root.dp(8)

                // 首次加载或索引变化时同步指示器位置
                Component.onCompleted: {
                    if (tabItem.index === root.currentIndex) {
                        root.updateIndicator(false)
                    }
                }

                Connections {
                    target: root
                    function onCurrentIndexChanged() {
                        if (tabItem.index === root.currentIndex) {
                            root.updateIndicator(true)
                        }
                    }
                }

                Text {
                    id: tabLabel
                    anchors.centerIn: parent
                    text: tabItem.modelData
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
                        root.currentIndex = tabItem.index
                        root.tabSelected(tabItem.index, tabItem.modelData)
                    }
                }
            }
            }
        }
    }

    Connections {
        target: tabViewport
        function onContentXChanged() {
            root.updateIndicator(false)
        }
    }

    onWidthChanged: root.updateIndicator(false)
}
