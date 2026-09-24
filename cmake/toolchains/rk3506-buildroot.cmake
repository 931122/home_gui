set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Buildroot output root on the Linux build server.
# You can override this when invoking cmake:
#   -DBUILDROOT_OUTPUT=/path/to/buildroot/output/rockchip_rk3506_luckfox
if(NOT BUILDROOT_OUTPUT)
    if(DEFINED ENV{BUILDROOT_OUTPUT})
        set(BUILDROOT_OUTPUT "$ENV{BUILDROOT_OUTPUT}" CACHE PATH "Buildroot output directory")
    elseif(EXISTS "$ENV{HOME}/work/rockchip/luckfox/Lyra-sdk/buildroot/output/rockchip_rk3506_luckfox")
        set(BUILDROOT_OUTPUT "$ENV{HOME}/work/rockchip/luckfox/Lyra-sdk/buildroot/output/rockchip_rk3506_luckfox" CACHE PATH "Buildroot output directory")
    endif()
endif()

set(TOOLCHAIN_TRIPLE "arm-buildroot-linux-gnueabihf"
    CACHE STRING "Cross toolchain target triple")

set(HOST_ROOT "${BUILDROOT_OUTPUT}/host")
set(TARGET_SYSROOT "${HOST_ROOT}/${TOOLCHAIN_TRIPLE}/sysroot")

set(CMAKE_SYSROOT "${TARGET_SYSROOT}")
set(CMAKE_STAGING_PREFIX "${TARGET_SYSROOT}/usr")

set(CMAKE_C_COMPILER "${HOST_ROOT}/bin/${TOOLCHAIN_TRIPLE}-gcc")
set(CMAKE_CXX_COMPILER "${HOST_ROOT}/bin/${TOOLCHAIN_TRIPLE}-g++")
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")

set(CMAKE_PROGRAM_PATH "${HOST_ROOT}/bin")

set(CMAKE_FIND_ROOT_PATH
    "${TARGET_SYSROOT}"
    "${HOST_ROOT}"
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ENV{PKG_CONFIG_SYSROOT_DIR} "${TARGET_SYSROOT}")
set(ENV{PKG_CONFIG_PATH}
    "${TARGET_SYSROOT}/usr/lib/pkgconfig:${TARGET_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR}
    "${TARGET_SYSROOT}/usr/lib/pkgconfig:${TARGET_SYSROOT}/usr/share/pkgconfig")

# Qt 5 from Buildroot host tools.
list(PREPEND CMAKE_PREFIX_PATH
    "${HOST_ROOT}"
    "${HOST_ROOT}/lib/cmake"
    "${TARGET_SYSROOT}/usr/lib/cmake"
)

set(QT_QMAKE_EXECUTABLE "${HOST_ROOT}/bin/qmake"
    CACHE FILEPATH "Qt qmake from Buildroot host tools")

# Buildroot toolchains generally do not support try-run on the target.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

message(STATUS "Using RK3506 Buildroot toolchain")
message(STATUS "BUILDROOT_OUTPUT=${BUILDROOT_OUTPUT}")
message(STATUS "CMAKE_SYSROOT=${CMAKE_SYSROOT}")
message(STATUS "CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
message(STATUS "QT_QMAKE_EXECUTABLE=${QT_QMAKE_EXECUTABLE}")
