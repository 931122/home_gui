#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLibraryInfo>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QTranslator>
#include <qqml.h>

#include <functional>
#include <memory>

#include "core/configmanager.h"
#include "core/globalstate.h"
#include "core/modulemanager.h"
#include "core/platformhelper.h"
#include "core/tstranslator.h"
#include "ui/appcontroller.h"
#include "ui/bootsplash.h"
#include "liquidglass/glassruntime.h"
#include "ui/videoitem.h"

#if defined(Q_OS_ANDROID)
#include <QtCore/QJniObject>
#include <QtCore/QJniEnvironment>
#include <QtCore/qnativeinterface.h>

static void setupAndroidImmersiveMode()
{
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (!activity.isValid()) {
        return;
    }

    QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
    if (!window.isValid()) {
        return;
    }

    const int sdkInt = QNativeInterface::QAndroidApplication::sdkVersion();
    if (sdkInt >= 28) {
        QJniObject layoutParams = window.callObjectMethod("getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
        if (layoutParams.isValid()) {
            layoutParams.setField<jint>("layoutInDisplayCutoutMode", 1);
            window.callMethod<void>("setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V", layoutParams.object<jobject>());
        }
    }

    const jint FLAG_FULLSCREEN = 1024;
    const jint FLAG_KEEP_SCREEN_ON = 128;
    window.callMethod<void>("addFlags", "(I)V", FLAG_FULLSCREEN | FLAG_KEEP_SCREEN_ON);

    QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
    if (decorView.isValid()) {
        const jint SYSTEM_UI_FLAG_LAYOUT_STABLE = 0x00000100;
        const jint SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = 0x00000200;
        const jint SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = 0x00000400;
        const jint SYSTEM_UI_FLAG_HIDE_NAVIGATION = 0x00000002;
        const jint SYSTEM_UI_FLAG_FULLSCREEN = 0x00000400;
        const jint SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 0x00001000;

        const jint immersiveFlags = SYSTEM_UI_FLAG_LAYOUT_STABLE
                                  | SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                  | SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                                  | SYSTEM_UI_FLAG_HIDE_NAVIGATION
                                  | SYSTEM_UI_FLAG_FULLSCREEN
                                  | SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

        decorView.callMethod<void>("setSystemUiVisibility", "(I)V", immersiveFlags);
    }
}
#endif

// 根据平台确定配置文件路径，在 Android 等沙盒环境中自动从资源释放到可写目录
static QString resolveConfigPath()
{
#if defined(Q_OS_ANDROID)
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
    const QString writableConfigPath = appDataDir + QStringLiteral("/config.yaml");

    ConfigManager::ensureConfigFile(writableConfigPath, QStringLiteral(":/config.yaml"));
    return writableConfigPath;
#else
    const QString localConfig = QStringLiteral("config.yaml");
    if (QFile::exists(localConfig)) {
        return localConfig;
    }
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString writableConfigPath = appDataDir + QStringLiteral("/config.yaml");
    if (QFile::exists(writableConfigPath)) {
        return writableConfigPath;
    }
    const QString exampleConfig = QStringLiteral("config.yaml.example");
    if (QFile::exists(exampleConfig)) {
        QFile sourceFile(exampleConfig);
        QDir().mkpath(QFileInfo(writableConfigPath).absolutePath());
        QFile targetFile(writableConfigPath);
        if (sourceFile.open(QIODevice::ReadOnly) && targetFile.open(QIODevice::WriteOnly)) {
            const QByteArray content = sourceFile.readAll();
            const bool written = targetFile.write(content) == content.size();
            targetFile.close();
            sourceFile.close();
            if (written) {
                return writableConfigPath;
            }
        }
    }
    return localConfig;
#endif
}

// 启动前先从配置文件里拿到平台相关参数，
// 这样可以在 QApplication 创建前设置合适的 Qt 运行环境。
static PlatformConfig loadBootPlatformConfig(const QString &configPath)
{
    return ConfigManager::loadBootPlatformConfig(configPath);
}

static void configureApplicationFont()
{
    QString mainFontFamily;

    // 1. 优先检查已知的高质量中文字体路径
    static const QStringList priorityFonts = {
        // Android 系统默认字体
        QStringLiteral("/system/fonts/NotoSansCJK-Regular.ttc"),
        QStringLiteral("/system/fonts/NotoSansSC-Regular.otf"),
        QStringLiteral("/system/fonts/SourceHanSansCN-Regular.otf"),
        // 嵌入式 Linux 与桌面 Linux 字体
        QStringLiteral("/usr/share/fonts/opentype/adobe-source-han-sans-cn/SourceHanSansCN-Regular.otf"),
        QStringLiteral("/usr/share/fonts/opentype/source-han-sans-cn/SourceHanSansCN-Regular.otf"),
        QStringLiteral("/usr/share/fonts/source-han-sans-cn/SourceHanSansCN-Regular.otf"),
        QStringLiteral("/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc"),
        QStringLiteral("/usr/share/fonts/wqy-zenhei/wqy-zenhei.ttc"),
        QStringLiteral("/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"),
        QStringLiteral("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc")
    };

    for (const QString &fontFile : priorityFonts) {
        if (!QFile::exists(fontFile)) {
            continue;
        }

        const int fontId = QFontDatabase::addApplicationFont(fontFile);
        if (fontId >= 0) {
            const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                mainFontFamily = families.first();
                break;
            }
        }
    }

    // 若未命中首选字体，则进行兜底扫描（涵盖 Linux 和 Android 字体目录）
    if (mainFontFamily.isEmpty()) {
        const QStringList scanDirs = { QStringLiteral("/usr/share/fonts"), QStringLiteral("/system/fonts") };
        for (const QString &scanDir : scanDirs) {
            if (!QDir(scanDir).exists()) continue;
            QDirIterator iterator(scanDir,
                                  QStringList() << QStringLiteral("*.ttf")
                                                << QStringLiteral("*.ttc")
                                                << QStringLiteral("*.otf"),
                                  QDir::Files,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                const QString filePath = iterator.next();
                const QString lowerPath = filePath.toLower();
                if (lowerPath.contains(QStringLiteral("sourcehan"))
                    || lowerPath.contains(QStringLiteral("source-han"))
                    || lowerPath.contains(QStringLiteral("notosanscjk"))
                    || lowerPath.contains(QStringLiteral("notosanssc"))
                    || lowerPath.contains(QStringLiteral("cjk"))
                    || lowerPath.contains(QStringLiteral("wqy"))
                    || lowerPath.contains(QStringLiteral("wenquanyi"))) {
                    const int fontId = QFontDatabase::addApplicationFont(filePath);
                    if (fontId >= 0) {
                        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
                        if (!families.isEmpty()) {
                            mainFontFamily = families.first();
                            break;
                        }
                    }
                }
            }
            if (!mainFontFamily.isEmpty()) break;
        }
    }

    // 2. 检查并加载 Emoji 与 Unicode 符号字体（让 Qt 的 Fallback 机制能够解析表情符号）
    static const QStringList priorityEmojiFonts = {
        // 内置跨平台 Symbola 字体（确保 Android、RK3506 及各 Linux 环境均能正常渲染表情符号）
        QStringLiteral(":/fonts/Symbola.ttf"),
        // Android 系统 Emoji
        QStringLiteral("/system/fonts/NotoColorEmoji.ttf"),
        // 系统 noto / emoji 字体
        QStringLiteral("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf"),
        QStringLiteral("/usr/share/fonts/noto/NotoColorEmoji.ttf"),
        QStringLiteral("/usr/share/fonts/truetype/noto/NotoEmoji-Regular.ttf"),
        QStringLiteral("/usr/share/fonts/noto/NotoEmoji-Regular.ttf"),
        QStringLiteral("/usr/share/fonts/truetype/symbola/Symbola.ttf"),
        QStringLiteral("/usr/share/fonts/symbola/Symbola.ttf"),
        QStringLiteral("/usr/share/fonts/NotoColorEmoji.ttf"),
        QStringLiteral("/usr/share/fonts/NotoEmoji-Regular.ttf"),
        QStringLiteral("/usr/share/fonts/Symbola.ttf"),
        // 应用程序自带 fonts/ 目录
        QStringLiteral("./fonts/NotoColorEmoji.ttf"),
        QStringLiteral("./fonts/NotoEmoji-Regular.ttf"),
        QStringLiteral("./fonts/Symbola.ttf")
    };

    QStringList emojiFamilies;
    for (const QString &emojiFile : priorityEmojiFonts) {
        if (QFile::exists(emojiFile)) {
            const int fontId = QFontDatabase::addApplicationFont(emojiFile);
            if (fontId >= 0) {
                const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
                for (const QString &f : families) {
                    if (!emojiFamilies.contains(f)) {
                        emojiFamilies.append(f);
                    }
                }
            }
        }
    }

    // 如果未命中预设路径，补充扫描 /usr/share/fonts 下的 emoji/symbol 字体
    if (emojiFamilies.isEmpty()) {
        QDirIterator emojiIter(QStringLiteral("/usr/share/fonts"),
                               QStringList() << QStringLiteral("*.ttf") << QStringLiteral("*.otf"),
                               QDir::Files,
                               QDirIterator::Subdirectories);
        while (emojiIter.hasNext()) {
            const QString filePath = emojiIter.next();
            const QString lowerPath = filePath.toLower();
            if (lowerPath.contains(QStringLiteral("emoji")) || lowerPath.contains(QStringLiteral("symbol"))) {
                const int fontId = QFontDatabase::addApplicationFont(filePath);
                if (fontId >= 0) {
                    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
                    for (const QString &f : families) {
                        if (!emojiFamilies.contains(f)) {
                            emojiFamilies.append(f);
                        }
                    }
                }
            }
        }
    }

    // 3. 应用字体设置并注册 Emoji Fallback 替换链
    QFont appFont = QGuiApplication::font();
    if (!mainFontFamily.isEmpty()) {
        appFont.setFamily(mainFontFamily);
    } else {
        mainFontFamily = appFont.family();
    }
    appFont.setStyleHint(QFont::SansSerif);

    if (!emojiFamilies.isEmpty()) {
        const QStringList commonBaseFamilies = {
            mainFontFamily,
            QStringLiteral("sans-serif"),
            QStringLiteral("Roboto"),
            QStringLiteral("Droid Sans"),
            QStringLiteral("Noto Sans"),
            QStringLiteral("Noto Sans CJK SC"),
            QStringLiteral("MiSans"),
            QStringLiteral("HarmonyOS Sans"),
            QStringLiteral("OPlusSans"),
            QStringLiteral("PingFang SC")
        };
        for (const QString &baseFamily : commonBaseFamilies) {
            if (!baseFamily.isEmpty()) {
                QFont::insertSubstitutions(baseFamily, emojiFamilies);
            }
        }
    }

    QGuiApplication::setFont(appFont);
}

static void installChineseTranslators(QGuiApplication &app)
{
    static QTranslator qtTranslator;
    static TsTranslator appTranslator;

    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));

    const QString qtTranslationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (!qtTranslator.load(QStringLiteral("qt_zh_CN"), qtTranslationsPath)) {
        // qWarning() << "Failed to load Qt translator from" << qtTranslationsPath;
    } else {
        app.installTranslator(&qtTranslator);
    }

    if (!appTranslator.loadFromTsFile(QStringLiteral(":/i18n/home_gui_zh_CN.ts"))) {
        // qWarning() << "Failed to load application translator";
    } else {
        app.installTranslator(&appTranslator);
    }
}

int main(int argc, char *argv[])
{
    // 注册给 queued signal/slot 和 QML 使用的自定义类型。
    qRegisterMetaType<PlatformConfig>("PlatformConfig");
    qRegisterMetaType<VideoConfig>("VideoConfig");
    qRegisterMetaType<HomeAssistantActionConfig>("HomeAssistantActionConfig");
    qRegisterMetaType<HomeAssistantConfig>("HomeAssistantConfig");
    qRegisterMetaType<WeatherConfig>("WeatherConfig");
    qRegisterMetaType<WifiConfig>("WifiConfig");
    qRegisterMetaType<XiaozhiConfig>("XiaozhiConfig");
    qmlRegisterType<VideoItem>("HomeGui", 1, 0, "VideoItem");
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qt/qml/HomeGui/LiquidGlass/qml/GlassTheme.qml")), "HomeGui", 1, 0, "Theme");

    const QString configPath = resolveConfigPath();
    const PlatformConfig bootPlatformConfig = loadBootPlatformConfig(configPath);
    PlatformHelper::applyEnvironment(bootPlatformConfig, argc, argv);
#if !defined(Q_OS_ANDROID)
    // 先直接写 framebuffer，覆盖 Qt 平台插件初始化前的空窗期。
    drawEarlyFramebufferSplash(bootPlatformConfig);
#endif

    QGuiApplication app(argc, argv);
#if !defined(Q_OS_ANDROID)
    // linuxfb 插件接管 framebuffer 时可能清屏，初始化后马上再刷一次。
    drawEarlyFramebufferSplash(bootPlatformConfig);
#endif
    app.setApplicationName(QStringLiteral("home_gui"));
    app.setOrganizationName(QStringLiteral("home_gui"));
#if !defined(Q_OS_ANDROID)
    std::unique_ptr<BootSplash> bootSplash(new BootSplash(bootPlatformConfig));
#else
    std::unique_ptr<BootSplash> bootSplash = nullptr;
#endif
    installChineseTranslators(app);
    configureApplicationFont();

    ConfigManager configManager(configPath);
    if (!configManager.load()) {
        qCritical() << "Failed to load config file:" << configPath;
        return 1;
    }

    GlobalState globalState;
    GlassRuntime glassRuntime;
    ModuleManager moduleManager(&configManager, &globalState);
    AppController controller(&configManager, &globalState, &moduleManager);

    // 配置热更新后，同时通知后端模块和前端控制器刷新状态。
    QObject::connect(&configManager, &ConfigManager::configReloaded,
                     &moduleManager, &ModuleManager::reloadModules);
    QObject::connect(&configManager, &ConfigManager::configReloaded,
                     &controller, &AppController::onConfigReloaded);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
                     [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings) {
            qWarning().noquote() << warning.toString();
        }
    });
    // QML 通过这两个上下文对象读取状态并发出控制命令。
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("globalState"), &globalState);
    engine.rootContext()->setContextProperty(QStringLiteral("glassRuntime"), &glassRuntime);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "QML engine did not create any root objects";
        return 2;
    }
    if (bootSplash) {
        bootSplash->raise();
    }

    bool deferredStarted = false;
    std::function<void()> startDeferredWork;
    startDeferredWork = [&deferredStarted, &controller, &moduleManager, &bootSplash, &startDeferredWork]() {
        if (deferredStarted) {
            return;
        }
        if (bootSplash && !bootSplash->minimumVisibleElapsed(900)) {
            QTimer::singleShot(120, qApp, startDeferredWork);
            return;
        }
        deferredStarted = true;
        if (bootSplash) {
            bootSplash->close();
            bootSplash.reset();
        }
#if defined(Q_OS_ANDROID)
        QNativeInterface::QAndroidApplication::hideSplashScreen(300);
        setupAndroidImmersiveMode();
#endif
        controller.startDeferredServices();
        moduleManager.startModules();
    };

    // 等第一帧真正提交后再创建 worker 线程和启动网络/视频模块，避免 RK3506 启动时短暂黑屏。
    QQuickWindow *rootWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (rootWindow != nullptr) {
#if defined(Q_OS_ANDROID)
        setupAndroidImmersiveMode();
        rootWindow->showFullScreen();
#else
        if (PlatformHelper::isDesktopEnvironment()) {
            rootWindow->setTitle(QStringLiteral("智能中控 (800x480)"));
            if (QScreen *screen = QGuiApplication::primaryScreen()) {
                const QRect avail = screen->availableGeometry();
                const int x = avail.x() + qMax(0, (avail.width() - rootWindow->width()) / 2);
                const int y = avail.y() + qMax(0, (avail.height() - rootWindow->height()) / 2);
                rootWindow->setX(x);
                rootWindow->setY(y);
            }
            rootWindow->show();
            rootWindow->raise();
        }
#endif

        auto frameConnection = std::make_shared<QMetaObject::Connection>();
        *frameConnection = QObject::connect(rootWindow, &QQuickWindow::frameSwapped,
                                            &app, [frameConnection, &startDeferredWork]() {
            QObject::disconnect(*frameConnection);
            startDeferredWork();
        });
    }
    // 兜底：如果 linuxfb 没发 frameSwapped，也不能让后端模块一直不启动。
    QTimer::singleShot(700, &app, startDeferredWork);

    return app.exec();
}
