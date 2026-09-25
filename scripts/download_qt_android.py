#!/usr/bin/env python3
"""
Automated downloader and installer for Qt 6.10.1 for Android (arm64-v8a).
Downloads prebuilt Qt Android packages matching host Qt version (6.10.1)
from fast mirrors (USTC / Tsinghua) and extracts them to ~/Qt/6.10.1/android_arm64_v8a.
"""

import os
import sys
import shutil
import subprocess
from pathlib import Path

TARGET_DIR = Path.home() / "Qt" / "6.10.1" / "android_arm64_v8a"
CACHE_DIR = Path("/tmp/qt_android_cache_6101")

MIRROR_PRIMARY = "https://mirrors.ustc.edu.cn/qtproject"
MIRROR_FALLBACK = "https://mirrors.tuna.tsinghua.edu.cn/qt"

PACKAGES = [
    (
        "qtbase",
        "online/qtsdkrepository/all_os/android/qt6_6101/qt6_6101_arm64_v8a/qt.qt6.6101.android_arm64_v8a/6.10.1-0-202511161843qtbase-MacOS-MacOS_14-Clang-Android-Android_ANY-ARM64.7z"
    ),
    (
        "qttools",
        "online/qtsdkrepository/all_os/android/qt6_6101/qt6_6101_arm64_v8a/qt.qt6.6101.android_arm64_v8a/6.10.1-0-202511161843qttools-MacOS-MacOS_14-Clang-Android-Android_ANY-ARM64.7z"
    ),
    (
        "qtsvg",
        "online/qtsdkrepository/all_os/android/qt6_6101/qt6_6101_arm64_v8a/qt.qt6.6101.android_arm64_v8a/6.10.1-0-202511161843qtsvg-MacOS-MacOS_14-Clang-Android-Android_ANY-ARM64.7z"
    ),
    (
        "qttranslations",
        "online/qtsdkrepository/all_os/android/qt6_6101/qt6_6101_arm64_v8a/qt.qt6.6101.android_arm64_v8a/6.10.1-0-202511161843qttranslations-MacOS-MacOS_14-Clang-Android-Android_ANY-ARM64.7z"
    ),
    (
        "qtdeclarative",
        "online/qtsdkrepository/all_os/android/qt6_6101/qt6_6101_arm64_v8a/qt.qt6.6101.android_arm64_v8a/6.10.1-0-202511161843qtdeclarative-MacOS-MacOS_14-Clang-Android-Android_ANY-ARM64.7z"
    ),
    (
        "qtshadertools",
        "online/qtsdkrepository/all_os/android/qt6_6101/qt6_6101_arm64_v8a/qt.qt6.6101.addons.qtshadertools.android_arm64_v8a/6.10.1-0-202511161843qtshadertools-MacOS-MacOS_14-Clang-Android-Android_ANY-ARM64.7z"
    ),
    (
        "qtwebsockets",
        "online/qtsdkrepository/all_os/android/qt6_6101/qt6_6101_arm64_v8a/qt.qt6.6101.addons.qtwebsockets.android_arm64_v8a/6.10.1-0-202511161843qtwebsockets-MacOS-MacOS_14-Clang-Android-Android_ANY-ARM64.7z"
    ),
]

def check_dependencies():
    seven_z = shutil.which("7z") or shutil.which("7za") or "/usr/local/bin/7z"
    if not os.path.exists(seven_z):
        print(f"Error: 7z executable not found. Please install via 'brew install sevenzip'.", file=sys.stderr)
        sys.exit(1)
    return seven_z

def download_package(name: str, rel_path: str) -> Path:
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    out_file = CACHE_DIR / f"{name}.7z"

    # Check if already cached and valid
    if out_file.exists():
        check = subprocess.run(["7z", "t", str(out_file)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if check.returncode == 0:
            print(f"[+] Found cached {out_file.name} (integrity verified)")
            return out_file
        else:
            print(f"[!] Cached {out_file.name} corrupted, redownloading...")
            out_file.unlink(missing_ok=True)

    mirrors = [MIRROR_PRIMARY, MIRROR_FALLBACK]
    downloaded = False

    for mirror in mirrors:
        url = f"{mirror}/{rel_path}"
        print(f"[*] Downloading {name} from {mirror} ...")
        cmd = [
            "curl", "-fSL",
            "--progress-bar",
            "--connect-timeout", "15",
            "--retry", "3",
            "--retry-delay", "2",
            "-C", "-",
            "-A", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36",
            "-o", str(out_file),
            url
        ]
        ret = subprocess.run(cmd)
        if ret.returncode == 0 and out_file.exists():
            # Validate 7z integrity
            check = subprocess.run(["7z", "t", str(out_file)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if check.returncode == 0:
                downloaded = True
                break
            else:
                print(f"[!] {name}.7z failed archive check, removing and retrying...", file=sys.stderr)
                out_file.unlink(missing_ok=True)

    if not downloaded:
        print(f"Error: Failed to download {name} from all mirrors.", file=sys.stderr)
        sys.exit(1)

    return out_file

def extract_package(seven_z: str, archive_path: Path):
    print(f"[*] Extracting {archive_path.name} into {TARGET_DIR} ...")
    cmd = [seven_z, "x", "-y", str(archive_path), f"-o{TARGET_DIR}"]
    subprocess.check_call(cmd, stdout=subprocess.DEVNULL)

def post_process():
    print("[*] Running post-processing for Qt 6 Android ...")
    TARGET_DIR.mkdir(parents=True, exist_ok=True)
    bin_dir = TARGET_DIR / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)

    # 1. Ensure target_qt.conf and qt.conf
    target_conf = bin_dir / "target_qt.conf"
    target_conf.write_text("[Paths]\nPrefix=..\n", encoding="utf-8")
    qt_conf = bin_dir / "qt.conf"
    qt_conf.write_text("[Paths]\nPrefix=..\n", encoding="utf-8")

    # 2. Make bin/qt-cmake, qmake, qtpaths executable if present
    for executable_name in ["qt-cmake", "qmake", "qmake6", "qtpaths", "qtpaths6", "androiddeployqt", "androidtestrunner"]:
        p = bin_dir / executable_name
        if p.exists():
            p.chmod(p.stat().st_mode | 0o755)

    # 3. Patch qconfig.pri with OpenSource edition
    qconfig_pri = TARGET_DIR / "mkspecs" / "qconfig.pri"
    if qconfig_pri.exists():
        lines = qconfig_pri.read_text(encoding="utf-8").splitlines()
        new_lines = []
        for line in lines:
            if line.startswith("QT_EDITION ="):
                new_lines.append("QT_EDITION = OpenSource")
            elif line.startswith("QT_LICHECK ="):
                new_lines.append("QT_LICHECK =")
            else:
                new_lines.append(line)
        qconfig_pri.write_text("\n".join(new_lines) + "\n", encoding="utf-8")

def main():
    print("================================================================")
    print(" Installing Qt 6.10.1 for Android (arm64-v8a) into:")
    print(f"   {TARGET_DIR}")
    print("================================================================")
    seven_z = check_dependencies()

    TARGET_DIR.mkdir(parents=True, exist_ok=True)

    for name, rel_path in PACKAGES:
        archive_path = download_package(name, rel_path)
        extract_package(seven_z, archive_path)

    post_process()

    print()
    print("================================================================")
    print(" Qt 6.10.1 for Android (arm64-v8a) installed successfully!")
    print(f" Location: {TARGET_DIR}")
    print("================================================================")

if __name__ == "__main__":
    main()
