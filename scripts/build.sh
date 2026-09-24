#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

DEFAULT_RK3506_OUTPUT="${HOME}/work/rockchip/luckfox/Lyra-sdk/buildroot/output/rockchip_rk3506_luckfox"
PLATFORM="${1:-}"

usage() {
    cat <<'EOF'
Usage:
  ./scripts/build.sh                  # interactive platform selection
  ./scripts/build.sh native           # build for local host (auto-detect Linux/macOS)
  ./scripts/build.sh linux            # build for local Linux
  ./scripts/build.sh macos            # build for local macOS
  ./scripts/build.sh rk3506           # cross-build for RK3506 Buildroot
  ./scripts/build.sh rk3506 /path/to/buildroot/output/rockchip_rk3506_luckfox
  ./scripts/build.sh android          # build Android APK (arm64-v8a)
  ./scripts/build.sh clean            # remove all local build directories
  ./scripts/build.sh clean native     # remove local build directory
  ./scripts/build.sh clean rk3506     # remove RK3506 build directory
  ./scripts/build.sh clean android    # remove Android build directory

Environment:
  BUILDROOT_OUTPUT    Override RK3506 Buildroot output path
  BUILD_JOBS          Override parallel build jobs
  PKG_CONFIG_PATH     Additional pkg-config search directories

Examples:
  ./scripts/build.sh native
  ./scripts/build.sh linux
  ./scripts/build.sh macos
  ./scripts/build.sh rk3506
  ./scripts/build.sh clean
  ./scripts/build.sh clean native
  BUILDROOT_OUTPUT=${HOME}/work/.../rockchip_rk3506_luckfox ./scripts/build.sh rk3506
  BUILD_JOBS=8 ./scripts/build.sh native
EOF
}

detect_host_os() {
    case "$(uname -s)" in
        Darwin) echo "macos" ;;
        Linux)  echo "linux" ;;
        *)      echo "native" ;;
    esac
}

detect_build_jobs() {
    if [[ -n "${BUILD_JOBS:-}" ]]; then
        echo "${BUILD_JOBS}"
        return
    fi

    if command -v nproc >/dev/null 2>&1; then
        nproc
        return
    fi

    if command -v getconf >/dev/null 2>&1; then
        getconf _NPROCESSORS_ONLN
        return
    fi

    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu
        return
    fi

    echo 4
}

choose_platform() {
    local host_os
    host_os="$(detect_host_os)"
    echo "Select build platform:"
    echo "  1) native (${host_os} - detected host)"
    echo "  2) rk3506 (cross-compile)"
    echo "  3) android (cross-compile arm64-v8a)"
    printf "> "
    read -r selection
    case "${selection}" in
        1|native|host|local|linux|macos) PLATFORM="native" ;;
        2|rk3506) PLATFORM="rk3506" ;;
        3|android) PLATFORM="android" ;;
        *) echo "Invalid selection: ${selection}" >&2; exit 1 ;;
    esac
}

ensure_qsb_shaders() {
    local qsb_bin="${HOME}/Android/Qt/6.6.3/gcc_64/bin/qsb"
    if [[ ! -x "${qsb_bin}" ]]; then
        qsb_bin="$(find ${HOME}/Android/Qt -name "qsb" -type f -perm /111 2>/dev/null | head -n 1)"
    fi
    if [[ ! -x "${qsb_bin}" ]]; then
        qsb_bin="$(command -v qsb || true)"
    fi

    local shader_dir="${ROOT_DIR}/src/liquidglass/shaders"
    local legacy_shader_dir="${ROOT_DIR}/src/ui/shaders"
    if [[ -n "${qsb_bin}" && -x "${qsb_bin}" && -d "${shader_dir}" ]]; then
        echo "Compiling Qt 6 QSB shaders..."
        "${qsb_bin}" --qt6 -b -o "${legacy_shader_dir}/default.vert.qsb" "${legacy_shader_dir}/default.vert"
        "${qsb_bin}" --qt6 -b -o "${shader_dir}/liquid_glass_slider.frag.qsb" "${shader_dir}/liquid_glass_slider.frag"
        "${qsb_bin}" --qt6 -b -o "${shader_dir}/liquid_glass_surface.frag.qsb" "${shader_dir}/liquid_glass_surface.frag"
        "${qsb_bin}" --qt6 -b -o "${shader_dir}/frosted_glass.frag.qsb" "${shader_dir}/frosted_glass.frag"
    fi
}

ensure_native_ffmpeg() {
    local deps_dir="${ROOT_DIR}/.deps/ffmpeg"

    if pkg-config --exists libavformat libavcodec libswscale libavutil 2>/dev/null; then
        return 0
    fi

    if [[ -d "${deps_dir}" ]]; then
        local cached_pc_dir
        cached_pc_dir="$(find "${deps_dir}" -name "libavformat.pc" -printf '%h\n' 2>/dev/null | head -n 1)"
        if [[ -n "${cached_pc_dir}" ]]; then
            export PKG_CONFIG_PATH="${cached_pc_dir}:${PKG_CONFIG_PATH:-}"
            if pkg-config --exists libavformat libavcodec libswscale libavutil 2>/dev/null; then
                return 0
            fi
        fi
    fi

    # On Debian/Ubuntu without system dev packages, attempt automatic local extraction without root
    if [[ "$(detect_host_os)" == "linux" ]] && command -v apt-get >/dev/null 2>&1 && command -v dpkg-deb >/dev/null 2>&1; then
        echo "未检测到系统 ffmpeg 开发包，正在为本机构建自动准备开发依赖到 .deps/ffmpeg ..."
        mkdir -p "${deps_dir}/download"
        (
            cd "${deps_dir}/download"
            apt-get download libavformat-dev libavcodec-dev libswscale-dev libavutil-dev libswresample-dev 2>/dev/null || true
            for deb in *.deb; do
                if [[ -f "${deb}" ]]; then
                    dpkg-deb -x "${deb}" "${deps_dir}"
                fi
            done
        )
        rm -rf "${deps_dir}/download"

        local pc_dir
        pc_dir="$(find "${deps_dir}" -name "libavformat.pc" -printf '%h\n' 2>/dev/null | head -n 1)"
        if [[ -n "${pc_dir}" ]]; then
            local real_prefix="${deps_dir}/usr"
            for pc in "${pc_dir}"/*.pc; do
                if [[ -f "${pc}" ]]; then
                    sed -i "s|prefix=/usr|prefix=${real_prefix}|g" "${pc}"
                    sed -i "s|/usr/include|${real_prefix}/include|g" "${pc}"
                    sed -i "s|libdir=/usr/lib|libdir=${real_prefix}/lib|g" "${pc}"
                fi
            done

            local arch_lib_dir
            arch_lib_dir="$(find "${deps_dir}/usr/lib" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | head -n 1)"
            if [[ -n "${arch_lib_dir}" ]]; then
                for lib in libavformat libavcodec libswscale libavutil libswresample; do
                    local sys_lib
                    sys_lib="$(find /usr/lib /lib -name "${lib}.so.*" 2>/dev/null | head -n 1)"
                    if [[ -n "${sys_lib}" ]]; then
                        ln -sf "${sys_lib}" "${arch_lib_dir}/${lib}.so"
                    fi
                done
            fi

            export PKG_CONFIG_PATH="${pc_dir}:${PKG_CONFIG_PATH:-}"
            if pkg-config --exists libavformat libavcodec libswscale libavutil 2>/dev/null; then
                echo "本机 ffmpeg 开发依赖准备完成。"
                return 0
            fi
        fi
    fi

    echo "Error: Required FFmpeg development libraries (libavformat, libavcodec, libswscale, libavutil) not found." >&2
    echo "Please install them via your package manager:" >&2
    echo "  Ubuntu/Debian: sudo apt install -y libavcodec-dev libavformat-dev libswscale-dev libavutil-dev libswresample-dev" >&2
    echo "  macOS:         brew install ffmpeg" >&2
    echo "  Fedora:        sudo dnf install ffmpeg-free-devel" >&2
    echo "  Arch Linux:    sudo pacman -S ffmpeg" >&2
    echo "Or set PKG_CONFIG_PATH pointing to existing ffmpeg pkg-config files." >&2
    return 1
}

setup_macos_env() {
    # Add common Homebrew search paths if available
    local brew_paths=(
        "/opt/homebrew/opt/qt6"
        "/opt/homebrew/opt/qt"
        "/usr/local/opt/qt6"
        "/usr/local/opt/qt"
        "/opt/homebrew/opt/ffmpeg"
        "/usr/local/opt/ffmpeg"
    )
    for bp in "${brew_paths[@]}"; do
        if [[ -d "${bp}/lib/pkgconfig" ]]; then
            export PKG_CONFIG_PATH="${bp}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
        fi
    done
}

run_native_build() {
    local requested_platform="$1"
    local host_os
    host_os="$(detect_host_os)"
    local build_dir="${ROOT_DIR}/build"
    local build_jobs
    build_jobs="$(detect_build_jobs)"

    if [[ "${host_os}" == "macos" || "${requested_platform}" == "macos" ]]; then
        setup_macos_env
    fi

    ensure_native_ffmpeg
    ensure_qsb_shaders

    local cmake_extra_args=()
    if [[ -z "${CMAKE_PREFIX_PATH:-}" ]]; then
        local possible_qt6_paths=(
            "${HOME}/Android/Qt/6.6.3/gcc_64"
            "/opt/Qt/6.6.3/gcc_64"
            "/usr/local/Qt-6.6.3"
        )
        for p in "${possible_qt6_paths[@]}"; do
            if [[ -d "${p}" ]]; then
                cmake_extra_args+=("-DCMAKE_PREFIX_PATH=${p}")
                break
            fi
        done
    fi

    cmake -S "${ROOT_DIR}" -B "${build_dir}" "${cmake_extra_args[@]}"
    cmake --build "${build_dir}" --parallel "${build_jobs}"
    echo
    echo "Build finished:"
    echo "  platform  : native (${host_os})"
    echo "  build jobs: ${build_jobs}"
    echo "  build dir : ${build_dir}"
    echo "  binary    : ${build_dir}/home_gui"
}

run_rk3506_build() {
    local buildroot_output="${2:-${BUILDROOT_OUTPUT:-${DEFAULT_RK3506_OUTPUT}}}"
    local build_dir="${ROOT_DIR}/build-rk3506"
    local toolchain_file="${ROOT_DIR}/cmake/toolchains/rk3506-buildroot.cmake"
    local build_jobs
    build_jobs="$(detect_build_jobs)"

    if [[ ! -d "${buildroot_output}" ]]; then
        echo "RK3506 Buildroot output not found:" >&2
        echo "  ${buildroot_output}" >&2
        echo "Pass it as the second argument or export BUILDROOT_OUTPUT." >&2
        exit 1
    fi

    # Toolchain selection is cached on first configure. Clear stale cache so a
    # previous native configure cannot silently override the cross toolchain.
    cmake -E remove -f "${build_dir}/CMakeCache.txt"
    cmake -E remove_directory "${build_dir}/CMakeFiles"

    cmake -S "${ROOT_DIR}" -B "${build_dir}" \
        -DCMAKE_TOOLCHAIN_FILE="${toolchain_file}" \
        -DHOME_GUI_CROSS_COMPILE=ON \
        -DHOME_GUI_TOOLCHAIN_FILE="${toolchain_file}" \
        -DBUILDROOT_OUTPUT="${buildroot_output}"
    cmake --build "${build_dir}" --parallel "${build_jobs}"
    echo
    echo "Build finished:"
    echo "  platform       : rk3506"
    echo "  build jobs     : ${build_jobs}"
    echo "  buildroot path : ${buildroot_output}"
    echo "  build dir      : ${build_dir}"
}

run_android_build() {
    local build_dir="${ROOT_DIR}/build-android"
    local build_jobs
    build_jobs="$(detect_build_jobs)"

    local qt_android_dir="${QT_ANDROID_DIR:-${HOME}/Android/Qt/6.6.3/android_arm64_v8a}"
    local android_sdk_root="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-${HOME}/Android/Sdk}}"
    local android_ndk_root="${ANDROID_NDK_ROOT:-${HOME}/Android/ndk/android-ndk-r25c}"
    local java_home="${JAVA_HOME:-/usr/lib/jvm/java-11-openjdk-amd64}"
    local android_platform="${ANDROID_PLATFORM:-android-33}"
    local android_build_tools="${ANDROID_BUILD_TOOLS:-33.0.2}"
    local android_abi="${ANDROID_ABI:-arm64-v8a}"

    if [[ ! -d "${qt_android_dir}" || (! -x "${qt_android_dir}/bin/qt-cmake" && ! -x "${qt_android_dir}/../gcc_64/bin/androiddeployqt") ]]; then
        echo "Error: Qt 6 for Android not found at ${qt_android_dir}" >&2
        echo "Please install Qt 6 for Android (e.g. Qt 6.6.3 / 6.8.x arm64-v8a) or export QT_ANDROID_DIR." >&2
        exit 1
    fi

    if [[ ! -d "${android_sdk_root}" ]]; then
        echo "Error: Android SDK not found at ${android_sdk_root}" >&2
        echo "Please export ANDROID_SDK_ROOT or ANDROID_HOME." >&2
        exit 1
    fi

    if [[ ! -d "${android_ndk_root}" ]]; then
        if [[ -d "${HOME}/Android/ndk/android-ndk-r25c" ]]; then
            android_ndk_root="${HOME}/Android/ndk/android-ndk-r25c"
        elif [[ -d "${android_sdk_root}/ndk-bundle" ]]; then
            android_ndk_root="${android_sdk_root}/ndk-bundle"
        else
            echo "Error: Android NDK not found at ${android_ndk_root}" >&2
            echo "Please export ANDROID_NDK_ROOT." >&2
            exit 1
        fi
    fi

    if [[ ! -d "${java_home}" ]]; then
        if command -v javac >/dev/null 2>&1; then
            java_home="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
        fi
    fi

    export ANDROID_SDK_ROOT="${android_sdk_root}"
    export ANDROID_HOME="${android_sdk_root}"
    export ANDROID_NDK_ROOT="${android_ndk_root}"
    export ANDROID_NDK_HOST="linux-x86_64"
    export JAVA_HOME="${java_home}"
    export PATH="${qt_android_dir}/bin:${java_home}/bin:${PATH}"

    # 检查并自动准备 Android 专属依赖 (OpenSSL & FFmpeg)
    local openssl_dir="${HOME}/Android/android_openssl"
    if [[ ! -f "${openssl_dir}/openssl.pri" ]]; then
        echo "正在获取 Qt Android OpenSSL 支持库..."
        git clone --depth 1 https://github.com/KDAB/android_openssl.git "${openssl_dir}" 2>/dev/null || true
    fi

    local ffmpeg_lib="${ROOT_DIR}/android/ffmpeg/lib/libavformat.a"
    local ffmpeg_tar="${HOME}/work/rockchip/luckfox/Lyra-sdk/buildroot/dl/ffmpeg/ffmpeg-4.4.4.tar.xz"
    if [[ ! -f "${ffmpeg_lib}" && -f "${ffmpeg_tar}" ]]; then
        echo "正在为 Android (arm64-v8a) 交叉编译纯净版 FFmpeg 静态库..."
        local tmp_build="/tmp/build-ffmpeg-android"
        rm -rf "${tmp_build}" && mkdir -p "${tmp_build}"
        tar -xf "${ffmpeg_tar}" -C "${tmp_build}" --strip-components=1
        echo "正在应用 Android 高通/联发科 MediaCodec 内存格式与鲁棒性补丁..."
        python3 - "${tmp_build}/libavcodec/mediacodecdec_common.c" <<'EOF'
import sys
c_path = sys.argv[1]
with open(c_path, "r") as f:
    code = f.read()

# 1. 声明 COLOR_FormatYUV420Flexible
target1 = "COLOR_FormatAndroidOpaque                             = 0x7F000789,"
repl1 = target1 + "\n    COLOR_FormatYUV420Flexible                            = 0x7f420888,"
if target1 in code and "COLOR_FormatYUV420Flexible" not in code:
    code = code.replace(target1, repl1, 1)

# 2. 注册 COLOR_FormatYUV420Flexible 映射为 NV12
target2 = "{ COLOR_FormatYUV420SemiPlanar,                          AV_PIX_FMT_NV12    },"
repl2 = target2 + "\n    { COLOR_FormatYUV420Flexible,                            AV_PIX_FMT_NV12    },"
if target2 in code and "{ COLOR_FormatYUV420Flexible," not in code:
    code = code.replace(target2, repl2, 1)

# 3. 针对未知或厂商私有色彩格式平滑回退 NV12
target3 = 'av_log(avctx, AV_LOG_ERROR, "Output color format 0x%x (value=%d) is not supported\\n",\n        color_format, color_format);\n\n    return ret;'
repl3 = 'av_log(avctx, AV_LOG_WARNING, "Output color format 0x%x (value=%d) unrecognized, fallback to NV12\\n",\n        color_format, color_format);\n    return AV_PIX_FMT_NV12;'
if target3 in code:
    code = code.replace(target3, repl3, 1)

# 4. 在 switch 中增加 flexible 模式分支
target4 = "case COLOR_FormatYUV420SemiPlanar:"
repl4 = "case COLOR_FormatYUV420SemiPlanar:\n    case COLOR_FormatYUV420Flexible:"
if target4 in code and "case COLOR_FormatYUV420Flexible:" not in code:
    code = code.replace(target4, repl4, 1)

# 5. default switch 分支默认尝试 semi-planar 拷贝
target5 = 'default:\n        av_log(avctx, AV_LOG_ERROR, "Unsupported color format 0x%x (value=%d)\\n",\n            s->color_format, s->color_format);\n        ret = AVERROR(EINVAL);\n        goto done;'
repl5 = 'default:\n        av_log(avctx, AV_LOG_WARNING, "Unsupported color format 0x%x (value=%d), attempting semi-planar copy\\n",\n            s->color_format, s->color_format);\n        ff_mediacodec_sw_buffer_copy_yuv420_semi_planar(avctx, s, data, size, info, frame);\n        break;'
if target5 in code:
    code = code.replace(target5, repl5, 1)

# 6. 将 color-format 字段设为非必选，未就绪时默认赋值 flexible
target6 = 'AMEDIAFORMAT_GET_INT32(s->color_format, "color-format", 1);'
repl6 = 'AMEDIAFORMAT_GET_INT32(s->color_format, "color-format", 0);\n    if (!s->color_format) {\n        s->color_format = COLOR_FormatYUV420Flexible;\n    }'
if target6 in code:
    code = code.replace(target6, repl6, 1)

# 7. 初始阶段 output format 不完整时不抛异常，延迟到首帧就绪
target7 = '''    s->format = ff_AMediaCodec_getOutputFormat(s->codec);
    if (s->format) {
        if ((ret = mediacodec_dec_parse_format(avctx, s)) < 0) {
            av_log(avctx, AV_LOG_ERROR,
                "Failed to configure context\\n");
            goto fail;
        }
    }'''
repl7 = '''    s->format = ff_AMediaCodec_getOutputFormat(s->codec);
    if (s->format) {
        if ((ret = mediacodec_dec_parse_format(avctx, s)) < 0) {
            av_log(avctx, AV_LOG_WARNING,
                "Initial output format incomplete, deferring format parse\\n");
            ret = 0;
        }
    }'''
if target7 in code:
    code = code.replace(target7, repl7, 1)

# 8. 若未提供 Surface，显式设置输出色彩格式为 NV12 并要求解码器输出 Buffer
target8 = """        if (!s->surface && user_ctx && user_ctx->surface) {
            s->surface = ff_mediacodec_surface_ref(user_ctx->surface, avctx);
            av_log(avctx, AV_LOG_INFO, "Using surface %p\\n", s->surface);
        }
    }"""
repl8 = """        if (!s->surface && user_ctx && user_ctx->surface) {
            s->surface = ff_mediacodec_surface_ref(user_ctx->surface, avctx);
            av_log(avctx, AV_LOG_INFO, "Using surface %p\\n", s->surface);
        }
    }

    if (!s->surface) {
        avctx->pix_fmt = AV_PIX_FMT_NV12;
        ff_AMediaFormat_setInt32(format, "color-format", 21);
    }"""
if target8 in code:
    code = code.replace(target8, repl8, 1)

# 9. 在 mediacodec_wrap_sw_buffer 中强制输出 NV12 格式，确保正确分配与拷贝内存
target9 = """    frame->width = avctx->width;
    frame->height = avctx->height;
    frame->format = avctx->pix_fmt;"""
repl9 = """    if (avctx->pix_fmt == AV_PIX_FMT_MEDIACODEC) {
        avctx->pix_fmt = AV_PIX_FMT_NV12;
    }
    frame->width = avctx->width;
    frame->height = avctx->height;
    frame->format = AV_PIX_FMT_NV12;"""
if target9 in code:
    code = code.replace(target9, repl9, 1)

with open(c_path, "w") as f:
    f.write(code)
print("MediaCodec 补丁已成功就绪。")
EOF
        (
            cd "${tmp_build}"
            ./configure \
                --prefix="${ROOT_DIR}/android/ffmpeg" \
                --target-os=android \
                --arch=aarch64 \
                --cpu=armv8-a \
                --enable-cross-compile \
                --cross-prefix="${android_ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android-" \
                --cc="${android_ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang" \
                --nm="${android_ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android-nm" \
                --ar="${android_ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android-ar" \
                --ranlib="${android_ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android-ranlib" \
                --strip="${android_ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android-strip" \
                --extra-cflags="-Os -fPIC -ffunction-sections -fdata-sections" \
                --extra-ldflags="-Wl,--gc-sections" \
                --enable-static \
                --disable-shared \
                --enable-pic \
                --disable-programs \
                --disable-doc \
                --disable-avdevice \
                --disable-postproc \
                --disable-avfilter \
                --disable-swresample \
                --disable-encoders \
                --disable-muxers \
                --disable-filters \
                --disable-devices \
                --disable-decoders \
                --enable-jni \
                --enable-mediacodec \
                --enable-decoder=h264_mediacodec,hevc_mediacodec,h264,hevc \
                --disable-demuxers \
                --enable-demuxer=rtsp,h264,hevc,sdp \
                --disable-parsers \
                --enable-parser=h264,hevc \
                --disable-protocols \
                --enable-protocol=tcp,udp,rtp,rtsp \
                --disable-bsfs \
                --enable-bsf=h264_mp4toannexb,hevc_mp4toannexb \
                --enable-network \
                --enable-swscale \
                --disable-debug \
                --enable-small >/dev/null 2>&1
            make -j"${build_jobs}" >/dev/null 2>&1
            make install >/dev/null 2>&1
            "${android_ndk_root}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android-strip" --strip-unneeded "${ROOT_DIR}/android/ffmpeg/lib"/*.a || true
        )
        rm -rf "${tmp_build}"
        echo "Android FFmpeg 静态库构建完成。"
    fi


    echo "=== Android Build Configuration ==="
    echo "  Qt Android : ${qt_android_dir}"
    echo "  Android SDK: ${android_sdk_root}"
    echo "  Android NDK: ${android_ndk_root}"
    echo "  Java Home  : ${java_home}"
    echo "  Target ABI : ${android_abi}"
    echo "  Platform   : ${android_platform}"
    echo "  Build Tools: ${android_build_tools}"
    echo "==================================="

    mkdir -p "${build_dir}"
    cd "${build_dir}"

    local example_config="${ROOT_DIR}/config.yaml.example"
    local local_config="${ROOT_DIR}/config.yaml"
    local backup_config="${ROOT_DIR}/config.yaml.example.tmp_bak"

    cleanup_android_config() {
        if [[ -f "${ROOT_DIR}/config.yaml.example.tmp_bak" ]]; then
            cp -f "${ROOT_DIR}/config.yaml.example.tmp_bak" "${ROOT_DIR}/config.yaml.example"
            rm -f "${ROOT_DIR}/config.yaml.example.tmp_bak"
        fi
    }
    trap cleanup_android_config EXIT INT TERM

    if [[ -f "${local_config}" ]]; then
        echo "检测到本地配置文件 config.yaml，临时注入打包资源..."
        cp -f "${example_config}" "${backup_config}"
        cp -f "${local_config}" "${example_config}"
    fi

    local qt_cmake_bin="${qt_android_dir}/bin/qt-cmake"
    if [[ ! -x "${qt_cmake_bin}" ]]; then
        qt_cmake_bin="cmake"
    fi

    local qt_host_path="${qt_android_dir}/../gcc_64"

    ensure_qsb_shaders

    echo "Configuring project with Qt 6 CMake for Android..."
    "${qt_cmake_bin}" \
        -S "${ROOT_DIR}" \
        -B "${build_dir}" \
        -DQT_HOST_PATH="${qt_host_path}" \
        -DANDROID_ABI="${android_abi}" \
        -DANDROID_PLATFORM="${android_platform}" \
        -DANDROID_SDK_ROOT="${android_sdk_root}" \
        -DANDROID_NDK_ROOT="${android_ndk_root}" \
        -DANDROID_OPENSSL_ROOT="${openssl_dir}" \
        -DCMAKE_BUILD_TYPE=Release

    echo "Compiling and packaging APK with Qt 6 CMake..."
    cmake --build "${build_dir}" --parallel "${build_jobs}" --target apk

    local apk_path
    apk_path="$(find "${build_dir}/android-build/build/outputs/apk" -type f -name "*.apk" 2>/dev/null | head -n 1)"
    local final_apk="${build_dir}/home_gui.apk"
    if [[ -n "${apk_path}" && -f "${apk_path}" ]]; then
        cp -f "${apk_path}" "${final_apk}"
        local apk_size
        apk_size="$(du -h "${final_apk}" | cut -f1)"
        echo
        echo "================================================================"
        echo " Android build succeeded!"
        echo "   Source APK   : ${apk_path}"
        echo "   Final APK    : ${final_apk}"
        echo "   APK Size     : ${apk_size}"
        echo "   Target Arch  : ${android_abi}"
        echo
        echo " To install on your phone via ADB:"
        echo "   adb install -r ${final_apk}"
        echo "================================================================"
        cleanup_android_config
        trap - EXIT INT TERM
    else
        echo "Error: APK was not generated at expected location under ${build_dir}/android-build/build/outputs/apk" >&2
        exit 1
    fi
}

clean_build_dir() {
    local build_dir="$1"
    if [[ -d "${build_dir}" ]]; then
        rm -rf "${build_dir}"
        echo "Removed build directory: ${build_dir}"
    else
        echo "Build directory not found, skipped: ${build_dir}"
    fi
}

run_clean() {
    local target="${2:-all}"
    case "${target}" in
        all)
            clean_build_dir "${ROOT_DIR}/build"
            clean_build_dir "${ROOT_DIR}/build-rk3506"
            clean_build_dir "${ROOT_DIR}/build-android"
            ;;
        native|host|local|linux|macos)
            clean_build_dir "${ROOT_DIR}/build"
            ;;
        rk3506)
            clean_build_dir "${ROOT_DIR}/build-rk3506"
            ;;
        android)
            clean_build_dir "${ROOT_DIR}/build-android"
            ;;
        deps)
            clean_build_dir "${ROOT_DIR}/.deps"
            ;;
        *)
            echo "Unsupported clean target: ${target}" >&2
            echo >&2
            usage
            exit 1
            ;;
    esac
}

case "${PLATFORM}" in
    "")
        choose_platform
        ;;
    -h|--help|help)
        usage
        exit 0
        ;;
esac

case "${PLATFORM}" in
    clean)
        run_clean "$@"
        ;;
    native|host|local)
        run_native_build "native"
        ;;
    linux)
        run_native_build "linux"
        ;;
    macos)
        run_native_build "macos"
        ;;
    rk3506)
        run_rk3506_build "$@"
        ;;
    android)
        run_android_build "$@"
        ;;
    *)
        echo "Unsupported platform: ${PLATFORM}" >&2
        echo >&2
        usage
        exit 1
        ;;
esac
