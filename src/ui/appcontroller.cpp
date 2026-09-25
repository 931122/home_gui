#include "appcontroller.h"

#include "core/configmanager.h"
#include "core/globalstate.h"
#include "core/modulemanager.h"
#include "core/networkutils.h"
#include "core/platformhelper.h"
#include "brightnesscontroller.h"
#include "screenpowermanager.h"
#include "holidayservice.h"
#include "weatherservice.h"

#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QtGlobal>

namespace {

QString currentWeekLabel(int dayOfWeek)
{
    static const QStringList labels = {
        QString(), QStringLiteral("周一"), QStringLiteral("周二"), QStringLiteral("周三"),
        QStringLiteral("周四"), QStringLiteral("周五"), QStringLiteral("周六"), QStringLiteral("周日")
    };
    if (dayOfWeek >= 1 && dayOfWeek < labels.size()) {
        return labels.at(dayOfWeek);
    }
    return QString();
}

}


static QUrl applyCredentialsToStreamUrl(const QString &streamUrl,
                                        const QString &username,
                                        const QString &password)
{
    QUrl source = QUrl::fromUserInput(streamUrl);
    if (!source.isValid() || source.isEmpty()) {
        return QUrl();
    }

    if ((source.scheme() == QStringLiteral("rtsp") || source.scheme() == QStringLiteral("rtsps"))
        && !username.isEmpty() && source.userName().isEmpty()) {
        source.setUserName(username);
        source.setPassword(password);
    }

    return source;
}

static QUrl buildVideoSourceUrl(const VideoConfig &config)
{
    // 配置里的 url 允许直接写 rtsp:// 或 rtsps://，账号密码缺失时自动补上。
    // 若为 onvif:// 等非 RTSP 地址，必须返回空 QUrl，交由 GlobalState 获取 ONVIF 探测出的真实流地址。
    QUrl source = applyCredentialsToStreamUrl(config.url, config.username, config.password);
    if (source.scheme() == QStringLiteral("rtsp") || source.scheme() == QStringLiteral("rtsps")) {
        return source;
    }
    return QUrl();
}

ScreenPowerManager *AppController::screenPower() const
{
    return m_screenPowerManager.get();
}

AppController::AppController(ConfigManager *configManager,
                             GlobalState *globalState,
                             ModuleManager *moduleManager,
                             QObject *parent)
    : QObject(parent)
    , m_configManager(configManager)
    , m_globalState(globalState)
    , m_moduleManager(moduleManager)
    , m_brightnessController(std::make_unique<BrightnessController>())
    , m_screenPowerManager(std::make_unique<ScreenPowerManager>(m_brightnessController.get(), this))
    , m_weatherService(new WeatherService(this))
    , m_holidayService(new HolidayService(this))
{
    qApp->installEventFilter(this);

    // 构造时先把界面依赖的基础状态全部准备好，避免 QML 首屏拿到空值。
    m_videoBottomCardMode = m_configManager->config().ui.videoBottomCardMode;
    m_brightnessController->initialize();
    if (m_screenPowerManager) {
        const ScreenPowerConfig &spConfig = m_configManager->config().screenPower;
        m_screenPowerManager->setEnabled(spConfig.enabled);
        m_screenPowerManager->setIdleTimeoutSeconds(spConfig.idleTimeoutSeconds);
        m_screenPowerManager->setPanelConfig(spConfig.panelConfig);
        m_screenPowerManager->initialize();
    }

    connect(m_screenPowerManager.get(), &ScreenPowerManager::screenOffChanged,
            this, [this](bool isOff) {
                setFramebufferBlank(isOff);
                if (isOff) {
                    m_clockTimer.stop();
                } else {
                    updateTimeText();
                    m_clockTimer.start();
                }
                emit screenOffChanged(isOff);
            });
    connect(m_screenPowerManager.get(), &ScreenPowerManager::panelTypeChanged,
            this, &AppController::screenPanelTypeChanged);
    connect(m_screenPowerManager.get(), &ScreenPowerManager::panelConfigChanged,
            this, &AppController::screenPanelConfigChanged);
    connect(m_screenPowerManager.get(), &ScreenPowerManager::idleTimeoutSecondsChanged,
            this, &AppController::screenIdleSecondsChanged);
    connect(m_screenPowerManager.get(), &ScreenPowerManager::humanPresenceChanged,
            this, &AppController::humanPresenceChanged);
    connect(m_configManager, &ConfigManager::configReloaded, this, [this](const AppConfig &config) {
        if (m_screenPowerManager) {
            m_screenPowerManager->setEnabled(config.screenPower.enabled);
            m_screenPowerManager->setIdleTimeoutSeconds(config.screenPower.idleTimeoutSeconds);
            m_screenPowerManager->setPanelConfig(config.screenPower.panelConfig);
        }
        emit configFilePathChanged();
        emit configLoadedChanged();
        emit configStatusTextChanged();
        refreshCandidateConfigFiles();
    });
    connect(m_configManager, &ConfigManager::configLoadFailed, this, [this](const QString &) {
        emit configLoadedChanged();
        emit configStatusTextChanged();
    });
    updateHaActionNames(m_configManager->config());
    updateTimeText();
    initializeNetworkMonitoring();
    connect(m_moduleManager, &ModuleManager::activeCameraChanged,
            this, [this]() {
                emit currentCameraIndexChanged();
                refreshVideoUi(m_configManager->config());
            });
    connect(m_weatherService, &WeatherService::stateChanged,
            this, &AppController::weatherSummaryChanged);
    connect(m_globalState, &GlobalState::wifiAvailableChanged,
            this, &AppController::wifiStateChanged);
    connect(&m_steamerTimer, &QTimer::timeout, this, [this]() {
        if (m_steamerRemainSeconds > 0) {
            --m_steamerRemainSeconds;
            emit steamerStateChanged();
        } else {
            stopSteamer();
        }
    });
    connect(m_globalState, &GlobalState::haActionStatesChanged, this, [this]() {
        emit haActionModelsChanged();
        const QVariantList states = m_globalState->haActionStates();
        for (const QVariant &itemVar : states) {
            const QVariantMap item = itemVar.toMap();
            const QString entityId = item.value(QStringLiteral("entityId")).toString();
            const bool isSteamer = item.value(QStringLiteral("isSteamer")).toBool()
                                || entityId == QStringLiteral("script.timed_cook_runner")
                                || entityId.contains(QStringLiteral("steamer"));
            if (isSteamer) {
                m_steamerSocketEntityId = item.value(QStringLiteral("steamerSocketEntityId")).toString();
                m_steamerTimerEntityId = item.value(QStringLiteral("steamerTimerEntityId")).toString();
                m_steamerModeEntityId = item.value(QStringLiteral("steamerModeEntityId")).toString();
                m_steamerStopScript = item.value(QStringLiteral("steamerStopScript")).toString();
                m_steamerStartScript = item.value(QStringLiteral("steamerStartScript")).toString();
                const QVariantList presets = item.value(QStringLiteral("steamerPresetModels")).toList();
                if (!presets.isEmpty() && m_steamerPresetModels != presets) {
                    m_steamerPresetModels = presets;
                }

                const bool socketActive = item.value(QStringLiteral("steamerSocketActive")).toBool();
                if (m_steamerSocketState != socketActive) {
                    m_steamerSocketState = socketActive;
                    emit steamerSocketStateChanged();
                }

                const bool running = item.value(QStringLiteral("steamerRunning")).toBool();
                const int remain = item.value(QStringLiteral("steamerRemainSeconds")).toInt();
                const int total = item.value(QStringLiteral("steamerTotalMinutes")).toInt();
                const QString dish = item.value(QStringLiteral("steamerDishName")).toString();
                const QString mode = item.value(QStringLiteral("steamerMode")).toString();
                const QStringList options = item.value(QStringLiteral("steamerModeOptions")).toStringList();
                if (!options.isEmpty()) {
                    m_steamerModeOptions = options;
                }
                if (!mode.isEmpty()) {
                    m_steamerMode = mode;
                }

                if (running) {
                    m_steamerRunning = true;
                    if (remain > 0) {
                        m_steamerRemainSeconds = remain;
                    }
                    if (total > 0) {
                        m_steamerTotalMinutes = total;
                    }
                    if (!dish.isEmpty()) {
                        m_steamerDishName = dish;
                    }
                    if (!m_steamerTimer.isActive()) {
                        m_steamerTimer.start(1000);
                    }
                    emit steamerStateChanged();
                } else if (m_steamerRunning || (m_steamerMode != QStringLiteral("待机") && mode == QStringLiteral("待机"))) {
                    m_steamerRunning = false;
                    m_steamerTimer.stop();
                    m_steamerRemainSeconds = 0;
                    if (!mode.isEmpty()) {
                        m_steamerMode = mode;
                    }
                    emit steamerStateChanged();
                }
                break;
            }
        }
    });
    connect(m_globalState, &GlobalState::wifiScanningChanged,
            this, &AppController::wifiStateChanged);
    connect(m_globalState, &GlobalState::wifiInterfaceChanged,
            this, &AppController::wifiStateChanged);
    connect(m_globalState, &GlobalState::wifiStatusChanged,
            this, &AppController::wifiStateChanged);
    connect(m_globalState, &GlobalState::wifiNetworksChanged,
            this, &AppController::wifiStateChanged);
    // ONVIF 解析成功后，后端会回写最终流地址，前端要跟着刷新 source。
    connect(m_globalState, &GlobalState::videoStreamUrlChanged,
            this, [this]() {
                refreshVideoUi(m_configManager->config());
            });
    connect(m_globalState, &GlobalState::onvifProfileSwitchSupportedChanged,
            this, &AppController::videoSourceChanged);
    connect(m_globalState, &GlobalState::onvifCurrentProfileChanged,
            this, &AppController::videoSourceChanged);
    connect(&m_clockTimer, &QTimer::timeout,
            this, &AppController::updateTimeText);
    m_cameraSwitchGuardTimer.setSingleShot(true);
    m_cameraSwitchGuardTimer.setInterval(1200);
    m_networkMonitorTimer.setInterval(5000);
    connect(&m_networkMonitorTimer, &QTimer::timeout, this, [this]() {
        handleNetworkOnlineStateChanged(detectNetworkOnline());
    });
    m_networkMonitorTimer.start();
    m_networkRecoveryTimer.setSingleShot(true);
    m_networkRecoveryTimer.setInterval(2500);
    connect(&m_networkRecoveryTimer, &QTimer::timeout,
            this, &AppController::recoverNetworkServices);
    initializeScreenPowerSchedule();
    m_clockTimer.start(1000);

}

AppController::~AppController()
{
    qApp->removeEventFilter(this);
}

void AppController::startDeferredServices()
{
    // 首屏已经提交后再启动网络/天气/屏幕调度相关动作，避免抢占 QML 第一帧。
    const AppConfig config = m_configManager->config();
    if (m_networkOnline) {
        refreshVideoUi(config);
        m_weatherService->applyConfig(config.weather, m_networkOnline);
    } else {
        applyNetworkOfflineState();
    }
    evaluateScreenPowerSchedule();
}

bool AppController::isScreenOff() const
{
    return m_screenPowerManager ? m_screenPowerManager->isScreenOff() : false;
}

QString AppController::screenPanelType() const
{
    return m_screenPowerManager ? m_screenPowerManager->panelType() : QStringLiteral("LCD");
}

QString AppController::screenPanelConfig() const
{
    return m_screenPowerManager ? m_screenPowerManager->panelConfig() : QStringLiteral("auto");
}

void AppController::setScreenPanelConfig(const QString &config)
{
    if (m_screenPowerManager) {
        m_screenPowerManager->setPanelConfig(config);
    }
    if (m_configManager) {
        const int idleSec = m_screenPowerManager ? m_screenPowerManager->idleTimeoutSeconds() : 180;
        m_configManager->updateScreenPowerSettings(idleSec, config);
    }
}

int AppController::screenIdleSeconds() const
{
    return m_screenPowerManager ? m_screenPowerManager->idleTimeoutSeconds() : 180;
}

void AppController::setScreenIdleSeconds(int seconds)
{
    if (m_screenPowerManager) {
        m_screenPowerManager->setIdleTimeoutSeconds(seconds);
    }
    if (m_configManager) {
        const QString panel = m_screenPowerManager ? m_screenPowerManager->panelConfig() : QStringLiteral("auto");
        m_configManager->updateScreenPowerSettings(seconds, panel);
    }
}

bool AppController::humanPresenceDetected() const
{
    return m_screenPowerManager ? m_screenPowerManager->humanPresenceDetected() : false;
}

void AppController::requestWake(const QString &source)
{
    if (m_screenPowerManager) {
        m_screenPowerManager->requestWake(source);
    }
    wakeScreen();
}

void AppController::requestSleep()
{
    if (!m_screenPowerManager || !m_screenPowerManager->enabled()) {
        return;
    }
    m_screenPowerManager->requestSleep();
    setFramebufferBlank(true);
}

void AppController::reportHumanPresence(bool detected)
{
    if (m_screenPowerManager) {
        m_screenPowerManager->reportHumanPresence(detected);
    }
}

bool AppController::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)
    const bool isOff = (m_screenBlanked || (m_screenPowerManager && m_screenPowerManager->isScreenOff()));
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::TouchBegin: {
        if (isOff) {
            // 灭屏状态下触发轻触唤醒：激活手势序列锁并开启 400ms 防误触冷却窗口
            m_wakeGestureActive = true;
            m_wakeCooldownUntil = now + 400;
            requestWake(QStringLiteral("touch"));
            return true; // 拦截并吞噬当前按下事件
        }
        if (m_wakeGestureActive || now < m_wakeCooldownUntil) {
            return true; // 唤醒手势或冷却期内的任何意外按下，一律拦截
        }
        if (m_screenPowerManager) {
            m_screenPowerManager->reportUserActivity(true); // 按下事件强制即时重置倒计时
        }
        return false;
    }
    case QEvent::MouseMove:
    case QEvent::TouchUpdate: {
        if (m_wakeGestureActive || now < m_wakeCooldownUntil) {
            return true; // 吞噬唤醒滑动
        }
        if (m_screenPowerManager && !isOff) {
            m_screenPowerManager->reportUserActivity(false); // 连续滑动自动执行 2000ms 节流
        }
        return false;
    }
    case QEvent::MouseButtonRelease:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        if (m_wakeGestureActive) {
            m_wakeGestureActive = false; // 消费完当前唤醒手势序列，解除手势锁
            return true; // 彻底拦截并吞噬抬起事件，杜绝穿透到底层 UI！
        }
        if (now < m_wakeCooldownUntil) {
            return true; // 处于冷却期内的抬起也拦截
        }
        if (isOff) {
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

QUrl AppController::videoSource() const
{
    return m_videoSource;
}

bool AppController::videoEnabled() const
{
    return m_videoEnabled;
}

QVariantList AppController::cameraPreviewModels() const
{
    return m_cameraPreviewModels;
}

QVariantList AppController::haActionModels() const
{
    return m_globalState->haActionStates();
}

QStringList AppController::haActionNames() const
{
    return m_haActionNames;
}

int AppController::currentCameraIndex() const
{
    return m_moduleManager->currentCameraIndex();
}

QString AppController::videoBackend() const
{
    return m_videoBackend;
}

QString AppController::videoDecoder() const
{
    return m_videoDecoder;
}

bool AppController::onvifProfileSwitchSupported() const
{
    return m_globalState->onvifProfileSwitchSupported();
}

QString AppController::onvifCurrentProfile() const
{
    return m_globalState->onvifCurrentProfile();
}

QString AppController::currentDateText() const
{
    return m_currentDateText;
}

QString AppController::currentClockText() const
{
    return m_currentClockText;
}

QString AppController::currentDateBadge() const
{
    return m_currentDateBadge;
}

QString AppController::weatherSummary() const
{
    return m_weatherService->summary();
}

QString AppController::weatherLocation() const
{
    return m_weatherService->location();
}

QString AppController::weatherDetail() const
{
    return m_weatherService->detail();
}

QVariantList AppController::weatherForecast() const
{
    return m_weatherService->forecast();
}

QVariantMap AppController::weatherCurrent() const
{
    return m_weatherService->current();
}

qreal AppController::brightness() const
{
    return m_brightnessController->brightness();
}

bool AppController::hardwareBrightnessAvailable() const
{
    return m_brightnessController->hardwareAvailable();
}

bool AppController::wifiAvailable() const
{
    return m_globalState->wifiAvailable();
}

bool AppController::wifiScanning() const
{
    return m_globalState->wifiScanning();
}

QString AppController::wifiInterface() const
{
    return m_globalState->wifiInterface();
}

QString AppController::wifiStatus() const
{
    return m_globalState->wifiStatus();
}

QVariantList AppController::wifiNetworks() const
{
    return m_globalState->wifiNetworks();
}

bool AppController::networkOnline() const
{
    return m_networkOnline;
}

bool AppController::screenBlanked() const
{
    return m_screenBlanked;
}

void AppController::onConfigReloaded(const AppConfig &config)
{
    // 配置热加载后，把界面展示相关状态全部重新整理一遍。
    if (m_videoBottomCardMode != config.ui.videoBottomCardMode) {
        m_videoBottomCardMode = config.ui.videoBottomCardMode;
        emit videoBottomCardModeChanged();
    }
    refreshVideoUi(config);
    updateHaActionNames(config);
    m_weatherService->applyConfig(config.weather, m_networkOnline);
    evaluateScreenPowerSchedule();
}

void AppController::setVideoBottomCardMode(const QString &mode)
{
    QString normalized = mode.trimmed().toLower();
    if (normalized == QStringLiteral("a") || normalized == QStringLiteral("camera_switcher")) {
        normalized = QStringLiteral("camera");
    } else if (normalized == QStringLiteral("b") || normalized == QStringLiteral("scenes")) {
        normalized = QStringLiteral("scene");
    } else if (normalized == QStringLiteral("c") || normalized == QStringLiteral("status")) {
        normalized = QStringLiteral("security");
    }
    if (normalized != QStringLiteral("camera") && normalized != QStringLiteral("scene") &&
        normalized != QStringLiteral("security")) {
        normalized = QStringLiteral("camera");
    }
    if (m_videoBottomCardMode != normalized) {
        m_videoBottomCardMode = normalized;
        emit videoBottomCardModeChanged();
    }
}

void AppController::movePtz(const QString &direction)
{
    m_moduleManager->moveVideoPtz(direction);
}

void AppController::selectCamera(int index)
{
    const AppConfig &config = m_configManager->config();
    const int currentIndex = currentCameraIndex();
    if (config.cameras.isEmpty()) {
        return;
    }
    if (m_cameraSwitchGuardTimer.isActive()) {
        return;
    }
    if (index < 0 || index >= config.cameras.size() || index == currentIndex) {
        return;
    }

    // 真正的视频切换由后端模块执行，这里只负责更新前端索引和触发切换。
    m_cameraSwitchGuardTimer.start();
    m_moduleManager->selectCamera(config.cameras.at(index));
}

void AppController::selectRelativeCamera(int offset)
{
    const AppConfig &config = m_configManager->config();
    if (config.cameras.size() <= 1 || offset == 0) {
        return;
    }

    const int count = config.cameras.size();
    const int currentIndex = qBound(0, currentCameraIndex(), count - 1);
    const int nextIndex = (currentIndex + offset % count + count) % count;
    if (nextIndex == currentIndex) {
        return;
    }

    selectCamera(nextIndex);
}

void AppController::selectOnvifProfile(const QString &profile)
{
    const AppConfig &config = m_configManager->config();
    const int currentIndex = currentCameraIndex();
    if (currentIndex < 0 || currentIndex >= config.cameras.size()) {
        return;
    }

    const QString normalized = profile.trimmed().toLower();
    if (normalized != QStringLiteral("main") && normalized != QStringLiteral("minor")) {
        return;
    }

    const VideoConfig activeVideoConfig = selectedVideoConfig(config);
    if (activeVideoConfig.onvifProfile.trimmed().toLower() == normalized) {
        return;
    }

    m_configManager->updateCameraOnvifProfile(currentIndex, normalized);
}

void AppController::setBrightness(qreal brightness)
{
    const qreal nextBrightness = qBound<qreal>(0.01, brightness, 1.0);
    if (m_screenPowerManager) {
        m_screenPowerManager->setNormalBrightness(nextBrightness);
    }
    emit brightnessChanged();
}

void AppController::scanWifi()
{
    m_moduleManager->scanWifiNetworks();
}

void AppController::connectWifi(const QString &ssid, const QString &password)
{
    m_moduleManager->connectWifiNetwork(ssid, password);
}

void AppController::forgetWifi(const QString &ssid)
{
    m_moduleManager->forgetWifiNetwork(ssid);
}

void AppController::wakeXiaozhi()
{
    wakeScreen();
    m_moduleManager->wakeXiaozhi();
}

void AppController::hideXiaozhiOverlay()
{
    m_moduleManager->hideXiaozhiOverlay();
}

void AppController::triggerHaAction(const QString &actionName)
{
    m_moduleManager->triggerHomeAssistantAction(actionName);
}

void AppController::callHaActionService(const QString &actionName, const QString &service)
{
    m_moduleManager->callHomeAssistantActionService(actionName, service);
}

void AppController::callHaCustomService(const QString &domain, const QString &service, const QVariantMap &data)
{
    m_moduleManager->callHomeAssistantCustomService(domain, service, data);
}

void AppController::startCooker(const QString &mode, const QString &name)
{
    const QString slug = PlatformHelper::cookerModeToSlug(mode);
    if (!slug.isEmpty()) {
        setCookerMode(slug);
    }

    QVariantMap data;
    data.insert(QStringLiteral("mode"), slug);
    if (!name.trimmed().isEmpty()) {
        data.insert(QStringLiteral("name"), name);
    }
    m_moduleManager->callHomeAssistantCustomService(QStringLiteral("chunmi_pre_cooker"),
                                                   QStringLiteral("start_cooking"),
                                                   data);

    QVariantMap btnData;
    btnData.insert(QStringLiteral("entity_id"), QStringLiteral("button.chun_mi_dian_ya_li_guo_ya_li_guo_kai_shi_peng_ren"));
    callHaCustomService(QStringLiteral("button"), QStringLiteral("press"), btnData);
}

void AppController::cancelCooker()
{
    QVariantMap btnData;
    btnData.insert(QStringLiteral("entity_id"), QStringLiteral("button.chun_mi_dian_ya_li_guo_ya_li_guo_ting_zhi_peng_ren"));
    callHaCustomService(QStringLiteral("button"), QStringLiteral("press"), btnData);

    m_moduleManager->callHomeAssistantCustomService(QStringLiteral("chunmi_pre_cooker"),
                                                   QStringLiteral("cancel_cooking"),
                                                   QVariantMap());
}

void AppController::setCookerTaste(const QString &taste)
{
    const QString slug = PlatformHelper::cookerTasteToSlug(taste);
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), QStringLiteral("select.chu_fang_chun_mi_dian_ya_li_guo_ya_li_guo_kou_gan_pian_hao"));
    data.insert(QStringLiteral("option"), slug);
    callHaCustomService(QStringLiteral("select"), QStringLiteral("select_option"), data);
}

void AppController::setCookerPressureTime(double minutes)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), QStringLiteral("number.chu_fang_chun_mi_dian_ya_li_guo_ya_li_guo_bao_ya_shi_jian"));
    data.insert(QStringLiteral("value"), minutes);
    callHaCustomService(QStringLiteral("number"), QStringLiteral("set_value"), data);
}

void AppController::setCookerMode(const QString &mode)
{
    const QString slug = PlatformHelper::cookerModeToSlug(mode);
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), QStringLiteral("select.chun_mi_dian_ya_li_guo_ya_li_guo_peng_ren_mo_shi_xuan_ze"));
    data.insert(QStringLiteral("option"), slug);
    callHaCustomService(QStringLiteral("select"), QStringLiteral("select_option"), data);
}

void AppController::cookerOpenLidSaute()
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), QStringLiteral("button.chu_fang_chun_mi_dian_ya_li_guo_ya_li_guo_kai_gai_shou_zhi"));
    callHaCustomService(QStringLiteral("button"), QStringLiteral("press"), data);
}

void AppController::cookerKeepWarm()
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), QStringLiteral("button.chu_fang_chun_mi_dian_ya_li_guo_ya_li_guo_kai_shi_bao_wen"));
    callHaCustomService(QStringLiteral("button"), QStringLiteral("press"), data);
}

void AppController::setWasherPower(bool on)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("switch"), QStringLiteral("power")));
    callHaCustomService(QStringLiteral("switch"), on ? QStringLiteral("turn_on") : QStringLiteral("turn_off"), data);
}

void AppController::setWasherStartPause(bool start)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("switch"), QStringLiteral("control_status")));
    callHaCustomService(QStringLiteral("switch"), start ? QStringLiteral("turn_on") : QStringLiteral("turn_off"), data);
}

void AppController::setWasherProgram(const QString &program)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("select"), QStringLiteral("program")));
    data.insert(QStringLiteral("option"), program);
    callHaCustomService(QStringLiteral("select"), QStringLiteral("select_option"), data);
}

void AppController::setWasherTemperature(const QString &temp)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("select"), QStringLiteral("temperature")));
    data.insert(QStringLiteral("option"), temp);
    callHaCustomService(QStringLiteral("select"), QStringLiteral("select_option"), data);
}

void AppController::setWasherSpinSpeed(const QString &speed)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("select"), QStringLiteral("dehydration_speed")));
    data.insert(QStringLiteral("option"), speed);
    callHaCustomService(QStringLiteral("select"), QStringLiteral("select_option"), data);
}

void AppController::setWasherRinseCount(const QString &count)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("select"), QStringLiteral("soak_count")));
    data.insert(QStringLiteral("option"), count);
    callHaCustomService(QStringLiteral("select"), QStringLiteral("select_option"), data);
}

void AppController::setWasherWaterLevel(const QString &level)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("select"), QStringLiteral("water_level")));
    data.insert(QStringLiteral("option"), level);
    callHaCustomService(QStringLiteral("select"), QStringLiteral("select_option"), data);
}

void AppController::setWasherDetergent(const QString &detergent)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("select"), QStringLiteral("detergent")));
    data.insert(QStringLiteral("option"), detergent);
    callHaCustomService(QStringLiteral("select"), QStringLiteral("select_option"), data);
}

void AppController::setWasherChildLock(bool locked)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("lock"), QStringLiteral("lock")));
    callHaCustomService(QStringLiteral("lock"), locked ? QStringLiteral("lock") : QStringLiteral("unlock"), data);
}

void AppController::setWasherWindDispel(bool on)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("switch"), QStringLiteral("wind_dispel")));
    callHaCustomService(QStringLiteral("switch"), on ? QStringLiteral("turn_on") : QStringLiteral("turn_off"), data);
}

void AppController::setWasherNightly(bool on)
{
    QVariantMap data;
    data.insert(QStringLiteral("entity_id"), washerEntityId(QStringLiteral("switch"), QStringLiteral("nightly")));
    callHaCustomService(QStringLiteral("switch"), on ? QStringLiteral("turn_on") : QStringLiteral("turn_off"), data);
}

void AppController::setHaLightBrightness(const QString &actionName, qreal brightness)
{
    m_moduleManager->setHomeAssistantLightBrightness(actionName, qBound<qreal>(0.01, brightness, 1.0));
}

void AppController::setHaCoverPosition(const QString &actionName, qreal position)
{
    m_moduleManager->setHomeAssistantCoverPosition(actionName, qBound<qreal>(0.0, position, 1.0));
}

void AppController::setHaMediaVolume(const QString &actionName, qreal volume)
{
    m_moduleManager->setHomeAssistantMediaVolume(actionName, qBound<qreal>(0.0, volume, 1.0));
}

void AppController::refreshCameraPreviews()
{
    resolveCameraPreviews(m_configManager->config());
}

void AppController::refreshWeather()
{
    if (m_weatherService) {
        m_weatherService->refresh();
    }
}

void AppController::blankScreen()
{
    m_screenTemporaryWakeTimer.stop();
    if (m_screenPowerManager) {
        m_screenPowerManager->setScheduledSleep(true);
    }
    setFramebufferBlank(true);
}

void AppController::wakeScreen()
{
    if (m_screenPowerManager) {
        m_screenPowerManager->setScheduledSleep(false);
    }
    if (!setFramebufferBlank(false)) {
        return;
    }

    const ScreenPowerConfig screenPower = m_configManager->config().screenPower;
    if (screenPower.enabled && isInScreenBlankWindow(QTime::currentTime())) {
        m_screenTemporaryWakeTimer.start(qMax(1, screenPower.temporaryWakeMinutes) * 60 * 1000);
    } else {
        m_screenTemporaryWakeTimer.stop();
    }
}

QString AppController::holidayBadgeForDate(const QString &isoDate)
{
    const QDate date = QDate::fromString(isoDate, QStringLiteral("yyyy-MM-dd"));
    if (!date.isValid()) {
        return QString();
    }

    return m_holidayService->badgeForDate(date);
}

void AppController::prepareHolidayYear(int year)
{
    if (year < 2000 || year > 2100) {
        return;
    }

    m_holidayService->ensureDateLoaded(QDate(year, 1, 1));
}

void AppController::refreshVideoUi(const AppConfig &config)
{
    updateVideoConfig(config);
    resolveCameraPreviews(config);
}

VideoConfig AppController::selectedVideoConfig(const AppConfig &config) const
{
    if (!config.cameras.isEmpty()) {
        const int index = qBound(0, currentCameraIndex(), config.cameras.size() - 1);
        return config.cameras.at(index);
    }
    return config.video;
}

void AppController::updateVideoConfig(const AppConfig &config)
{
    // 视频地址优先级：
    // 1. config 里显式 rtsp/rtsps url
    // 2. 后端 ONVIF 解析后回写到 GlobalState 的 stream url
    // AppController 只做“前端展示态拼装”，不参与 ONVIF/Wi-Fi/HA 的具体协议处理。
    const VideoConfig activeVideoConfig = selectedVideoConfig(config);

    QUrl nextSource = buildVideoSourceUrl(activeVideoConfig);
    if (nextSource.isEmpty()) {
        nextSource = applyCredentialsToStreamUrl(m_globalState->videoStreamUrl(),
                                                 activeVideoConfig.username,
                                                 activeVideoConfig.password);
    }
    const bool nextEnabled = activeVideoConfig.enabled && nextSource.isValid() && !nextSource.isEmpty();
    const QString nextBackend = activeVideoConfig.backend.trimmed().isEmpty()
            ? QStringLiteral("auto")
            : activeVideoConfig.backend.trimmed().toLower();
    const QString nextDecoder = activeVideoConfig.decoder.trimmed().isEmpty()
            ? QStringLiteral("auto")
            : activeVideoConfig.decoder.trimmed().toLower();

    bool changed = false;
    if (m_videoSource != nextSource) {
        m_videoSource = nextSource;
        changed = true;
    }
    if (m_videoEnabled != nextEnabled) {
        m_videoEnabled = nextEnabled;
        changed = true;
    }
    if (m_videoBackend != nextBackend) {
        m_videoBackend = nextBackend;
        changed = true;
    }
    if (m_videoDecoder != nextDecoder) {
        m_videoDecoder = nextDecoder;
        changed = true;
    }
    if (changed) {
        emit videoSourceChanged();
    }
}

void AppController::updateHaActionNames(const AppConfig &config)
{
    QStringList nextActionNames;
    for (const HomeAssistantActionConfig &action : config.homeAssistant.actions) {
        if (!action.name.isEmpty()) {
            nextActionNames.append(action.name);
        }
    }

    if (m_haActionNames == nextActionNames) {
        return;
    }

    m_haActionNames = nextActionNames;
    emit haActionNamesChanged();
    emit haActionModelsChanged();
}

void AppController::resolveCameraPreviews(const AppConfig &config)
{
    // 弹窗里的每个摄像头卡片独立预览。ONVIF 只复用当前已解析的流地址，避免后台批量探测。
    QVariantList nextModels;
    const QList<VideoConfig> cameras = config.cameras.isEmpty()
            ? QList<VideoConfig>{config.video}
            : config.cameras;
    const int activeIndex = currentCameraIndex();

    for (int index = 0; index < cameras.size(); ++index) {
        const VideoConfig &camera = cameras.at(index);
        QVariantMap model;
        model.insert(QStringLiteral("name"), camera.cameraName);
        QUrl source = buildVideoSourceUrl(camera);
        if (source.isEmpty() && index == activeIndex) {
            source = m_videoSource;
        }
        const bool useSharedFrame = index == activeIndex && !source.isEmpty();
        model.insert(QStringLiteral("source"), source);
        model.insert(QStringLiteral("shared"), useSharedFrame);
        model.insert(QStringLiteral("sharedKey"), useSharedFrame ? source.toString() : QString());
        model.insert(QStringLiteral("backend"),
                     camera.backend.trimmed().isEmpty()
                     ? QStringLiteral("auto")
                     : camera.backend.trimmed().toLower());
        model.insert(QStringLiteral("decoder"),
                     camera.decoder.trimmed().isEmpty()
                     ? QStringLiteral("auto")
                     : camera.decoder.trimmed().toLower());
        nextModels.append(model);
    }

    if (m_cameraPreviewModels != nextModels) {
        m_cameraPreviewModels = nextModels;
        emit cameraListChanged();
    }
}

void AppController::updateTimeText()
{
    // 顶部时间区域拆成日期和时钟两段，避免换行测量导致小屏闪烁。
    const QDateTime now = QDateTime::currentDateTime();
    const QString nextDateText = QStringLiteral("%1 %2")
            .arg(now.toString(QStringLiteral("MM-dd")))
            .arg(currentWeekLabel(now.date().dayOfWeek()));
    const QString nextClockText = now.toString(QStringLiteral("HH:mm:ss"));
    m_holidayService->setCurrentDate(now.date());
    if (m_currentDateText == nextDateText
        && m_currentClockText == nextClockText) {
        return;
    }
    m_currentDateText = nextDateText;
    m_currentClockText = nextClockText;
    m_currentDateBadge.clear();
    emit currentTimeChanged();
}

void AppController::initializeNetworkMonitoring()
{
    m_networkOnline = detectNetworkOnline();
    emit networkOnlineChanged();
}

bool AppController::detectNetworkOnline() const
{
    return ::detectNetworkOnline();
}

void AppController::handleNetworkOnlineStateChanged(bool online)
{
    if (m_networkOnline == online) {
        return;
    }

    m_networkOnline = online;
    emit networkOnlineChanged();
    if (!m_networkOnline) {
        m_networkRecoveryTimer.stop();
        applyNetworkOfflineState();
        return;
    }

    m_weatherService->setNetworkOnline(true);
    m_holidayService->setNetworkOnline(true);
    m_globalState->setHaStatus(tr("Network reconnecting..."));
    m_networkRecoveryTimer.start();
}

void AppController::applyNetworkOfflineState()
{
    m_globalState->setOnvifAvailable(false);
    m_globalState->setPtzStatus(tr("PTZ unavailable"));
    m_globalState->setPtzAvailable(false);
    m_globalState->setHaStatus(tr("Network disconnected"));
    m_weatherService->setNetworkOnline(false);
    m_holidayService->setNetworkOnline(false);
}

void AppController::recoverNetworkServices()
{
    if (!m_networkOnline) {
        return;
    }

    AppConfig config = m_configManager->config();
    config.video = selectedVideoConfig(config);
    m_moduleManager->restartModules(config);
    updateVideoConfig(config);
    resolveCameraPreviews(config);
    m_weatherService->applyConfig(config.weather, m_networkOnline);
}

void AppController::initializeScreenPowerSchedule()
{
    m_screenScheduleTimer.setInterval(30000);
    connect(&m_screenScheduleTimer, &QTimer::timeout,
            this, &AppController::evaluateScreenPowerSchedule);
    m_screenScheduleTimer.start();

    m_screenTemporaryWakeTimer.setSingleShot(true);
    connect(&m_screenTemporaryWakeTimer, &QTimer::timeout, this, [this]() {
        if (isInScreenBlankWindow(QTime::currentTime())) {
            blankScreen();
        }
    });
}

void AppController::evaluateScreenPowerSchedule()
{
    const ScreenPowerConfig screenPower = m_configManager->config().screenPower;
    if (!screenPower.enabled) {
        m_screenTemporaryWakeTimer.stop();
        if (m_screenBlanked) {
            wakeScreen();
        }
        return;
    }

    const bool shouldBlank = isInScreenBlankWindow(QTime::currentTime());
    if (shouldBlank) {
        if (!m_screenBlanked && !m_screenTemporaryWakeTimer.isActive()) {
            blankScreen();
        }
        return;
    }

    m_screenTemporaryWakeTimer.stop();
    if (m_screenBlanked) {
        wakeScreen();
    }
}

bool AppController::isInScreenBlankWindow(const QTime &now) const
{
    const ScreenPowerConfig screenPower = m_configManager->config().screenPower;
    QTime blankStart = QTime::fromString(screenPower.blankStart, QStringLiteral("hh:mm"));
    QTime blankEnd = QTime::fromString(screenPower.blankEnd, QStringLiteral("hh:mm"));
    if (!blankStart.isValid()) {
        blankStart = QTime::fromString(screenPower.blankStart, QStringLiteral("h:mm"));
    }
    if (!blankEnd.isValid()) {
        blankEnd = QTime::fromString(screenPower.blankEnd, QStringLiteral("h:mm"));
    }
    if (!screenPower.enabled || !now.isValid() || !blankStart.isValid() || !blankEnd.isValid()
        || blankStart == blankEnd) {
        return false;
    }

    if (blankStart < blankEnd) {
        return now >= blankStart && now < blankEnd;
    }
    return now >= blankStart || now < blankEnd;
}

bool AppController::setFramebufferBlank(bool blanked)
{
    if (m_screenBlanked == blanked) {
        return true;
    }

    m_screenBlanked = blanked;
    emit screenBlankedChanged();

    // 无论在任何平台，确保 ScreenPowerManager 同步灭屏/亮屏
    if (m_screenPowerManager) {
        if (blanked) {
            m_screenPowerManager->requestSleep();
        } else {
            m_screenPowerManager->requestWake(QStringLiteral("schedule"));
        }
    }

    if (PlatformHelper::isDesktopEnvironment()) {
        return true;
    }

#if defined(Q_OS_ANDROID)
    return true;
#endif

    QFile blankFile(QStringLiteral("/sys/class/graphics/fb0/blank"));
    if (!blankFile.exists()) {
        return true;
    }

    if (!blankFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open framebuffer blank control:" << blankFile.errorString();
        return false;
    }

    const QByteArray data = blanked ? "1\n" : "0\n";
    if (blankFile.write(data) != data.size()) {
        qWarning() << "Failed to write all data to blank control";
        return false;
    }
    blankFile.flush();

    qInfo() << "Screen blank state changed to:" << (blanked ? "OFF" : "ON");
    return true;
}

void AppController::startSteamer(int minutes, const QString &dishName)
{
    const QString resolvedDish = dishName.isEmpty() ? tr("定时蒸煮") : dishName;
    int resolvedMinutes = minutes;
    if (resolvedMinutes <= 0) {
        // 先在从 HA 获取的动态菜谱中寻找匹配时长
        for (const QVariant &pv : m_steamerPresetModels) {
            const QVariantMap pm = pv.toMap();
            if (pm.value(QStringLiteral("name")).toString() == resolvedDish) {
                resolvedMinutes = pm.value(QStringLiteral("time")).toInt();
                break;
            }
        }
        if (resolvedMinutes <= 0) {
            resolvedMinutes = 15;
        }
    }

    m_steamerRunning = true;
    m_steamerTotalMinutes = resolvedMinutes;
    m_steamerRemainSeconds = resolvedMinutes * 60;
    m_steamerDishName = resolvedDish;
    m_steamerMode = resolvedDish;
    m_steamerTimer.start(1000);
    emit steamerStateChanged();

    // 1. 若存在启动执行器脚本 (如 script.timed_cook_runner 或配置的自定义脚本)
    if (!m_steamerStartScript.isEmpty()) {
        QVariantMap data;
        if (minutes > 0) {
            data.insert(QStringLiteral("minutes"), minutes);
        }
        data.insert(QStringLiteral("dish_name"), m_steamerDishName);
        data.insert(QStringLiteral("mode"), m_steamerDishName);
        const QString socketId = steamerSocketEntityId();
        if (!socketId.isEmpty()) {
            data.insert(QStringLiteral("target_socket"), socketId);
        }
        const QString domain = m_steamerStartScript.contains(QLatin1Char('.')) ? m_steamerStartScript.section(QLatin1Char('.'), 0, 0) : QStringLiteral("script");
        const QString service = m_steamerStartScript.contains(QLatin1Char('.')) ? m_steamerStartScript.section(QLatin1Char('.'), 1) : m_steamerStartScript;
        callHaCustomService(domain, service, data);
        return;
    }

    // 2. 若无执行器脚本，自适应驱动标准 HA 实体:
    // a. 设定模式选择器 (input_select)
    if (!m_steamerModeEntityId.isEmpty()) {
        QVariantMap modeData;
        modeData.insert(QStringLiteral("entity_id"), m_steamerModeEntityId);
        modeData.insert(QStringLiteral("option"), m_steamerDishName);
        callHaCustomService(QStringLiteral("input_select"), QStringLiteral("select_option"), modeData);
    }

    // b. 启动倒计时器 (timer)
    if (!m_steamerTimerEntityId.isEmpty()) {
        QVariantMap timerData;
        timerData.insert(QStringLiteral("entity_id"), m_steamerTimerEntityId);
        timerData.insert(QStringLiteral("duration"), QString::number(resolvedMinutes * 60));
        callHaCustomService(QStringLiteral("timer"), QStringLiteral("start"), timerData);
    }

    // c. 开启电源插座/开关 (switch)
    const QString socketId = steamerSocketEntityId();
    if (!socketId.isEmpty()) {
        QVariantMap socketData;
        socketData.insert(QStringLiteral("entity_id"), socketId);
        callHaCustomService(QStringLiteral("switch"), QStringLiteral("turn_on"), socketData);
    }
}

void AppController::setSteamerMode(const QString &mode, int minutes)
{
    if (mode.isEmpty() || mode == QStringLiteral("待机")) {
        stopSteamer();
        return;
    }
    startSteamer(minutes, mode);
}

void AppController::stopSteamer()
{
    m_steamerRunning = false;
    m_steamerTimer.stop();
    m_steamerRemainSeconds = 0;
    m_steamerMode = QStringLiteral("待机");
    emit steamerStateChanged();

    // 1. 若存在停机脚本 (如 script.steamer_stop 或配置的停机服务)，直接调用
    if (!m_steamerStopScript.isEmpty()) {
        const QString domain = m_steamerStopScript.contains(QLatin1Char('.')) ? m_steamerStopScript.section(QLatin1Char('.'), 0, 0) : QStringLiteral("script");
        const QString service = m_steamerStopScript.contains(QLatin1Char('.')) ? m_steamerStopScript.section(QLatin1Char('.'), 1) : m_steamerStopScript;
        callHaCustomService(domain, service, QVariantMap());
        return;
    }

    // 2. 否则自适应停止通用 HA 组件:
    // a. 取消倒计时器
    if (!m_steamerTimerEntityId.isEmpty()) {
        QVariantMap timerData;
        timerData.insert(QStringLiteral("entity_id"), m_steamerTimerEntityId);
        callHaCustomService(QStringLiteral("timer"), QStringLiteral("cancel"), timerData);
    }

    // b. 关断电源插座
    const QString socketId = steamerSocketEntityId();
    if (!socketId.isEmpty()) {
        QVariantMap socketData;
        socketData.insert(QStringLiteral("entity_id"), socketId);
        callHaCustomService(QStringLiteral("switch"), QStringLiteral("turn_off"), socketData);
    }

    // c. 模式选择器复位为待机
    if (!m_steamerModeEntityId.isEmpty()) {
        QVariantMap modeData;
        modeData.insert(QStringLiteral("entity_id"), m_steamerModeEntityId);
        modeData.insert(QStringLiteral("option"), QStringLiteral("待机"));
        callHaCustomService(QStringLiteral("input_select"), QStringLiteral("select_option"), modeData);
    }
}

void AppController::toggleSteamerSocket()
{
    const QString socketId = steamerSocketEntityId();
    if (!socketId.isEmpty()) {
        QVariantMap socketData;
        socketData.insert(QStringLiteral("entity_id"), socketId);
        callHaCustomService(QStringLiteral("switch"), QStringLiteral("toggle"), socketData);
    }
}

QString AppController::steamerSocketEntityId() const
{
    if (!m_steamerSocketEntityId.isEmpty()) {
        return m_steamerSocketEntityId;
    }

    if (m_configManager) {
        // 优先精准匹配餐桌/蒸煮插座，避免误匹配主卧等其他插座
        for (const auto &action : m_configManager->config().homeAssistant.actions) {
            if (!action.socketEntity.isEmpty()) {
                return action.socketEntity;
            }
            if (action.domain == QStringLiteral("switch") &&
                (action.name.contains(QStringLiteral("餐桌")) || action.name.contains(QStringLiteral("蒸")) || action.entityId.contains(QStringLiteral("dining")) || action.entityId.contains(QStringLiteral("steamer")))) {
                return action.entityId;
            }
        }
        for (const auto &action : m_configManager->config().homeAssistant.actions) {
            if (action.domain == QStringLiteral("switch") &&
                (action.name.contains(QStringLiteral("插座")) || action.entityId.contains(QStringLiteral("socket")))) {
                return action.entityId;
            }
        }
    }
    return QString();
}

QString AppController::washerEntityId(const QString &domain, const QString &propertySuffix) const
{
    QString devId;
    if (m_globalState) {
        const QVariantList states = m_globalState->haActionStates();
        for (const QVariant &v : states) {
            const QVariantMap item = v.toMap();
            if (item.value(QStringLiteral("isWasher")).toBool()) {
                devId = item.value(QStringLiteral("washerDeviceId")).toString();
                if (!devId.isEmpty()) {
                    break;
                }
            }
        }
    }

    if (devId.isEmpty() && m_configManager) {
        for (const auto &action : m_configManager->config().homeAssistant.actions) {
            if (action.domain.contains(QStringLiteral("washer")) || action.name.contains(QStringLiteral("洗衣机"))) {
                static const QRegularExpression devIdRegex(QStringLiteral(R"((?:midea_)?(\d{6,})|(?:washer_[a-zA-Z0-9]+))"));
                const auto match = devIdRegex.match(action.entityId);
                if (match.hasMatch()) {
                    devId = match.captured(1).isEmpty() ? match.captured(0) : match.captured(1);
                    break;
                }
            }
        }
    }

    if (devId.isEmpty()) {
        devId = QStringLiteral("washer");
    }

    if (devId.startsWith(QStringLiteral("washer")) || !devId.contains(QRegularExpression(QStringLiteral(R"(^\d+$)")))) {
        return QStringLiteral("%1.%2_%3").arg(domain, devId, propertySuffix);
    }
    return QStringLiteral("%1.midea_%2_%3").arg(domain, devId, propertySuffix);
}

QString AppController::configFilePath() const
{
    return m_configManager ? m_configManager->configPath() : QString();
}

bool AppController::configLoaded() const
{
    return m_configManager ? m_configManager->isLoaded() : false;
}

QString AppController::configStatusText() const
{
    if (!m_configManager) {
        return tr("配置管理器未初始化");
    }
    if (!m_configManager->isLoaded()) {
        if (m_configManager->configPath().isEmpty()) {
            return tr("未选择配置文件 (运行默认空配置)");
        }
        return tr("配置文件加载失败");
    }
    const AppConfig &cfg = m_configManager->config();
    return tr("已生效: %1 个摄像头, %2 个 HA 设备")
            .arg(cfg.cameras.size())
            .arg(cfg.homeAssistant.actions.size());
}

QStringList AppController::candidateConfigFiles() const
{
    QStringList candidates;
    const QString currentDir = QDir::currentPath();
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    const QStringList testPaths = {
        currentDir + QStringLiteral("/config.yaml"),
        currentDir + QStringLiteral("/config.yaml.example"),
        appDir + QStringLiteral("/config.yaml"),
        appDir + QStringLiteral("/config.yaml.example"),
        appDataDir + QStringLiteral("/config.yaml"),
        QStringLiteral("/etc/home_gui/config.yaml")
    };

    QSet<QString> seen;
    for (const QString &p : testPaths) {
        const QFileInfo fi(p);
        if (fi.exists() && fi.isFile()) {
            const QString abs = fi.absoluteFilePath();
            if (!seen.contains(abs)) {
                seen.insert(abs);
                candidates.append(abs);
            }
        }
    }
    return candidates;
}

bool AppController::loadConfigFile(const QString &fileUrlOrPath)
{
    if (!m_configManager) {
        return false;
    }
    QString path = fileUrlOrPath.trimmed();
    if (path.startsWith(QStringLiteral("file://"))) {
        path = QUrl(path).toLocalFile();
    }
    if (path.isEmpty() || !QFile::exists(path)) {
        return false;
    }
    const bool success = m_configManager->switchConfigFile(path);
    emit configFilePathChanged();
    emit configLoadedChanged();
    emit configStatusTextChanged();
    return success;
}

void AppController::resetConfigFile()
{
    if (m_configManager) {
        m_configManager->resetConfig();
        emit configFilePathChanged();
        emit configLoadedChanged();
        emit configStatusTextChanged();
    }
}

void AppController::refreshCandidateConfigFiles()
{
    emit candidateConfigFilesChanged();
}

