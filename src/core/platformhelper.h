#ifndef PLATFORMHELPER_H
#define PLATFORMHELPER_H

#include "appconfig.h"

class PlatformHelper
{
public:
    // 根据平台配置设置 Qt 运行时环境变量。
    static void applyEnvironment(const PlatformConfig &config, int argc, char *argv[]);

    // 判断是否在桌面图形环境下运行（X11/Wayland/macOS/Windows）
    static bool isDesktopEnvironment();

    // 动态控制移动端系统状态栏显隐
    static void setStatusBarVisible(bool visible);

    // 查询移动端安全区域内边距（如 iOS 状态栏/灵动岛/Home指示条，非移动端返回 0）
    static qreal safeAreaTop();
    static qreal safeAreaBottom();
    static qreal safeAreaLeft();
    static qreal safeAreaRight();

    // 电饭煲模式与口感中英文双向映射
    static QString cookerModeToSlug(const QString &mode);
    static QString cookerSlugToMode(const QString &slug);
    static QString cookerTasteToSlug(const QString &taste);
    static QString cookerSlugToTaste(const QString &slug);
};

#endif
