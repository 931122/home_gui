import QtQuick
import QtQuick.Layouts
import HomeGui 1.0

Item {
    id: root

    enum Position { Single, First, Middle, Last }

    property string headline: ""
    property string supportingText: ""
    property string leadingIcon: ""
    property string trailingText: ""
    property string trailingIcon: ""
    property Item backgroundSource: null
    property int position: LiquidGlassListItem.Position.Single
    property real groupCornerRadius: Theme.dp(14)
    property real innerCornerRadius: Theme.dp(4)
    property bool expanded: false
    property Component expandedContent: null
    property bool glassEnabled: true
    readonly property bool hasCustomContent: customContent.children.length > 0
    default property alias contentData: customContent.data

    signal clicked()
    signal expandedChangedByUser(bool expanded)

    function configureGlass(enabled, source, radius) {
        glassEnabled = enabled
        backgroundSource = source
        groupCornerRadius = radius
    }

    implicitHeight: contentColumn.implicitHeight
    implicitWidth: 280
    width: parent ? parent.width : implicitWidth
    height: implicitHeight

    LiquidGlassSurface {
        anchors.fill: parent
        visible: root.glassEnabled
        backgroundSource: root.backgroundSource
        cornerRadius: root.groupCornerRadius
        baseOpacity: 0.34
        tintColor: Qt.rgba(0.82, 0.90, 1.0, 0.18)
        tintStrength: 0.16
        highlightIntensity: 0.48
    }

    Column {
        id: contentColumn
        anchors.fill: parent

        RowLayout {
            id: baseRow
            visible: !root.hasCustomContent
            Layout.fillWidth: true
            implicitHeight: root.supportingText.length > 0 ? Theme.dp(68) : Theme.dp(56)
            height: implicitHeight
            spacing: Theme.dp(12)
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.dp(14)
            anchors.rightMargin: Theme.dp(14)

            Image {
                Layout.preferredWidth: Theme.dp(22)
                Layout.preferredHeight: Theme.dp(22)
                source: root.leadingIcon
                sourceSize: Qt.size(width, height)
                visible: status === Image.Ready
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.dp(3)

                Text {
                    Layout.fillWidth: true
                    text: root.headline
                    color: root._foreground
                    font.pixelSize: Theme.fs(14)
                    font.bold: true
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: root.supportingText
                    color: root._secondaryForeground
                    font.pixelSize: Theme.fs(11)
                    elide: Text.ElideRight
                    visible: text.length > 0
                }
            }

            Text {
                text: root.trailingText
                color: root._secondaryForeground
                font.pixelSize: Theme.fs(12)
                visible: text.length > 0
            }

            Image {
                Layout.preferredWidth: Theme.dp(18)
                Layout.preferredHeight: Theme.dp(18)
                source: root.trailingIcon
                sourceSize: Qt.size(width, height)
                visible: status === Image.Ready
            }
        }

        Loader {
            id: expandedItem
            width: parent.width
            implicitHeight: item ? item.implicitHeight : 0
            height: active ? implicitHeight : 0
            active: root.expanded
            sourceComponent: root.expandedContent
        }
        Item { id: customContent; width: parent.width; implicitHeight: childrenRect.height }
    }

    readonly property color _foreground: (typeof glassRuntime !== "undefined" && glassRuntime.backdropLuminance > 0.55) ? "#111827" : "#f8fafc"
    readonly property color _secondaryForeground: (typeof glassRuntime !== "undefined" && glassRuntime.backdropLuminance > 0.55) ? "#475569" : "#b7c4d3"

    TapHandler {
        onTapped: {
            root.clicked()
            if (root.expandedContent) {
                root.expanded = !root.expanded
                root.expandedChangedByUser(root.expanded)
            }
        }
    }
}
