import QtQuick
import HomeGui 1.0

Item {
    id: root

    enum Style { Merged, Separated }

    property int style: LiquidGlassListGroup.Style.Merged
    property real itemSpacing: Theme.dp(8)
    property real cornerRadius: Theme.dp(14)
    property Item backgroundSource: null
    default property alias contentData: contentColumn.data
    implicitWidth: Math.max(280, contentColumn.implicitWidth)
    implicitHeight: contentColumn.implicitHeight
    width: implicitWidth
    height: implicitHeight

    LiquidGlassSurface {
        anchors.fill: parent
        visible: root.style === LiquidGlassListGroup.Style.Merged
        backgroundSource: root.backgroundSource
        cornerRadius: root.cornerRadius
        baseOpacity: 0.40
        tintColor: Qt.rgba(0.82, 0.90, 1.0, 0.18)
        tintStrength: 0.16
        highlightIntensity: 0.48
    }

    Rectangle {
        anchors.fill: parent
        visible: root.style === LiquidGlassListGroup.Style.Merged
        radius: root.cornerRadius
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.13)
    }

    Column {
        id: contentColumn
        anchors.fill: parent
        spacing: root.style === LiquidGlassListGroup.Style.Separated ? root.itemSpacing : 0

        onChildrenChanged: {
            updateItems()
        }

        function updateItems() {
            for (var i = 0; i < children.length; ++i) {
                var child = children[i]
                if (child.configureGlass) {
                    child.position = children.length === 1 ? 0 : (i === 0 ? 1 : (i === children.length - 1 ? 3 : 2))
                    child.configureGlass(root.style === LiquidGlassListGroup.Style.Separated, root.backgroundSource, root.cornerRadius)
                }
            }
        }

        Component.onCompleted: updateItems()
    }

    onStyleChanged: contentColumn.updateItems()
    onBackgroundSourceChanged: contentColumn.updateItems()
    onCornerRadiusChanged: contentColumn.updateItems()
    onWidthChanged: contentColumn.updateItems()
}
