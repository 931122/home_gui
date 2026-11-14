#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QElapsedTimer>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QTimer>
#include <QTime>
#include <QUrl>
#include <memory>

#include "core/appconfig.h"
#include "ui/screenpowermanager.h"

class ConfigManager;
class GlobalState;
class ModuleManager;
class BrightnessController;
class WeatherService;
class HolidayService;

// 前端控制层：
// 负责把 ConfigManager / GlobalState / ModuleManager 这三层桥接给 QML。
class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl videoSource READ videoSource NOTIFY videoSourceChanged)
    Q_PROPERTY(bool videoEnabled READ videoEnabled NOTIFY videoSourceChanged)
    Q_PROPERTY(QVariantList cameraPreviewModels READ cameraPreviewModels NOTIFY cameraListChanged)
    Q_PROPERTY(QVariantList haActionModels READ haActionModels NOTIFY haActionModelsChanged)
    Q_PROPERTY(QStringList haActionNames READ haActionNames NOTIFY haActionNamesChanged)
    Q_PROPERTY(int currentCameraIndex READ currentCameraIndex NOTIFY currentCameraIndexChanged)
    Q_PROPERTY(QString videoBackend READ videoBackend NOTIFY videoSourceChanged)
    Q_PROPERTY(QString videoDecoder READ videoDecoder NOTIFY videoSourceChanged)
    Q_PROPERTY(bool onvifProfileSwitchSupported READ onvifProfileSwitchSupported NOTIFY videoSourceChanged)
    Q_PROPERTY(QString onvifCurrentProfile READ onvifCurrentProfile NOTIFY videoSourceChanged)
    Q_PROPERTY(QString currentDateText READ currentDateText NOTIFY currentTimeChanged)
    Q_PROPERTY(QString currentClockText READ currentClockText NOTIFY currentTimeChanged)
    Q_PROPERTY(QString currentDateBadge READ currentDateBadge NOTIFY currentTimeChanged)
    Q_PROPERTY(QString weatherSummary READ weatherSummary NOTIFY weatherSummaryChanged)
    Q_PROPERTY(QString weatherLocation READ weatherLocation NOTIFY weatherSummaryChanged)
    Q_PROPERTY(QString weatherDetail READ weatherDetail NOTIFY weatherSummaryChanged)
    Q_PROPERTY(QVariantList weatherForecast READ weatherForecast NOTIFY weatherSummaryChanged)
    Q_PROPERTY(QVariantMap weatherCurrent READ weatherCurrent NOTIFY weatherSummaryChanged)
    Q_PROPERTY(qreal brightness READ brightness NOTIFY brightnessChanged)
    Q_PROPERTY(bool hardwareBrightnessAvailable READ hardwareBrightnessAvailable NOTIFY brightnessChanged)
    Q_PROPERTY(bool wifiAvailable READ wifiAvailable NOTIFY wifiStateChanged)
    Q_PROPERTY(bool wifiScanning READ wifiScanning NOTIFY wifiStateChanged)
    Q_PROPERTY(QString wifiInterface READ wifiInterface NOTIFY wifiStateChanged)
    Q_PROPERTY(QString wifiStatus READ wifiStatus NOTIFY wifiStateChanged)
    Q_PROPERTY(QVariantList wifiNetworks READ wifiNetworks NOTIFY wifiStateChanged)
    Q_PROPERTY(bool networkOnline READ networkOnline NOTIFY networkOnlineChanged)
    Q_PROPERTY(bool screenBlanked READ screenBlanked NOTIFY screenBlankedChanged)
    Q_PROPERTY(ScreenPowerManager* screenPower READ screenPower CONSTANT)
    Q_PROPERTY(bool isScreenOff READ isScreenOff NOTIFY screenOffChanged)
    Q_PROPERTY(QString screenPanelType READ screenPanelType NOTIFY screenPanelTypeChanged)
    Q_PROPERTY(QString screenPanelConfig READ screenPanelConfig WRITE setScreenPanelConfig NOTIFY screenPanelConfigChanged)
    Q_PROPERTY(int screenIdleSeconds READ screenIdleSeconds WRITE setScreenIdleSeconds NOTIFY screenIdleSecondsChanged)
    Q_PROPERTY(bool humanPresenceDetected READ humanPresenceDetected NOTIFY humanPresenceChanged)
    Q_PROPERTY(bool steamerRunning READ steamerRunning NOTIFY steamerStateChanged)
    Q_PROPERTY(int steamerRemainSeconds READ steamerRemainSeconds NOTIFY steamerStateChanged)
    Q_PROPERTY(QString steamerDishName READ steamerDishName NOTIFY steamerStateChanged)
    Q_PROPERTY(int steamerTotalMinutes READ steamerTotalMinutes NOTIFY steamerStateChanged)
    Q_PROPERTY(bool steamerSocketState READ steamerSocketState NOTIFY steamerSocketStateChanged)
    Q_PROPERTY(QString steamerMode READ steamerMode NOTIFY steamerStateChanged)
    Q_PROPERTY(QStringList steamerModeOptions READ steamerModeOptions NOTIFY steamerStateChanged)
    Q_PROPERTY(QVariantList steamerPresetModels READ steamerPresetModels NOTIFY steamerStateChanged)
    Q_PROPERTY(QString videoBottomCardMode READ videoBottomCardMode WRITE setVideoBottomCardMode NOTIFY videoBottomCardModeChanged)
    Q_PROPERTY(QString configFilePath READ configFilePath NOTIFY configFilePathChanged)
    Q_PROPERTY(bool configLoaded READ configLoaded NOTIFY configLoadedChanged)
    Q_PROPERTY(QString configStatusText READ configStatusText NOTIFY configStatusTextChanged)
    Q_PROPERTY(QStringList candidateConfigFiles READ candidateConfigFiles NOTIFY candidateConfigFilesChanged)
    Q_PROPERTY(bool isAndroid READ isAndroid CONSTANT)
    Q_PROPERTY(bool isIos READ isIos CONSTANT)
    Q_PROPERTY(bool isMobile READ isMobile CONSTANT)
    Q_PROPERTY(qreal safeAreaTop READ safeAreaTop NOTIFY safeAreaChanged)
    Q_PROPERTY(qreal safeAreaBottom READ safeAreaBottom NOTIFY safeAreaChanged)
    Q_PROPERTY(qreal safeAreaLeft READ safeAreaLeft NOTIFY safeAreaChanged)
    Q_PROPERTY(qreal safeAreaRight READ safeAreaRight NOTIFY safeAreaChanged)
    Q_PROPERTY(bool intercomAvailable READ intercomAvailable NOTIFY intercomStateChanged)
    Q_PROPERTY(QString intercomStatus READ intercomStatus NOTIFY intercomStateChanged)
    Q_PROPERTY(QString intercomUserId READ intercomUserId NOTIFY intercomConfigChanged)
    Q_PROPERTY(QString intercomDeviceId READ intercomDeviceId NOTIFY intercomConfigChanged)
    Q_PROPERTY(QString intercomServiceToken READ intercomServiceToken NOTIFY intercomConfigChanged)
    Q_PROPERTY(QString micoapiServiceToken READ micoapiServiceToken NOTIFY intercomConfigChanged)
    Q_PROPERTY(QString intercomHomeId READ intercomHomeId NOTIFY intercomConfigChanged)
    Q_PROPERTY(QVariantList intercomRooms READ intercomRooms NOTIFY intercomRoomsChanged)
    Q_PROPERTY(bool intercomRecording READ intercomRecording NOTIFY intercomRecordingChanged)
    Q_PROPERTY(int intercomRecordDuration READ intercomRecordDuration NOTIFY intercomRecordingChanged)
    Q_PROPERTY(qreal intercomAudioLevel READ intercomAudioLevel NOTIFY intercomAudioLevelChanged)
    Q_PROPERTY(QString intercomQrImageUrl READ intercomQrImageUrl NOTIFY intercomQrLoginChanged)
    Q_PROPERTY(QString intercomQrWebUrl READ intercomQrWebUrl NOTIFY intercomQrLoginChanged)
    Q_PROPERTY(QString intercomQrLoginStatus READ intercomQrLoginStatus NOTIFY intercomQrLoginChanged)
    Q_PROPERTY(bool intercomQrLoggingIn READ intercomQrLoggingIn NOTIFY intercomQrLoginChanged)
    Q_PROPERTY(bool voipAvailable READ voipAvailable NOTIFY voipAvailableChanged)
    Q_PROPERTY(bool voipActive READ voipActive NOTIFY voipStateChanged)
    Q_PROPERTY(int voipState READ voipState NOTIFY voipStateChanged)
    Q_PROPERTY(QString voipStatusText READ voipStatusText NOTIFY voipStateChanged)
    Q_PROPERTY(QString voipTargetDeviceId READ voipTargetDeviceId NOTIFY voipTargetChanged)
    Q_PROPERTY(QString voipTargetDeviceName READ voipTargetDeviceName NOTIFY voipTargetChanged)
    Q_PROPERTY(QString voipTargetAvatar READ voipTargetAvatar NOTIFY voipTargetChanged)
    Q_PROPERTY(int voipDurationSeconds READ voipDurationSeconds NOTIFY voipDurationChanged)
    Q_PROPERTY(QString voipDurationText READ voipDurationText NOTIFY voipDurationChanged)
    Q_PROPERTY(bool voipMuted READ voipMuted NOTIFY voipAudioSettingsChanged)
    Q_PROPERTY(bool voipSpeakerphone READ voipSpeakerphone NOTIFY voipAudioSettingsChanged)
    Q_PROPERTY(QVariantList voipSpeakers READ voipSpeakers NOTIFY voipSpeakersChanged)
    Q_PROPERTY(QString voipBackendType READ voipBackendType NOTIFY voipBackendTypeChanged)

public:
    explicit AppController(ConfigManager *configManager,
                           GlobalState *globalState,
                           ModuleManager *moduleManager,
                           QObject *parent = nullptr);
    ~AppController() override;

    QUrl videoSource() const;
    bool videoEnabled() const;
    QVariantList cameraPreviewModels() const;
    QVariantList haActionModels() const;
    QStringList haActionNames() const;
    int currentCameraIndex() const;
    QString videoBackend() const;
    QString videoDecoder() const;
    bool onvifProfileSwitchSupported() const;
    QString onvifCurrentProfile() const;
    QString currentDateText() const;
    QString currentClockText() const;
    QString currentDateBadge() const;
    QString weatherSummary() const;
    QString weatherLocation() const;
    QString weatherDetail() const;
    QVariantList weatherForecast() const;
    QVariantMap weatherCurrent() const;
    qreal brightness() const;
    bool hardwareBrightnessAvailable() const;
    bool wifiAvailable() const;
    bool wifiScanning() const;
    QString wifiInterface() const;
    QString wifiStatus() const;
    QVariantList wifiNetworks() const;
    bool networkOnline() const;
    bool screenBlanked() const;
    bool isScreenOff() const;
    QString screenPanelType() const;
    QString screenPanelConfig() const;
    Q_INVOKABLE void setScreenPanelConfig(const QString &config);
    int screenIdleSeconds() const;
    Q_INVOKABLE void setScreenIdleSeconds(int seconds);
    bool humanPresenceDetected() const;

    Q_INVOKABLE void requestWake(const QString &source = QStringLiteral("touch"));
    Q_INVOKABLE void requestSleep();
    Q_INVOKABLE void reportHumanPresence(bool detected);

    bool steamerRunning() const { return m_steamerRunning; }
    int steamerRemainSeconds() const { return m_steamerRemainSeconds; }
    QString steamerDishName() const { return m_steamerDishName; }
    int steamerTotalMinutes() const { return m_steamerTotalMinutes; }
    bool steamerSocketState() const { return m_steamerSocketState; }
    QString steamerMode() const { return m_steamerMode; }
    QStringList steamerModeOptions() const { return m_steamerModeOptions; }
    QVariantList steamerPresetModels() const { return m_steamerPresetModels; }
    QString videoBottomCardMode() const { return m_videoBottomCardMode; }
    Q_INVOKABLE void setVideoBottomCardMode(const QString &mode);
    bool isAndroid() const {
#if defined(Q_OS_ANDROID)
        return true;
#else
        return false;
#endif
    }
    bool isIos() const {
#if defined(Q_OS_IOS)
        return true;
#else
        return false;
#endif
    }
    bool isMobile() const {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        return true;
#else
        return false;
#endif
    }
    ScreenPowerManager *screenPower() const;

    bool intercomAvailable() const;
    QString intercomStatus() const;
    QString intercomUserId() const;
    QString intercomDeviceId() const;
    QString intercomServiceToken() const;
    QString micoapiServiceToken() const;
    QString intercomHomeId() const;
    QVariantList intercomRooms() const;
    bool intercomRecording() const;
    int intercomRecordDuration() const;
    qreal intercomAudioLevel() const;
    QString intercomQrImageUrl() const;
    QString intercomQrWebUrl() const;
    QString intercomQrLoginStatus() const;
    bool intercomQrLoggingIn() const;
    bool voipAvailable() const;
    bool voipActive() const;
    int voipState() const;
    QString voipStatusText() const;
    QString voipTargetDeviceId() const;
    QString voipTargetDeviceName() const;
    QString voipTargetAvatar() const;
    int voipDurationSeconds() const;
    QString voipDurationText() const;
    bool voipMuted() const;
    bool voipSpeakerphone() const;
    QVariantList voipSpeakers() const;
    QString voipBackendType() const;

    Q_INVOKABLE bool startIntercomRecord();
    Q_INVOKABLE void stopIntercomRecord();
    Q_INVOKABLE void cancelIntercomRecord();
    Q_INVOKABLE void sendIntercomAudio(const QString &roomId, const QString &roomName);
    Q_INVOKABLE void loginIntercom(const QString &username, const QString &password);
    Q_INVOKABLE void startIntercomQrLogin();
    Q_INVOKABLE void cancelIntercomQrLogin();
    Q_INVOKABLE bool importLocalXiaoAiToken();
    Q_INVOKABLE void saveIntercomSettings(const QString &userId, const QString &serviceToken, const QString &deviceId, const QString &homeId, const QString &micoapiToken = QString());
    Q_INVOKABLE void refreshIntercomRooms();
    Q_INVOKABLE void startVoipCall(const QString &deviceId, const QString &deviceName, const QString &avatar = QString(),
                                   const QString &uid = QString(), const QString &deviceModel = QString(), int deviceType = 0,
                                   const QString &did = QString());
    Q_INVOKABLE void startDirectIntercom(const QString &deviceId, const QString &deviceName, const QString &avatar = QString(),
                                         const QString &uid = QString(), const QString &deviceModel = QString(), int deviceType = 0,
                                         const QString &did = QString());
    Q_INVOKABLE void hangupVoipCall();
    Q_INVOKABLE void setVoipMute(bool mute);
    Q_INVOKABLE void setVoipSpeakerphone(bool enable);
    Q_INVOKABLE void refreshVoipContacts();
    Q_INVOKABLE void setVoipBackendType(const QString &backendType);

public slots:
    void startDeferredServices();
    void onConfigReloaded(const AppConfig &config);
    void movePtz(const QString &direction);
    void selectCamera(int index);
    void selectRelativeCamera(int offset);
    void selectOnvifProfile(const QString &profile);
    void setBrightness(qreal brightness);
    void scanWifi();
    void connectWifi(const QString &ssid, const QString &password);
    void forgetWifi(const QString &ssid);
    void triggerHaAction(const QString &actionName);
    void callHaActionService(const QString &actionName, const QString &service);
    void callHaCustomService(const QString &domain, const QString &service, const QVariantMap &data = QVariantMap());
    void startCooker(const QString &mode, const QString &name = QString());
    void cancelCooker();
    void setCookerTaste(const QString &taste);
    void setCookerPressureTime(double minutes);
    void setCookerMode(const QString &mode);
    void cookerOpenLidSaute();
    void cookerKeepWarm();
    void setWasherPower(bool on);
    void setWasherStartPause(bool start);
    void setWasherProgram(const QString &program);
    void setWasherTemperature(const QString &temp);
    void setWasherSpinSpeed(const QString &speed);
    void setWasherRinseCount(const QString &count);
    void setWasherWaterLevel(const QString &level);
    void setWasherDetergent(const QString &detergent);
    void setWasherChildLock(bool locked);
    void setWasherWindDispel(bool on);
    void setWasherNightly(bool on);
    void setHaLightBrightness(const QString &actionName, qreal brightness);
    void setHaCoverPosition(const QString &actionName, qreal position);
    void setHaMediaVolume(const QString &actionName, qreal volume);
    void refreshCameraPreviews();
    void refreshWeather();
    void blankScreen();
    void wakeScreen();
    void wakeXiaozhi();
    void hideXiaozhiOverlay();
    void startSteamer(int minutes, const QString &dishName = QString());
    void setSteamerMode(const QString &mode, int minutes = 0);
    void stopSteamer();
    void toggleSteamerSocket();

public:
    Q_INVOKABLE QString holidayBadgeForDate(const QString &isoDate);
    Q_INVOKABLE void prepareHolidayYear(int year);
    Q_INVOKABLE bool loadConfigFile(const QString &fileUrlOrPath);
    Q_INVOKABLE void resetConfigFile();
    Q_INVOKABLE void refreshCandidateConfigFiles();
    Q_INVOKABLE void setStatusBarVisible(bool visible);
    Q_INVOKABLE qreal safeAreaTop() const;
    Q_INVOKABLE qreal safeAreaBottom() const;
    Q_INVOKABLE qreal safeAreaLeft() const;
    Q_INVOKABLE qreal safeAreaRight() const;

    QString configFilePath() const;
    bool configLoaded() const;
    QString configStatusText() const;
    QStringList candidateConfigFiles() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void videoSourceChanged();
    void cameraListChanged();
    void haActionNamesChanged();
    void haActionModelsChanged();
    void currentCameraIndexChanged();
    void currentTimeChanged();
    void weatherSummaryChanged();
    void brightnessChanged();
    void wifiStateChanged();
    void networkOnlineChanged();
    void screenBlankedChanged();
    void screenOffChanged(bool isOff);
    void screenPanelTypeChanged(const QString &type);
    void screenPanelConfigChanged(const QString &config);
    void screenIdleSecondsChanged(int seconds);
    void humanPresenceChanged(bool detected);
    void steamerStateChanged();
    void steamerSocketStateChanged();
    void videoBottomCardModeChanged();
    void configFilePathChanged();
    void configLoadedChanged();
    void configStatusTextChanged();
    void candidateConfigFilesChanged();
    void safeAreaChanged();
    void intercomStateChanged();
    void intercomConfigChanged();
    void intercomRoomsChanged();
    void intercomRecordingChanged();
    void intercomAudioLevelChanged();
    void intercomQrLoginChanged();
    void intercomRecordingReady(qint64 durationMs);
    void intercomLoginResult(bool success, const QString &message);
    void intercomSendResult(bool success, const QString &message);
    void voipAvailableChanged();
    void voipStateChanged();
    void voipTargetChanged();
    void voipDurationChanged();
    void voipAudioSettingsChanged();
    void voipSpeakersChanged();
    void voipBackendTypeChanged();

private:
    // 根据当前配置和全局状态，得到界面应该使用的“当前视频配置”。
    VideoConfig selectedVideoConfig(const AppConfig &config) const;
    // 为摄像头选择弹窗准备缩略预览数据。
    void refreshVideoUi(const AppConfig &config);
    void resolveCameraPreviews(const AppConfig &config);
    void updateHaActionNames(const AppConfig &config);
    void updateVideoConfig(const AppConfig &config);
    void updateTimeText();
    void initializeNetworkMonitoring();
    bool detectNetworkOnline() const;
    void handleNetworkOnlineStateChanged(bool online);
    void applyNetworkOfflineState();
    void recoverNetworkServices();
    void initializeScreenPowerSchedule();
    void evaluateScreenPowerSchedule();
    bool isInScreenBlankWindow(const QTime &now) const;
    bool setFramebufferBlank(bool blanked);
    QString steamerSocketEntityId() const;
    QString washerEntityId(const QString &domain, const QString &propertySuffix) const;

    ConfigManager *m_configManager;
    GlobalState *m_globalState;
    ModuleManager *m_moduleManager;
    std::unique_ptr<BrightnessController> m_brightnessController;
    std::unique_ptr<ScreenPowerManager> m_screenPowerManager;
    WeatherService *m_weatherService;
    HolidayService *m_holidayService;
    QTimer m_clockTimer;
    QTimer m_networkMonitorTimer;
    QTimer m_networkRecoveryTimer;
    QTimer m_cameraSwitchGuardTimer;
    QTimer m_screenScheduleTimer;
    QTimer m_screenTemporaryWakeTimer;
    QUrl m_videoSource;
    bool m_videoEnabled = false;
    QVariantList m_cameraPreviewModels;
    QStringList m_haActionNames;
    QString m_videoBackend;
    QString m_videoDecoder;
    QString m_currentDateText;
    QString m_currentClockText;
    QString m_currentDateBadge;
    bool m_networkOnline = true;
    bool m_screenBlanked = false;
    bool m_wakeGestureActive = false;
    qint64 m_wakeCooldownUntil = 0;
    QTimer m_steamerTimer;
    bool m_steamerRunning = false;
    int m_steamerRemainSeconds = 0;
    QString m_steamerDishName;
    int m_steamerTotalMinutes = 15;
    bool m_steamerSocketState = false;
    QString m_steamerMode = QStringLiteral("待机");
    QStringList m_steamerModeOptions;
    QString m_steamerSocketEntityId;
    QString m_steamerTimerEntityId;
    QString m_steamerModeEntityId;
    QString m_steamerStopScript;
    QString m_steamerStartScript;
    QVariantList m_steamerPresetModels;
    QString m_videoBottomCardMode = QStringLiteral("camera");
};

#endif
