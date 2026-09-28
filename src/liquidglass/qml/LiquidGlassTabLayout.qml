LiquidGlassTabBar {
    id: root

    // LiquidGlassTabLayout 默认保持为可横向滑动的 ModeScrollable
    tabMode: LiquidGlassTabBar.TabMode.ModeScrollable

    function tabCount() {
        return root._getModelCount()
    }
}
