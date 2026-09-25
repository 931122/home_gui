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

if(DEFINED ENV{QSB_EXECUTABLE} AND EXISTS "$ENV{QSB_EXECUTABLE}")
    set(QSB_EXECUTABLE "$ENV{QSB_EXECUTABLE}")
elseif(TARGET Qt6::qsb)
    get_target_property(QSB_EXECUTABLE Qt6::qsb IMPORTED_LOCATION)
else()
    set(_qt_search_bins "")
    if(Qt6_DIR)
        get_filename_component(_qt_root_dir "${Qt6_DIR}/../../.." ABSOLUTE)
        list(APPEND _qt_search_bins "${_qt_root_dir}/bin" "${_qt_root_dir}/libexec")
    endif()
    if(QT_HOST_PATH)
        list(APPEND _qt_search_bins "${QT_HOST_PATH}/bin" "${QT_HOST_PATH}/libexec")
    endif()
    if(DEFINED ENV{QT_HOST_PATH})
        list(APPEND _qt_search_bins "$ENV{QT_HOST_PATH}/bin" "$ENV{QT_HOST_PATH}/libexec")
    endif()

    find_program(QSB_EXECUTABLE NAMES qsb
        PATHS
            ${_qt_search_bins}
            /opt/homebrew/opt/qtshadertools/bin
            /usr/local/opt/qtshadertools/bin
            /opt/homebrew/bin
            /usr/local/bin
            /opt/homebrew/opt/qt/bin
            /usr/local/opt/qt/bin
            /opt/homebrew/opt/qt6/bin
            /opt/homebrew/opt/qt6/libexec
            /usr/local/opt/qt6/bin
            /usr/local/opt/qt6/libexec
            $ENV{HOME}/Android/Qt/6.6.3/gcc_64/bin
    )
endif()

if(NOT QSB_EXECUTABLE)
    message(FATAL_ERROR "Qt Shader Baker (qsb) not found. Please install Qt Shader Tools (e.g. 'brew install qtshadertools' on macOS) or specify QSB_EXECUTABLE/QT_HOST_PATH.")
endif()

message(STATUS "LiquidGlass QSB Compiler: ${QSB_EXECUTABLE}")

set(LIQUIDGLASS_SHADERS
    liquid_glass_surface.vert
    liquid_glass_surface.frag
    frosted_glass.vert
    frosted_glass.frag
    liquid_glass_slider.frag
)

set(LIQUIDGLASS_QSB_RESOURCES "")

foreach(_shader IN LISTS LIQUIDGLASS_SHADERS)
    set(_src "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${_shader}")
    set(_out "${CMAKE_CURRENT_BINARY_DIR}/shaders/${_shader}.qsb")

    add_custom_command(
        OUTPUT "${_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND "${QSB_EXECUTABLE}" --qt6 -b -o "${_out}" "${_src}"
        DEPENDS "${_src}"
        COMMENT "Building QSB shader: ${_shader} -> ${CMAKE_CURRENT_BINARY_DIR}/shaders/${_shader}.qsb"
        VERBATIM
    )

    set_source_files_properties("${_out}" PROPERTIES
        GENERATED TRUE
        QT_RESOURCE_ALIAS "shaders/${_shader}.qsb"
    )
    list(APPEND LIQUIDGLASS_QSB_RESOURCES "${_out}")
endforeach()

set(LIQUIDGLASS_QML_EXTRA_OPTS "")
if(IOS OR QT_QML_NO_CACHEGEN)
    list(APPEND LIQUIDGLASS_QML_EXTRA_OPTS NO_CACHEGEN)
endif()

qt_add_qml_module(home_gui_liquidglass
    URI HomeGui.LiquidGlass
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/HomeGui/LiquidGlass
    ${LIQUIDGLASS_QML_EXTRA_OPTS}
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
        ${LIQUIDGLASS_QSB_RESOURCES}
    SOURCES
        glassruntime.cpp
        glassruntime.h
)
