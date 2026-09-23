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
    Q_PROPERTY(QVariantList haActionModels READ haActionModels NOTIFY haActionNamesChanged)
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
    Q_PROPERTY(QString videoBottomCardMode READ videoBottomCardMode WRITE setVideoBottomCardMode NOTIFY videoBottomCardModeChanged)
    Q_PROPERTY(bool isAndroid READ isAndroid CONSTANT)

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
    QString videoBottomCardMode() const { return m_videoBottomCardMode; }
    Q_INVOKABLE void setVideoBottomCardMode(const QString &mode);
    bool isAndroid() const {
#if defined(Q_OS_ANDROID)
        return true;
#else
        return false;
#endif
    }
    ScreenPowerManager *screenPower() const;

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
    void stopSteamer();
    void toggleSteamerSocket();

public:
    Q_INVOKABLE QString holidayBadgeForDate(const QString &isoDate);
    Q_INVOKABLE void prepareHolidayYear(int year);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void videoSourceChanged();
    void cameraListChanged();
    void haActionNamesChanged();
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
    QString m_videoBottomCardMode = QStringLiteral("camera");
};

#endif
