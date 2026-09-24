LiquidGlassTabBar {
    id: root

    enum TabMode { ModeFixed, ModeScrollable }

    property int tabMode: LiquidGlassTabLayout.TabMode.ModeFixed
    scrollable: tabMode === LiquidGlassTabLayout.TabMode.ModeScrollable

    function selectTab(index, animate) {
        if (index >= 0 && index < (model ? model.length : 0)) {
            currentIndex = index
            updateIndicator(animate !== false)
        }
    }

    function tabCount() {
        return model ? model.length : 0
    }
}
