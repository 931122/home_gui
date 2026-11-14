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
class IntercomModule;

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

    // 小爱家庭传声 (Intercom) 接口
    bool startIntercomRecording();
    void stopIntercomRecording();
    void cancelIntercomRecording();
    void sendIntercomAudio(const QString &roomId, const QString &roomName);
    void loginIntercom(const QString &username, const QString &password);
    void startIntercomQrLogin();
    void cancelIntercomQrLogin();
    bool importIntercomLocalXiaoAiToken();
    void saveIntercomSettings(const QString &userId, const QString &serviceToken, const QString &deviceId, const QString &homeId, const QString &micoapiToken = QString());
    void fetchIntercomRooms();

    // 音箱实时 VoIP 通话控制
    void startVoipCall(const QString &targetDeviceId, const QString &targetDeviceName, const QString &avatar = QString(),
                       const QString &uid = QString(), const QString &deviceModel = QString(), int deviceType = 0,
                       const QString &did = QString());
    void startDirectIntercom(const QString &targetDeviceId, const QString &targetDeviceName, const QString &avatar = QString(),
                             const QString &uid = QString(), const QString &deviceModel = QString(), int deviceType = 0,
                             const QString &did = QString());
    void hangupVoipCall();
    void setVoipMute(bool mute);
    void setVoipSpeakerphone(bool enable);
    void fetchVoipContacts();
    QString voipBackendType() const;
    void setVoipBackendType(const QString &backendType);

signals:
    void activeCameraChanged(int index);
    void intercomLoginResult(bool success, const QString &message);
    void intercomSendAudioResult(bool success, const QString &message);
    void intercomRecordingReadyForRoomSelect(qint64 durationMs);

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
    IntercomModule *m_intercomModule = nullptr;
    QString m_activeCameraName;
    QString m_activeCameraUrl;
};

#endif
