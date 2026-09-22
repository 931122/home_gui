QT += core gui qml quick network websockets svg
QT -= widgets

CONFIG += c++14
CONFIG(debug, debug|release) {
    CONFIG += qml_debug
}

TEMPLATE = app
TARGET = home_gui

RK3506_DEFAULT_BUILDROOT_OUTPUT = ${HOME}/work/rockchip/luckfox/Lyra-sdk/buildroot/output/rockchip_rk3506_luckfox
isEmpty(BUILDROOT_OUTPUT) {
    BUILDROOT_OUTPUT = $$(BUILDROOT_OUTPUT)
}
isEmpty(BUILDROOT_OUTPUT) {
    BUILDROOT_OUTPUT = $$RK3506_DEFAULT_BUILDROOT_OUTPUT
}

RK3506_TOOLCHAIN_TRIPLE = arm-buildroot-linux-gnueabihf
RK3506_HOST_ROOT = $$BUILDROOT_OUTPUT/host
RK3506_SYSROOT = $$RK3506_HOST_ROOT/$$RK3506_TOOLCHAIN_TRIPLE/sysroot

SOURCES += \
    src/main.cpp \
    src/core/appconfig.cpp \
    src/core/configmanager.cpp \
    src/core/globalstate.cpp \
    src/core/imodule.cpp \
    src/core/modulemanager.cpp \
    src/core/networkutils.cpp \
    src/core/platformhelper.cpp \
    src/core/tstranslator.cpp \
    src/modules/homeassistant/homeassistantmodule.cpp \
    src/modules/video/backends/abstractvideobackend.cpp \
    src/modules/video/backends/gstreamervideobackend.cpp \
    src/modules/video/backends/libavvideobackend.cpp \
    src/modules/video/onvifclient.cpp \
    src/modules/video/videobackendfactory.cpp \
    src/modules/video/videomodule.cpp \
    src/modules/video/videoplayer.cpp \
    src/modules/wifi/wifimodule.cpp \
    src/modules/xiaozhi/xiaozhimodule.cpp \
    src/ui/appcontroller.cpp \
    src/ui/bootsplash.cpp \
    src/ui/brightnesscontroller.cpp \
    src/ui/screenpowermanager.cpp \
    src/ui/holidayservice.cpp \
    src/ui/weatherservice.cpp \
    src/ui/videoitem.cpp

HEADERS += \
    src/core/appconfig.h \
    src/core/configmanager.h \
    src/core/globalstate.h \
    src/core/imodule.h \
    src/core/modulemanager.h \
    src/core/networkutils.h \
    src/core/platformhelper.h \
    src/core/tstranslator.h \
    src/modules/homeassistant/homeassistantmodule.h \
    src/modules/video/backends/abstractvideobackend.h \
    src/modules/video/backends/gstreamervideobackend.h \
    src/modules/video/backends/libavvideobackend.h \
    src/modules/video/onvifclient.h \
    src/modules/video/videobackendfactory.h \
    src/modules/video/videomodule.h \
    src/modules/video/videoplayer.h \
    src/modules/wifi/wifimodule.h \
    src/modules/xiaozhi/xiaozhimodule.h \
    src/ui/appcontroller.h \
    src/ui/bootsplash.h \
    src/ui/brightnesscontroller.h \
    src/ui/screenpowermanager.h \
    src/ui/holidayservice.h \
    src/ui/weatherservice.h \
    src/ui/videoitem.h

RESOURCES += \
    resources/qml.qrc

TRANSLATIONS += \
    i18n/home_gui_zh_CN.ts

DISTFILES += \
    config.yaml.example \
    config.yaml \
    README.md \
    docs/ARCHITECTURE.md \
    docs/CONFIG_REFERENCE.md \
    docs/XIAOZHI_PORT.md \
    src/ui/qml/HaSidebar.qml \
    src/ui/qml/RoomSensorsPanel.qml \
    src/ui/qml/VideoBottomDockCard.qml \
    src/ui/qml/VideoBottomStatusCard.qml \
    src/ui/qml/VideoPanel.qml \
    src/ui/qml/VideoSurface.qml \
    src/ui/qml/WeatherPopup.qml \
    src/ui/qml/WifiPopups.qml

INCLUDEPATH += \
    $$PWD/src \
    $$PWD/src/3rdparty

contains(CONFIG, rk3506) {
    message("Building home_gui for RK3506 with Buildroot output: $$BUILDROOT_OUTPUT")

    QMAKE_CROSS_COMPILE = $$RK3506_HOST_ROOT/bin/$$RK3506_TOOLCHAIN_TRIPLE-
    QMAKE_CC = $$QMAKE_CROSS_COMPILE"gcc"
    QMAKE_CXX = $$QMAKE_CROSS_COMPILE"g++"
    QMAKE_LINK = $$QMAKE_CXX
    QMAKE_AR = $$QMAKE_CROSS_COMPILE"ar cqs"
    QMAKE_STRIP = $$QMAKE_CROSS_COMPILE"strip"
    QMAKE_OBJCOPY = $$QMAKE_CROSS_COMPILE"objcopy"

    QMAKE_INCDIR += $$RK3506_SYSROOT/usr/include
    QMAKE_LIBDIR += $$RK3506_SYSROOT/usr/lib

    QMAKE_CFLAGS += --sysroot=$$RK3506_SYSROOT
    QMAKE_CXXFLAGS += --sysroot=$$RK3506_SYSROOT
    QMAKE_LFLAGS += --sysroot=$$RK3506_SYSROOT

    PKG_CONFIG_SYSROOT_DIR = $$RK3506_SYSROOT
    PKG_CONFIG_LIBDIR = $$RK3506_SYSROOT/usr/lib/pkgconfig:$$RK3506_SYSROOT/usr/share/pkgconfig
    QMAKE_RPATHDIR =

}

PKG_CONFIG_CMD = pkg-config
contains(CONFIG, rk3506) {
    PKG_CONFIG_CMD = PKG_CONFIG_SYSROOT_DIR=$$PKG_CONFIG_SYSROOT_DIR PKG_CONFIG_LIBDIR=$$PKG_CONFIG_LIBDIR pkg-config
}

android {
    QT += androidextras
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    QMAKE_CFLAGS += -ffunction-sections -fdata-sections
    QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
    QMAKE_LFLAGS += -Wl,--gc-sections

    exists(${HOME}/Android/android_openssl/openssl.pri) {
        include(${HOME}/Android/android_openssl/openssl.pri)
        message("Android OpenSSL included from ${HOME}/Android/android_openssl")
    }

    exists($$PWD/android/ffmpeg/lib/libavformat.a) {
        INCLUDEPATH += $$PWD/android/ffmpeg/include
        LIBS += -L$$PWD/android/ffmpeg/lib -lavformat -lavcodec -lswscale -lavutil -lz
        DEFINES += HOME_GUI_HAS_FFMPEG
        message("Android FFmpeg static libraries enabled")
    }
}

!android {
    FFMPEG_PACKAGES = libavformat libavcodec libswscale libavutil
    ffmpeg_probe = $$system($$PKG_CONFIG_CMD --exists $$FFMPEG_PACKAGES && echo yes)
    equals(ffmpeg_probe, yes) {
        FFMPEG_CFLAGS = $$system($$PKG_CONFIG_CMD --cflags $$FFMPEG_PACKAGES)
        FFMPEG_LIBS = $$system($$PKG_CONFIG_CMD --libs $$FFMPEG_PACKAGES)
        QMAKE_CFLAGS += $$FFMPEG_CFLAGS
        QMAKE_CXXFLAGS += $$FFMPEG_CFLAGS
        LIBS += $$FFMPEG_LIBS
        DEFINES += HOME_GUI_HAS_FFMPEG
        message("Enabling FFmpeg backend via pkg-config: $$FFMPEG_PACKAGES")
    } else {
        warning("FFmpeg development files not found. qmake project can be opened, but building requires: $$FFMPEG_PACKAGES")
    }
}

unix:!android {
    GSTREAMER_PACKAGES = gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0
    gst_probe = $$system($$PKG_CONFIG_CMD --exists $$GSTREAMER_PACKAGES && echo yes)
    equals(gst_probe, yes) {
        GSTREAMER_CFLAGS = $$system($$PKG_CONFIG_CMD --cflags $$GSTREAMER_PACKAGES)
        GSTREAMER_LIBS = $$system($$PKG_CONFIG_CMD --libs $$GSTREAMER_PACKAGES)
        QMAKE_CFLAGS += $$GSTREAMER_CFLAGS
        QMAKE_CXXFLAGS += $$GSTREAMER_CFLAGS
        LIBS += $$GSTREAMER_LIBS
        DEFINES += HOME_GUI_HAS_GSTREAMER
        message("Enabling GStreamer backend via pkg-config: $$GSTREAMER_PACKAGES")
    } else {
        message("GStreamer development files not found, building without GStreamer backend")
    }
}

equals(OUT_PWD, $$PWD) {
    contains(CONFIG, rk3506) {
        BUILD_OUTPUT_DIR = $$PWD/build-rk3506
    } else: android {
        BUILD_OUTPUT_DIR = $$PWD/build-android
    } else {
        BUILD_OUTPUT_DIR = $$PWD/build
    }
} else {
    BUILD_OUTPUT_DIR = $$OUT_PWD
}

DESTDIR = $$BUILD_OUTPUT_DIR
OBJECTS_DIR = $$BUILD_OUTPUT_DIR/.obj
MOC_DIR = $$BUILD_OUTPUT_DIR/.moc
RCC_DIR = $$BUILD_OUTPUT_DIR/.rcc
UI_DIR = $$BUILD_OUTPUT_DIR/.uic

target.path = $$DESTDIR
INSTALLS += target

exists($$PWD/config.yaml) {
    CONFIG_FILE = $$PWD/config.yaml
} else {
    CONFIG_FILE = $$PWD/config.yaml.example
}

config_files.path = $$DESTDIR
config_files.files = $$CONFIG_FILE
INSTALLS += config_files

unix {
    config_source = $$shell_path($$CONFIG_FILE)
    config_target = $$shell_path($$DESTDIR/config.yaml)
    QMAKE_POST_LINK += $$QMAKE_COPY $$config_source $$config_target$$escape_expand(\\n\\t)
}

defineTest(printBuildSummary) {
    message("home_gui target output: $$DESTDIR")
    return(true)
}

printBuildSummary()
