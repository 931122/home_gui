import QtQuick

Item {
    id: root

    // ============================================================
    // 公开属性 (Public API - Faithful to QWEA0/Liquid-Glass-Android)
    // ============================================================
    enum TabMode {
        ModeFixed = 0,      // 等宽平分（分段控制器标准模式，适用于卡片与弹窗）
        ModeScrollable = 1  // 内容自适应宽度（可横向滑动模式）
    }

    property var model: []
    property int currentIndex: 0
    property var currentValue: undefined
    property int tabMode: LiquidGlassTabBar.TabMode.ModeFixed
    // `scrollable` is retained for callers of the original API.  `tabMode`
    // remains the preferred API, and either value enables the scroll layout.
    property bool scrollable: false
    readonly property bool _isScrollable: scrollable || tabMode === LiquidGlassTabBar.TabMode.ModeScrollable

    property Item backgroundSource: typeof glassRuntime !== "undefined" ? glassRuntime.backdropSource : null
    property color accentColor: "#38bdf8"
    property var selectedTintColor: null // 选中项文字/图标高亮色，null 则使用高对比白/黑

    function dp(v) { return (typeof GlassTheme !== "undefined" && GlassTheme) ? GlassTheme.dp(v) : v }
    function fs(v) { return (typeof GlassTheme !== "undefined" && GlassTheme) ? GlassTheme.fs(v) : v }

    property real cornerRadius: height / 2
    property int materialVariant: LiquidGlassSurface.MaterialVariant.Regular

    // 💧 玻璃滴透镜光学参数 (对齐 Liquid-Glass-Android: bevel=7dp, refraction=4dp, dispersion=0.04)
    property real dropletBevelWidth: root.dp(7)
    property real dropletRefractionHeight: root.dp(4)
    property real dropletDispersion: 0.04
    property bool dropletBackdropBlur: false // 保持清透，不模糊底图

    // 手势与动画开关
    property bool interactive: true
    property bool indicatorAnimation: true

    signal tabSelected(int index, var data)

    implicitWidth: _isScrollable
                   ? (tabRow.implicitWidth + root.dp(16))
                   : root.dp(240)
    implicitHeight: root.dp(40)

    // ============================================================
    // 模型解析与辅助函数
    // ============================================================
    function _getModelCount() {
        if (!model) return 0
        if (typeof model.count === "number") return model.count
        if (typeof model.length === "number") return model.length
        return 0
    }

    function _getItemAt(index) {
        if (!model || index < 0 || index >= _getModelCount()) return null
        if (typeof model.get === "function") return model.get(index)
        return model[index]
    }

    function _getItemTitle(item) {
        if (item === null || item === undefined) return ""
        if (typeof item === "string" || typeof item === "number") return String(item)
        if (typeof item === "object") {
            if (item.title !== undefined) return String(item.title)
            if (item.label !== undefined) return String(item.label)
            if (item.name !== undefined) return String(item.name)
            if (item.text !== undefined) return String(item.text)
        }
        return ""
    }

    function _getItemValue(item) {
        if (item === null || item === undefined) return null
        if (typeof item === "string" || typeof item === "number") return item
        if (typeof item === "object") {
            if (item.value !== undefined) return item.value
            if (item.val !== undefined) return item.val
            if (item.key !== undefined) return item.key
            if (item.sec !== undefined) return item.sec
            if (item.title !== undefined) return item.title
            if (item.label !== undefined) return item.label
            if (item.name !== undefined) return item.name
        }
        return item
    }

    function _getItemIcon(item) {
        if (item && typeof item === "object") {
            if (item.icon !== undefined) return String(item.icon)
            if (item.iconSource !== undefined) return String(item.iconSource)
        }
        return ""
    }

    // ============================================================
    // 外部值只驱动选中态，选择信号仅由用户交互发出
    // ============================================================
    property bool _suppressIndexAnimation: false

    function syncCurrentIndexFromValue() {
        if (currentValue === undefined || currentValue === null) return false
        var count = _getModelCount()
        for (var i = 0; i < count; ++i) {
            var item = _getItemAt(i)
            if (_getItemValue(item) === currentValue || _getItemTitle(item) === String(currentValue)) {
                if (currentIndex !== i) currentIndex = i
                return true
            }
        }
        return false
    }

    onCurrentIndexChanged: {
        if (!_suppressIndexAnimation) animateDropletTo(currentIndex)
    }

    onCurrentValueChanged: syncCurrentIndexFromValue()

    onModelChanged: {
        Qt.callLater(function() {
            var previousIndex = root.currentIndex
            if (root.syncCurrentIndexFromValue()) {
                if (root.currentIndex === previousIndex) root.syncDroplet()
                return
            }
            var count = root._getModelCount()
            if (count > 0 && root.currentIndex >= count) {
                root.currentIndex = count - 1
            } else {
                root.syncDroplet()
            }
        })
    }

    // ============================================================
    // 1. 底层视觉内容：包含底轨与全部标签文字/图标 (Tab Bar Background Content)
    // 完整作为底层渲染，并供上层透镜进行实时物理折射、微放大与色散采样
    // ============================================================
    Item {
        id: tabBarContent
        anchors.fill: parent
        z: 1

        // 1.1 底层液态微晶长条轨道
        LiquidGlassSurface {
            id: baseTrack
            anchors.fill: parent
            backgroundSource: root.backgroundSource
            cornerRadius: root.cornerRadius
            materialVariant: root.materialVariant
            tintColor: Qt.rgba(0.06, 0.10, 0.18, 0.42)
            baseOpacity: 0.38
            highlightIntensity: 0.65
            edgeFresnelPower: 2.4

            // 🫧 次级形状 smin 平滑黏连融合 (Metaball 双形状液态张力粘连)
            secondaryPos: Qt.vector2d(
                droplet.x + droplet.width / 2 - baseTrack.width / 2,
                droplet.y + droplet.height / 2 - baseTrack.height / 2
            )
            secondarySize: Qt.vector2d(droplet.width * 0.46, droplet.height * 0.46)
            secondaryRadius: droplet.cornerRadius
            secondaryActive: (settleAnim.running || root.dragging) ? 0.95 : 0.60
            sminFactor: (settleAnim.running || root.dragging) ? 26.0 : 16.0
        }

        // 1.2 标签内容排布 (Tabs Row: Icon + Label)
        Flickable {
            id: tabViewport
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: root.dp(4)
            contentWidth: tabRow.implicitWidth
            contentHeight: height
            interactive: root.interactive && root._isScrollable
            clip: root._isScrollable
            boundsBehavior: Flickable.StopAtBounds
            z: 2

            Row {
                id: tabRow
                y: 0
                height: parent.height
                spacing: root.dp(2)

                Repeater {
                    id: tabRepeater
                    model: root.model

                    delegate: Item {
                        id: tabItem
                        required property int index
                        required property var modelData
                        readonly property bool isSelected: root.currentIndex === index
                        readonly property string itemTitle: root._getItemTitle(modelData)
                        readonly property string itemIcon: root._getItemIcon(modelData)

                        // 固定模式下等宽平分；横滑模式下自适应内容宽度
                        width: !root._isScrollable
                               ? Math.max(1, (tabViewport.width - tabRow.spacing * (Math.max(1, tabRepeater.count) - 1)) / Math.max(1, tabRepeater.count))
                               : (contentLayout.implicitWidth + root.dp(20))
                        height: tabViewport.height

                        Component.onCompleted: {
                            if (tabItem.index === root.currentIndex) {
                                Qt.callLater(function() { root.syncDroplet() })
                            }
                        }

                        Row {
                            id: contentLayout
                            anchors.centerIn: parent
                            spacing: root.dp(4)

                            Image {
                                id: tabIcon
                                width: root.dp(16)
                                height: root.dp(16)
                                anchors.verticalCenter: parent.verticalCenter
                                source: tabItem.itemIcon
                                sourceSize: Qt.size(width, height)
                                smooth: true
                                visible: tabItem.itemIcon !== "" && status === Image.Ready
                            }

                            Text {
                                id: tabLabel
                                anchors.verticalCenter: parent.verticalCenter
                                text: tabItem.itemTitle
                                font.pixelSize: root.fs(12)
                                font.bold: tabItem.isSelected
                                color: tabItem.isSelected
                                       ? (root.selectedTintColor ? root.selectedTintColor : "#ffffff")
                                       : (baseTrack.isDarkBackground ? "#94a3b8" : "#475569")

                                Behavior on color {
                                    ColorAnimation { duration: 160 }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: root.interactive && root._isScrollable
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.selectTab(tabItem.index, true)
                        }
                    }
                }
            }
        }
    }

    // ============================================================
    // 2. 实时离屏背景捕获源 (ShaderEffectSource)
    // 捕获整条包含底轨与文字的 TabBar，供液态透镜进行实时折射与色散采样
    // ============================================================
    ShaderEffectSource {
        id: tabBarCapture
        sourceItem: tabBarContent
        anchors.fill: parent
        live: true
        smooth: true
        hideSource: false
        visible: false
        z: 2
    }

    // ============================================================
    // 3. 真实液态玻璃滴透镜 (True Liquid Glass Droplet Lens)
    // 叠在文字与底轨上方 (z: 10)，执行真实 SDF 光学折射、向心压缩环与凸透镜微放大
    // 对齐 LiquidGlassTabBar.kt:
    // - Clear 冰晶材质、无模糊 (enableBackdropBlur = false) 纯折射放大镜
    // - 窄斜面 (8dp)、浅折射 (6dp)、物理光谱色散 (0.04)
    // ============================================================
    Item {
        id: dropletContainer
        anchors.fill: parent
        z: 10

        Item {
            id: droplet
            x: root.dp(4)
            y: root.dp(4)
            width: root.dp(60)
            height: Math.max(1, parent.height - root.dp(8))
            property real cornerRadius: height / 2

            transform: Scale {
                id: dropletScale
                origin.x: droplet.width / 2
                origin.y: droplet.height / 2
                xScale: 1.0
                yScale: 1.0
            }

            // 玻璃滴本体软投影
            Rectangle {
                anchors.fill: parent
                anchors.topMargin: root.dp(2)
                anchors.bottomMargin: -root.dp(2)
                radius: droplet.cornerRadius
                color: Qt.rgba(0, 0, 0, 0.28)
                opacity: root.dragging ? 0.45 : 0.25
                z: 0
            }

            // 真实物理折射透镜 ShaderEffect (True Liquid Glass Refraction Lens)
            ShaderEffect {
                id: lensEffect
                anchors.fill: parent
                z: 1

                property var source: tabBarCapture
                property real bevel: root.dropletBevelWidth
                property real _curW: droplet.width * dropletScale.xScale
                property real _curH: droplet.height * dropletScale.yScale
                property real _curX: droplet.x + (droplet.width - _curW) * 0.5
                property real _curY: droplet.y + (droplet.height - _curH) * 0.5
                property vector2d dropletPos: Qt.vector2d(_curX, _curY)
                property vector2d dropletSize: Qt.vector2d(Math.max(1, _curW), Math.max(1, _curH))
                property vector2d contentSize: Qt.vector2d(Math.max(1, root.width), Math.max(1, root.height))
                property vector2d tilt: (typeof glassRuntime !== "undefined")
                                       ? Qt.vector2d(glassRuntime.tilt.x, glassRuntime.tilt.y)
                                       : Qt.vector2d(0, 0)
                property real refractPx: root.dropletRefractionHeight
                property real dispersion: root.dropletDispersion
                property color accentColor: {
                    var base = root.selectedTintColor ? root.selectedTintColor : root.accentColor
                    return Qt.rgba(base.r, base.g, base.b, 0.22)
                }

                fragmentShader: "qrc:/qt/qml/HomeGui/LiquidGlass/shaders/liquid_glass_tab_lens.frag.qsb"
            }
        }
    }

    // ============================================================
    // 4. 动力学与动画 (Kinematics & Interpolation)
    // 严格复刻 LiquidGlassTabBar.kt:
    // - 距离越远液态拉伸越明显: stretch = 0.22 * min(1, abs(dist)/(w*3))
    // - 等体积压缩: sx = 1 + stretch*sin(PI*t), sy = 1 - stretch*0.55*sin(PI*t)
    // - 过冲插值器 OvershootInterpolator(1.1f): 380ms 弹性滑行
    // - clampDropletX 边缘钳制防出界
    // ============================================================
    property bool isAnimating: settleAnim.running
    property real animProgress: 0.0
    property real animStartX: 0.0
    property real animTargetX: 0.0
    property real animDist: 0.0
    property real animStretch: 0.0

    function _overshoot(t) {
        var tension = 1.1
        var p = t - 1.0
        return (tension + 1.0) * p * p * p + tension * p * p + 1.0
    }

    NumberAnimation {
        id: settleAnim
        target: root
        property: "animProgress"
        from: 0.0
        to: 1.0
        duration: (typeof glassRuntime === "undefined" || glassRuntime.animationsEnabled) ? 380 : 0
        easing.type: Easing.Linear

        onRunningChanged: {
            if (!running) {
                dropletScale.xScale = 1.0
                dropletScale.yScale = 1.0
                root.syncDroplet()
            }
        }
    }

    onAnimProgressChanged: {
        if (!settleAnim.running) return
        var t = animProgress
        var s = Math.sin(Math.PI * Math.min(t * 1.15, 1.0))
        var sx = 1.0 + animStretch * s
        var sy = 1.0 - animStretch * 0.55 * s
        dropletScale.xScale = sx
        dropletScale.yScale = sy

        var interpX = animStartX + animDist * _overshoot(t)
        droplet.x = clampDropletX(interpX, sx)
    }

    function getTabRect(index) {
        var count = _getModelCount()
        if (count <= 0) return Qt.rect(root.dp(4), root.dp(4), root.dp(60), Math.max(1, root.height - root.dp(8)))
        if (!root._isScrollable) {
            var availW = tabViewport.width
            var spacing = tabRow.spacing
            var itemW = Math.max(1, (availW - spacing * (count - 1)) / count)
            var itemH = tabViewport.height
            var itemX = tabViewport.x + index * (itemW + spacing)
            var itemY = tabViewport.y
            return Qt.rect(itemX, itemY, itemW, itemH)
        } else {
            var tab = tabRepeater.itemAt(index)
            if (tab && tab.width > 0) {
                var pt = tab.mapToItem(root, 0, 0)
                return Qt.rect(pt.x, pt.y, tab.width, tab.height)
            }
            return Qt.rect(root.dp(4), root.dp(4), root.dp(60), Math.max(1, root.height - root.dp(8)))
        }
    }

    function clampDropletX(x, scaleX) {
        var count = _getModelCount()
        var firstR = getTabRect(0)
        var lastR = getTabRect(count > 0 ? count - 1 : 0)
        var minBound = firstR.x
        var maxBound = lastR.x + lastR.width
        var w = droplet.width
        var bulge = (scaleX - 1.0) * w / 2.0
        var minX = minBound + bulge
        var maxX = maxBound - w - bulge
        return (minX <= maxX) ? Math.max(minX, Math.min(maxX, x)) : (minBound + (maxBound - minBound - w) / 2.0)
    }

    function syncDroplet() {
        var r = getTabRect(currentIndex)
        droplet.x = r.x
        droplet.y = r.y
        droplet.width = r.width
        droplet.height = r.height
        dropletScale.xScale = 1.0
        dropletScale.yScale = 1.0
    }

    function animateDropletTo(index) {
        if (!indicatorAnimation) {
            syncDroplet()
            return
        }
        var r = getTabRect(index)
        droplet.width = r.width
        droplet.height = r.height

        settleAnim.stop()
        animStartX = droplet.x
        animTargetX = r.x
        animDist = animTargetX - animStartX

        if (Math.abs(animDist) < 0.5) {
            syncDroplet()
            return
        }

        // 液体体积拉伸率：跨距越远拉伸越显著 (上限 22%)
        animStretch = 0.22 * Math.min(1.0, Math.abs(animDist) / (r.width * 3.0))
        settleAnim.start()
    }

    function selectTab(index, animate) {
        var count = _getModelCount()
        if (count <= 0) return
        var clamped = Math.max(0, Math.min(count - 1, index))
        var changed = (clamped !== currentIndex)

        if (changed) {
            if (animate === false) {
                _suppressIndexAnimation = true
                currentIndex = clamped
                _suppressIndexAnimation = false
                syncDroplet()
            } else {
                currentIndex = clamped
            }
        } else if (animate === false) {
            syncDroplet()
        } else if (animate !== false) {
            // A drag can finish on the current tab.  Still animate the
            // droplet back to its tab instead of leaving it under the finger.
            root.animateDropletTo(currentIndex)
        }
        if (changed) {
            var it = _getItemAt(clamped)
            root.tabSelected(clamped, _getItemValue(it))
        }
    }

    // ============================================================
    // 5. 触摸手势：点击选择 + 拖拽玻璃滴 (Touch & Drag Gestures)
    // 严格复刻 LiquidGlassTabBar.kt:
    // - 触摸位移超出 touchSlop (~8dp) 进入拖拽
    // - 拖拽时玻璃滴微微鼓起 1.06x 展现水滴张力
    // - 松手按中心点吸附到最近标签 (nearestTabIndex)
    // ============================================================
    property bool dragging: false

    function dragDropletTo(mouseX) {
        var local = mouseX - droplet.width / 2.0
        droplet.x = clampDropletX(local, dropletScale.xScale)
    }

    function nearestTabIndex() {
        var count = _getModelCount()
        if (count <= 0) return 0
        var dropletCenter = droplet.x + droplet.width / 2.0
        var best = currentIndex
        var bestDist = 999999
        for (var i = 0; i < count; ++i) {
            var r = getTabRect(i)
            var tabCenter = r.x + r.width / 2.0
            var d = Math.abs(tabCenter - dropletCenter)
            if (d < bestDist) {
                bestDist = d
                best = i
            }
        }
        return best
    }

    function tabIndexAt(mouseX) {
        var count = _getModelCount()
        if (count <= 0) return 0
        for (var i = 0; i < count; ++i) {
            var r = getTabRect(i)
            if (mouseX < r.x + r.width) {
                return i
            }
        }
        return Math.max(0, count - 1)
    }

    MouseArea {
        id: gestureArea
        anchors.fill: parent
        z: 20
        preventStealing: true
        enabled: root.interactive && !root._isScrollable
        cursorShape: Qt.PointingHandCursor
        property real downX: 0
        property real touchSlop: root.dp(8)

        onPressed: (mouse) => {
            downX = mouse.x
            root.dragging = false
        }

        onPositionChanged: (mouse) => {
            if (!root.dragging && Math.abs(mouse.x - downX) > touchSlop) {
                root.dragging = true
                settleAnim.stop()
                // 按住拖拽时玻璃滴鼓起 1.06x
                dropletScale.xScale = 1.06
                dropletScale.yScale = 1.06
            }
            if (root.dragging) {
                root.dragDropletTo(mouse.x)
            }
        }

        onReleased: (mouse) => {
            if (root.dragging) {
                root.dragging = false
                root.selectTab(root.nearestTabIndex(), true)
            } else {
                root.selectTab(root.tabIndexAt(mouse.x), true)
            }
        }

        onCanceled: {
            root.dragging = false
            root.animateDropletTo(root.currentIndex)
        }
    }

    Connections {
        target: tabViewport
        function onContentXChanged() {
            if (!root.dragging) root.syncDroplet()
        }
    }

    onWidthChanged: {
        Qt.callLater(function() { root.syncDroplet() })
    }

    onHeightChanged: {
        Qt.callLater(function() { root.syncDroplet() })
    }

    onTabModeChanged: {
        Qt.callLater(function() { root.syncDroplet() })
    }

    onScrollableChanged: {
        Qt.callLater(function() { root.syncDroplet() })
    }
}
