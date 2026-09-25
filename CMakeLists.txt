cmake_minimum_required(VERSION 3.16)

set(HOME_GUI_TOOLCHAIN_FILE "" CACHE FILEPATH "CMake toolchain file for cross-compilation")
if(HOME_GUI_TOOLCHAIN_FILE AND NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${HOME_GUI_TOOLCHAIN_FILE}" CACHE FILEPATH "CMake toolchain file" FORCE)
endif()

project(home_gui VERSION 0.1.0 LANGUAGES CXX)

option(HOME_GUI_CROSS_COMPILE "Enable embedded cross-compilation settings" OFF)
if(CMAKE_CROSSCOMPILING)
    set(HOME_GUI_CROSS_COMPILE ON CACHE BOOL "Enable embedded cross-compilation settings" FORCE)
endif()
set(HOME_GUI_TARGET_QT_ROOT "" CACHE PATH "Target Qt installation prefix for cross-compilation")
set(HOME_GUI_TARGET_SYSROOT "" CACHE PATH "Target sysroot path for cross-compilation")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(PkgConfig REQUIRED)
option(HOME_GUI_ENABLE_GSTREAMER "Enable GStreamer video backend when development files are available" ON)
option(HOME_GUI_ENABLE_XIAOZHI "Build bundled Xiaozhi Linux audio backend" ON)

if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IOS TRUE)
endif()

if(APPLE AND NOT IOS)
    list(PREPEND CMAKE_PREFIX_PATH
        "/opt/homebrew/opt/qt6"
        "/opt/homebrew/opt/qt"
        "/usr/local/opt/qt6"
        "/usr/local/opt/qt"
    )
endif()

if(HOME_GUI_CROSS_COMPILE)
    if(HOME_GUI_TARGET_SYSROOT)
        set(CMAKE_SYSROOT "${HOME_GUI_TARGET_SYSROOT}" CACHE PATH "Target sysroot" FORCE)
    endif()

    if(HOME_GUI_TARGET_QT_ROOT)
        list(PREPEND CMAKE_PREFIX_PATH
            "${HOME_GUI_TARGET_QT_ROOT}"
            "${HOME_GUI_TARGET_QT_ROOT}/lib/cmake"
            "${HOME_GUI_TARGET_QT_ROOT}/lib/cmake/Qt6"
        )
    endif()

    add_compile_definitions(HOME_GUI_CROSS_COMPILE)
endif()

# 纯 Qt 6 架构要求
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Qml Quick Network WebSockets)
find_package(Qt6Svg QUIET)

message(STATUS "Configured with Qt ${Qt6_VERSION}")

add_subdirectory(src/liquidglass)

if(ANDROID)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android/ffmpeg/lib/libavformat.a")
        set(FFMPEG_FOUND TRUE)
        add_compile_definitions(HOME_GUI_HAS_FFMPEG)
        set(ANDROID_FFMPEG_LIBRARIES
            "${CMAKE_CURRENT_SOURCE_DIR}/android/ffmpeg/lib/libavformat.a"
            "${CMAKE_CURRENT_SOURCE_DIR}/android/ffmpeg/lib/libavcodec.a"
            "${CMAKE_CURRENT_SOURCE_DIR}/android/ffmpeg/lib/libswscale.a"
            "${CMAKE_CURRENT_SOURCE_DIR}/android/ffmpeg/lib/libavutil.a"
        )
    endif()
elseif(IOS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ios/ffmpeg/lib/libavformat.a")
        set(FFMPEG_FOUND TRUE)
        add_compile_definitions(HOME_GUI_HAS_FFMPEG)
        set(IOS_FFMPEG_LIBRARIES
            "${CMAKE_CURRENT_SOURCE_DIR}/ios/ffmpeg/lib/libavformat.a"
            "${CMAKE_CURRENT_SOURCE_DIR}/ios/ffmpeg/lib/libavcodec.a"
            "${CMAKE_CURRENT_SOURCE_DIR}/ios/ffmpeg/lib/libswscale.a"
            "${CMAKE_CURRENT_SOURCE_DIR}/ios/ffmpeg/lib/libavutil.a"
        )
    endif()
else()
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libswscale libavutil)
    if(FFMPEG_FOUND)
        add_compile_definitions(HOME_GUI_HAS_FFMPEG)
    endif()
endif()

if(HOME_GUI_ENABLE_GSTREAMER AND NOT ANDROID AND NOT IOS)
    pkg_check_modules(GSTREAMER QUIET IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
    if(GSTREAMER_FOUND)
        add_compile_definitions(HOME_GUI_HAS_GSTREAMER)
    endif()
endif()

if(IOS)
    set(QT_NO_ADD_IOS_LAUNCH_SCREEN_TO_BUNDLE TRUE)
endif()

qt_add_executable(home_gui
    src/main.cpp
    src/core/appconfig.cpp
    src/core/configmanager.cpp
    src/core/globalstate.cpp
    src/core/imodule.cpp
    src/core/modulemanager.cpp
    src/core/networkutils.cpp
    src/core/platformhelper.cpp
    src/core/tstranslator.cpp
    src/modules/video/videomodule.cpp
    src/modules/video/onvifclient.cpp
    src/modules/video/backends/abstractvideobackend.cpp
    src/modules/video/backends/gstreamervideobackend.cpp
    src/modules/video/backends/libavvideobackend.cpp
    src/modules/video/videobackendfactory.cpp
    src/modules/video/videoplayer.cpp
    src/modules/homeassistant/homeassistantmodule.cpp
    src/modules/wifi/wifimodule.cpp
    src/modules/xiaozhi/xiaozhimodule.cpp
    src/ui/appcontroller.cpp
    src/ui/bootsplash.cpp
    src/ui/brightnesscontroller.cpp
    src/ui/screenpowermanager.cpp
    src/ui/holidayservice.cpp
    src/ui/weatherservice.cpp
    src/ui/videoitem.cpp
    resources/qml.qrc
)

target_include_directories(home_gui PRIVATE src src/3rdparty)
if(ANDROID AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android/ffmpeg/include")
    target_include_directories(home_gui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/android/ffmpeg/include")
endif()
if(IOS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ios/ffmpeg/include")
    target_include_directories(home_gui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ios/ffmpeg/include")
endif()

find_package(ZLIB QUIET)

target_link_libraries(home_gui PRIVATE
    home_gui_liquidglass
    home_gui_liquidglassplugin_init
    home_gui_liquidglassplugin
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::WebSockets
)

if(ANDROID)
    # androiddeployqt otherwise emits an unsigned Release APK, which Android
    # refuses to install. Qt uses the local debug keystore by default when
    # QT_ANDROID_SIGN_APK is enabled; production builds can override the
    # keystore through QT_ANDROID_KEYSTORE_* environment variables.
    set(QT_ANDROID_SIGN_APK ON CACHE BOOL "Sign Android APKs during packaging")

    if(ANDROID_FFMPEG_LIBRARIES)
        target_link_libraries(home_gui PRIVATE ${ANDROID_FFMPEG_LIBRARIES} z)
    endif()

    # OpenSSL 支持库集成 (KDAB android_openssl，确保 HTTPS / TLS 网络请求正常)
    if(NOT ANDROID_OPENSSL_ROOT)
        if(DEFINED ENV{ANDROID_OPENSSL_ROOT})
            set(ANDROID_OPENSSL_ROOT "$ENV{ANDROID_OPENSSL_ROOT}")
        elseif(EXISTS "$ENV{HOME}/Android/android_openssl")
            set(ANDROID_OPENSSL_ROOT "$ENV{HOME}/Android/android_openssl")
        endif()
    endif()
    if(EXISTS "${ANDROID_OPENSSL_ROOT}/android_openssl.cmake")
        message(STATUS "Found Android OpenSSL: ${ANDROID_OPENSSL_ROOT}")
        include("${ANDROID_OPENSSL_ROOT}/android_openssl.cmake")
        add_android_openssl_libraries(home_gui)
    else()
        message(WARNING "Android OpenSSL not found at ${ANDROID_OPENSSL_ROOT}")
    endif()

    set_target_properties(home_gui PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_TARGET_SDK_VERSION "33"
        QT_ANDROID_MIN_SDK_VERSION "23"
    )
elseif(IOS)
    if(IOS_FFMPEG_LIBRARIES)
        target_link_libraries(home_gui PRIVATE ${IOS_FFMPEG_LIBRARIES} z bz2 iconv)
    endif()

    set_target_properties(home_gui PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.homegui.app"
        MACOSX_BUNDLE_BUNDLE_NAME "HomeGui"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.homegui.app"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    )


    target_link_libraries(home_gui PRIVATE
        "-framework UIKit"
        "-framework Foundation"
        "-framework CoreGraphics"
        "-framework QuartzCore"
        "-framework Network"
        "-framework Security"
        "-framework SystemConfiguration"
    )
else()
    if(FFMPEG_FOUND)
        target_link_libraries(home_gui PRIVATE PkgConfig::FFMPEG)
    endif()
endif()

if(Qt6Svg_FOUND)
    target_link_libraries(home_gui PRIVATE Qt6::Svg)
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(home_gui PRIVATE z)
endif()


if(GSTREAMER_FOUND)
    target_link_libraries(home_gui PRIVATE PkgConfig::GSTREAMER)
endif()

if(HOME_GUI_ENABLE_XIAOZHI AND NOT ANDROID AND NOT IOS)
    pkg_check_modules(XIAOZHI_AUDIO QUIET IMPORTED_TARGET alsa opus speexdsp)
    if(XIAOZHI_AUDIO_FOUND)
        add_executable(xiaozhi_sound_app
            third_party/xiaozhi-linux/sound_app/sound_app.cpp
            third_party/xiaozhi-linux/sound_app/aplay.cpp
            third_party/xiaozhi-linux/sound_app/record.cpp
            third_party/xiaozhi-linux/sound_app/opus.cpp
            third_party/xiaozhi-linux/sound_app/ipc_udp.cpp
        )
        target_include_directories(xiaozhi_sound_app PRIVATE
            third_party/xiaozhi-linux/sound_app
        )
        target_link_libraries(xiaozhi_sound_app PRIVATE
            PkgConfig::XIAOZHI_AUDIO
            pthread
        )
    else()
        message(WARNING "Xiaozhi audio backend skipped: alsa/opus/speexdsp development files not found")
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml")
    file(COPY config.yaml DESTINATION ${CMAKE_BINARY_DIR})
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml.example")
    file(COPY config.yaml.example DESTINATION ${CMAKE_BINARY_DIR})
endif()

message(STATUS "HOME_GUI_CROSS_COMPILE=${HOME_GUI_CROSS_COMPILE}")
message(STATUS "HOME_GUI_TARGET_SYSROOT=${HOME_GUI_TARGET_SYSROOT}")
message(STATUS "HOME_GUI_TARGET_QT_ROOT=${HOME_GUI_TARGET_QT_ROOT}")
