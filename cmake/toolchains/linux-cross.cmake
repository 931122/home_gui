# Unified Linux Cross-Compilation Toolchain for CMake
#
# Supports all embedded Linux cross-compilation environments:
# 1. Buildroot SDKs (RK3506, RK3568, RK3588, Allwinner, etc.) via BUILDROOT_OUTPUT
# 2. Yocto / OpenEmbedded SDKs via SDKTARGETSYSROOT / environment
# 3. Generic standalone toolchains (Linaro, GNU Toolchain, Debian multiarch, etc.)
#
# Usage:
#   # Buildroot mode:
#   cmake -S . -B build-cross \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-cross.cmake \
#     -DBUILDROOT_OUTPUT=/path/to/buildroot/output
#
#   # Generic mode:
#   cmake -S . -B build-cross \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-cross.cmake \
#     -DCROSS_COMPILE_PREFIX=aarch64-linux-gnu- \
#     -DCMAKE_SYSROOT=/path/to/sysroot \
#     -DQT_TARGET_ROOT=/path/to/target/qt6 \
#     -DQT_HOST_PATH=/path/to/host/qt6

set(CMAKE_SYSTEM_NAME Linux)

# ------------------------------------------------------------
# 1. Buildroot 模式识别与自动推导
# ------------------------------------------------------------
if(NOT BUILDROOT_OUTPUT AND DEFINED ENV{BUILDROOT_OUTPUT})
    set(BUILDROOT_OUTPUT "$ENV{BUILDROOT_OUTPUT}" CACHE PATH "Buildroot output directory")
endif()

if(BUILDROOT_OUTPUT AND EXISTS "${BUILDROOT_OUTPUT}/host/bin")
    set(HOST_ROOT "${BUILDROOT_OUTPUT}/host")

    # 自动探测编译器 Target Triple (如 aarch64-buildroot-linux-gnu 或 arm-buildroot-linux-gnueabihf)
    if(NOT TOOLCHAIN_TRIPLE)
        file(GLOB _detected_gccs "${HOST_ROOT}/bin/*-buildroot-linux-*-gcc")
        foreach(_gcc IN LISTS _detected_gccs)
            get_filename_component(_gcc_name "${_gcc}" NAME)
            if(_gcc_name MATCHES "^(.*)-gcc$")
                set(TOOLCHAIN_TRIPLE "${CMAKE_MATCH_1}" CACHE STRING "Cross toolchain target triple" FORCE)
                break
            endif()
        endforeach()
    endif()

    if(NOT TOOLCHAIN_TRIPLE)
        set(TOOLCHAIN_TRIPLE "arm-buildroot-linux-gnueabihf" CACHE STRING "Cross toolchain target triple")
    endif()

    set(TARGET_SYSROOT "${HOST_ROOT}/${TOOLCHAIN_TRIPLE}/sysroot")
    set(CMAKE_SYSROOT "${TARGET_SYSROOT}")
    set(CMAKE_STAGING_PREFIX "${TARGET_SYSROOT}/usr")

    set(CMAKE_C_COMPILER "${HOST_ROOT}/bin/${TOOLCHAIN_TRIPLE}-gcc")
    set(CMAKE_CXX_COMPILER "${HOST_ROOT}/bin/${TOOLCHAIN_TRIPLE}-g++")
    set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")

    set(CMAKE_PROGRAM_PATH "${HOST_ROOT}/bin")
    list(PREPEND CMAKE_PREFIX_PATH
        "${HOST_ROOT}"
        "${HOST_ROOT}/lib/cmake"
        "${TARGET_SYSROOT}/usr/lib/cmake"
    )
    set(QT_QMAKE_EXECUTABLE "${HOST_ROOT}/bin/qmake" CACHE FILEPATH "Qt qmake from Buildroot host tools")
endif()

# ------------------------------------------------------------
# 2. 通用 / Yocto / Standalone 模式参数处理
# ------------------------------------------------------------
if(NOT CMAKE_C_COMPILER)
    if(NOT CROSS_COMPILE_PREFIX)
        if(DEFINED ENV{CROSS_COMPILE_PREFIX})
            set(CROSS_COMPILE_PREFIX "$ENV{CROSS_COMPILE_PREFIX}")
        elseif(DEFINED ENV{CROSS_COMPILE})
            set(CROSS_COMPILE_PREFIX "$ENV{CROSS_COMPILE}")
        endif()
    endif()

    if(CROSS_COMPILE_PREFIX)
        set(CMAKE_C_COMPILER "${CROSS_COMPILE_PREFIX}gcc")
        set(CMAKE_CXX_COMPILER "${CROSS_COMPILE_PREFIX}g++")
        set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
    endif()
endif()

if(NOT CMAKE_SYSROOT)
    if(DEFINED ENV{SYSROOT})
        set(CMAKE_SYSROOT "$ENV{SYSROOT}" CACHE PATH "Target sysroot")
    elseif(DEFINED ENV{SDKTARGETSYSROOT})
        set(CMAKE_SYSROOT "$ENV{SDKTARGETSYSROOT}" CACHE PATH "Target sysroot (Yocto)")
    endif()
endif()

# ------------------------------------------------------------
# 3. 目标体系结构自动推导 (CMAKE_SYSTEM_PROCESSOR)
# ------------------------------------------------------------
if(NOT CMAKE_SYSTEM_PROCESSOR)
    set(_probe_str "${TOOLCHAIN_TRIPLE} ${CROSS_COMPILE_PREFIX} ${CMAKE_C_COMPILER}")
    if(_probe_str MATCHES "aarch64")
        set(CMAKE_SYSTEM_PROCESSOR aarch64)
    elseif(_probe_str MATCHES "arm")
        set(CMAKE_SYSTEM_PROCESSOR arm)
    elseif(_probe_str MATCHES "riscv64")
        set(CMAKE_SYSTEM_PROCESSOR riscv64)
    elseif(_probe_str MATCHES "x86_64")
        set(CMAKE_SYSTEM_PROCESSOR x86_64)
    else()
        set(CMAKE_SYSTEM_PROCESSOR aarch64)
    endif()
endif()

# ------------------------------------------------------------
# 4. Sysroot 与 PKG-CONFIG 环境变量隔离
# ------------------------------------------------------------
if(CMAKE_SYSROOT)
    set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}" ${HOST_ROOT})
    set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
    set(ENV{PKG_CONFIG_PATH}
        "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig:${CMAKE_SYSROOT}/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu/pkgconfig")
    set(ENV{PKG_CONFIG_LIBDIR}
        "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig:${CMAKE_SYSROOT}/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu/pkgconfig")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ------------------------------------------------------------
# 5. 目标端 Qt 6 路径补充
# ------------------------------------------------------------
if(NOT QT_TARGET_ROOT AND DEFINED ENV{QT_TARGET_ROOT})
    set(QT_TARGET_ROOT "$ENV{QT_TARGET_ROOT}" CACHE PATH "Target Qt installation")
endif()

if(QT_TARGET_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${QT_TARGET_ROOT}")
    list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_TARGET_ROOT}")
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

message(STATUS "Unified Linux Cross Toolchain initialized:")
message(STATUS "  Processor  : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Sysroot    : ${CMAKE_SYSROOT}")
message(STATUS "  C Compiler : ${CMAKE_C_COMPILER}")
message(STATUS "  CXX Compiler: ${CMAKE_CXX_COMPILER}")
