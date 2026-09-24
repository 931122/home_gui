import QtQuick

Flickable {
    id: root

    property var model: []
    property int currentIndex: -1
    property var checkedIndices: []
    property bool singleSelection: true
    property bool selectionRequired: false
    property bool singleLine: false
    property real horizontalSpacing: GlassTheme.dp(8)
    property real verticalSpacing: GlassTheme.dp(8)
    property Item backgroundSource: null
    property color accentColor: "#38bdf8"

    signal selectionChanged(int index, var modelData)

    contentWidth: Math.max(width, chipFlow.childrenRect.width)
    contentHeight: chipFlow.implicitHeight
    interactive: singleLine
    boundsBehavior: Flickable.StopAtBounds
    height: singleLine ? Math.max(GlassTheme.dp(32), chipFlow.implicitHeight) : chipFlow.implicitHeight
    clip: singleLine

    Flow {
        id: chipFlow
        width: root.singleLine ? childrenRect.width : root.width
        spacing: root.horizontalSpacing
        flow: Flow.LeftToRight
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled ? 150 : 0
            }
        }

        Repeater {
            model: root.model

        delegate: Item {
            id: chipDelegate
            required property int index
            required property var modelData
            implicitWidth: chip.implicitWidth + root.horizontalSpacing
            implicitHeight: chip.implicitHeight + root.verticalSpacing

            LiquidGlassChip {
                id: chip
                text: typeof chipDelegate.modelData === "string" ? chipDelegate.modelData : chipDelegate.modelData.text
                iconSource: typeof chipDelegate.modelData === "string" ? "" : (chipDelegate.modelData.icon || "")
                checked: root.singleSelection ? root.currentIndex === chipDelegate.index : root.checkedIndices.indexOf(chipDelegate.index) !== -1
                backgroundSource: root.backgroundSource
                accentColor: root.accentColor
                onClicked: {
                    if (root.singleSelection) {
                        if (root.currentIndex === chipDelegate.index && !root.selectionRequired) root.currentIndex = -1
                        else root.currentIndex = chipDelegate.index
                    } else {
                        var next = root.checkedIndices.slice(0)
                        var position = next.indexOf(chipDelegate.index)
                        if (position >= 0) next.splice(position, 1)
                        else next.push(chipDelegate.index)
                        root.checkedIndices = next
                    }
                    root.selectionChanged(chipDelegate.index, chipDelegate.modelData)
                }
            }
        }
        }
    }
}
