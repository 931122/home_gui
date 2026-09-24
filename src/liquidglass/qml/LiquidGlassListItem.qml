import QtQuick
import QtQuick.Layouts

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
    property real groupCornerRadius: GlassTheme.dp(14)
    property real innerCornerRadius: GlassTheme.dp(4)
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
        cornerRadius: root.position === LiquidGlassListItem.Position.Middle
                    ? root.innerCornerRadius
                    : (root.position === LiquidGlassListItem.Position.Single ? root.groupCornerRadius : Math.max(root.innerCornerRadius, root.groupCornerRadius * 0.8))
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
            implicitHeight: root.supportingText.length > 0 ? GlassTheme.dp(68) : GlassTheme.dp(56)
            height: implicitHeight
            spacing: GlassTheme.dp(12)
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: GlassTheme.dp(14)
            anchors.rightMargin: GlassTheme.dp(14)

            Image {
                Layout.preferredWidth: GlassTheme.dp(22)
                Layout.preferredHeight: GlassTheme.dp(22)
                source: root.leadingIcon
                sourceSize: Qt.size(width, height)
                visible: status === Image.Ready
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: GlassTheme.dp(3)

                Text {
                    Layout.fillWidth: true
                    text: root.headline
                    color: root._foreground
                    font.pixelSize: GlassTheme.fs(14)
                    font.bold: true
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: root.supportingText
                    color: root._secondaryForeground
                    font.pixelSize: GlassTheme.fs(11)
                    elide: Text.ElideRight
                    visible: text.length > 0
                }
            }

            Text {
                text: root.trailingText
                color: root._secondaryForeground
                font.pixelSize: GlassTheme.fs(12)
                visible: text.length > 0
            }

            Image {
                Layout.preferredWidth: GlassTheme.dp(18)
                Layout.preferredHeight: GlassTheme.dp(18)
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
