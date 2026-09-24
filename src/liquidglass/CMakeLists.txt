qt_add_library(home_gui_liquidglass STATIC)

target_include_directories(home_gui_liquidglass
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(home_gui_liquidglass
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

set_source_files_properties(qml/GlassTheme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(home_gui_liquidglass
    URI HomeGui.LiquidGlass
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/HomeGui/LiquidGlass
    QML_FILES
        qml/GlassTheme.qml
        qml/FrostedGlassOverlay.qml
        qml/LiquidGlassSurface.qml
        qml/LiquidGlassButton.qml
        qml/LiquidGlassSlider.qml
        qml/LiquidGlassTabBar.qml
        qml/LiquidGlassChip.qml
        qml/ScrollEdgeBlurView.qml
        qml/LiquidGlassToast.qml
        qml/LiquidGlassFab.qml
        qml/LiquidGlassTabLayout.qml
        qml/LiquidGlassChipGroup.qml
        qml/LiquidGlassListItem.qml
        qml/LiquidGlassListGroup.qml
        qml/LiquidGlassDialog.qml
    RESOURCES
        shaders/liquid_glass_surface.vert.qsb
        shaders/liquid_glass_surface.frag.qsb
        shaders/frosted_glass.vert.qsb
        shaders/frosted_glass.frag.qsb
        shaders/liquid_glass_slider.frag.qsb
    SOURCES
        glassruntime.cpp
        glassruntime.h
)
