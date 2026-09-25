#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>

#include "appconfig.h"

class QFileSystemWatcher;

// 配置管理器。
// 负责加载 config.yaml，并在文件变化时触发热重载。
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(const QString &configPath, QObject *parent = nullptr);

    bool load();
    const AppConfig &config() const;
    QString configPath() const;
    bool isLoaded() const;
    bool switchConfigFile(const QString &newPath);
    void resetConfig();
    bool updateCameraOnvifProfile(int cameraIndex, const QString &profile);
    bool updateScreenPowerSettings(int idleTimeoutSeconds, const QString &panelConfig);

    // 静态辅助方法：启动引导阶段（QApplication 创建前）快速读取平台配置
    static PlatformConfig loadBootPlatformConfig(const QString &configPath);
    // 确保可写配置存在
    static void ensureConfigFile(const QString &writableConfigPath, const QString &builtinResourcePath);

signals:
    void configReloaded(const AppConfig &config);
    void configLoadFailed(const QString &message);

private slots:
    void reloadFromDisk();

private:
    // 解析整个配置文件内容。
    bool parseConfig(const QByteArray &payload, AppConfig &nextConfig, QString &errorMessage) const;
    // 确保配置文件已加入文件监听。
    void ensureWatch();

    QString m_configPath;
    AppConfig m_config;
    QFileSystemWatcher *m_watcher;
    bool m_isLoaded = false;
    bool m_ignoreNextFileChange = false;
};

#endif
