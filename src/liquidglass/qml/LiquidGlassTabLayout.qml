LiquidGlassTabBar {
    id: root

    enum TabMode { ModeFixed, ModeScrollable }

    // 恢复默认行为与兼容性：默认保持为可横向滑动的 ModeScrollable
    property int tabMode: LiquidGlassTabLayout.TabMode.ModeScrollable
    scrollable: tabMode === LiquidGlassTabLayout.TabMode.ModeScrollable

    function _getModelCount() {
        if (!model) return 0
        if (typeof model.count === "number") return model.count
        if (typeof model.length === "number") return model.length
        if (typeof tabRepeater !== "undefined" && tabRepeater && typeof tabRepeater.count === "number") {
            return tabRepeater.count
        }
        return 0
    }

    function selectTab(index, animate) {
        var count = _getModelCount()
        if (index >= 0 && index < count) {
            currentIndex = index
            updateIndicator(animate !== false)
        }
    }

    function tabCount() {
        return _getModelCount()
    }
}
