#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

#include "appconfig.h"

class ConfigManager;
class GlobalState;
class IModule;
class HomeAssistantModule;
class VideoModule;
class WifiModule;
class XiaozhiModule;

// 模块管理器。
// 统一负责模块创建、启动、配置热重载，以及当前摄像头切换。
class ModuleManager : public QObject
{
    Q_OBJECT

public:
    explicit ModuleManager(ConfigManager *configManager, GlobalState *globalState, QObject *parent = nullptr);
    ~ModuleManager() override;

    void startModules();
    void restartModules(const AppConfig &config);
    int currentCameraIndex() const;
    // 以下接口供前端控制视频相关能力。
    void moveVideoPtz(const QString &direction);
    void selectCamera(const VideoConfig &videoConfig);
    void triggerHomeAssistantAction(const QString &actionName);
    void callHomeAssistantActionService(const QString &actionName, const QString &service);
    void callHomeAssistantCustomService(const QString &domain, const QString &service, const QVariantMap &data = QVariantMap());
    void setHomeAssistantLightBrightness(const QString &actionName, qreal brightness);
    void setHomeAssistantCoverPosition(const QString &actionName, qreal position);
    void setHomeAssistantMediaVolume(const QString &actionName, qreal volume);
    void scanWifiNetworks();
    void connectWifiNetwork(const QString &ssid, const QString &password);
    void forgetWifiNetwork(const QString &ssid);
    void wakeXiaozhi();
    void hideXiaozhiOverlay();

signals:
    void activeCameraChanged(int index);

public slots:
    void reloadModules(const AppConfig &config);

private:
    void ensureModules();
    void applyPlatformState(const AppConfig &config);
    int activeCameraIndex(const AppConfig &config) const;
    VideoConfig activeVideoConfig(const AppConfig &config) const;

    ConfigManager *m_configManager;
    GlobalState *m_globalState;
    QVector<IModule *> m_modules;
    VideoModule *m_videoModule = nullptr;
    HomeAssistantModule *m_homeAssistantModule = nullptr;
    WifiModule *m_wifiModule = nullptr;
    XiaozhiModule *m_xiaozhiModule = nullptr;
    QString m_activeCameraName;
    QString m_activeCameraUrl;
};

#endif
