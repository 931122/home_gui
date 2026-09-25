#include "modulemanager.h"

#include "configmanager.h"
#include "globalstate.h"
#include "imodule.h"
#include "modules/homeassistant/homeassistantmodule.h"
#include "modules/video/videomodule.h"
#include "modules/wifi/wifimodule.h"
#include "modules/xiaozhi/xiaozhimodule.h"

ModuleManager::ModuleManager(ConfigManager *configManager, GlobalState *globalState, QObject *parent)
    : QObject(parent)
    , m_configManager(configManager)
    , m_globalState(globalState)
{
}

ModuleManager::~ModuleManager()
{
    for (IModule *module : m_modules) {
        module->stop();
    }
}

void ModuleManager::startModules()
{
    ensureModules();
    AppConfig config = m_configManager->config();
    // 多摄像头模式默认启动第一路，真正切换由 selectCamera 处理。
    config.video = activeVideoConfig(config);
    m_activeCameraName = config.video.cameraName;
    m_activeCameraUrl = config.video.url;
    applyPlatformState(config);
    for (IModule *module : m_modules) {
        module->applyConfig(config);
        module->start();
    }
    emit activeCameraChanged(activeCameraIndex(config));
}

void ModuleManager::restartModules(const AppConfig &config)
{
    ensureModules();
    // 配置热更新和网络恢复都走这里，保证所有模块采用同一套重启顺序。
    applyPlatformState(config);
    for (IModule *module : m_modules) {
        module->stop();
        module->applyConfig(config);
        module->start();
    }
}

void ModuleManager::reloadModules(const AppConfig &config)
{
    ensureModules();
    AppConfig effectiveConfig = config;
    effectiveConfig.video = activeVideoConfig(effectiveConfig);
    m_activeCameraName = effectiveConfig.video.cameraName;
    m_activeCameraUrl = effectiveConfig.video.url;
    applyPlatformState(effectiveConfig);
    // 当前采用“停掉再重启”的方式重载模块，逻辑更直接，也便于调试。
    restartModules(effectiveConfig);
    emit activeCameraChanged(activeCameraIndex(effectiveConfig));
}

void ModuleManager::moveVideoPtz(const QString &direction)
{
    ensureModules();
    if (m_videoModule != nullptr) {
        m_videoModule->movePtz(direction);
    }
}

void ModuleManager::selectCamera(const VideoConfig &videoConfig)
{
    ensureModules();
    if (m_videoModule == nullptr) {
        return;
    }

    // 切换摄像头只替换当前激活的视频配置，不改原始 cameras 列表本身。
    AppConfig effectiveConfig = m_configManager->config();
    effectiveConfig.video = videoConfig;
    m_activeCameraName = videoConfig.cameraName;
    m_activeCameraUrl = videoConfig.url;
    applyPlatformState(effectiveConfig);
    m_videoModule->stop();
    m_videoModule->applyConfig(effectiveConfig);
    m_videoModule->start();
    emit activeCameraChanged(activeCameraIndex(effectiveConfig));
}

void ModuleManager::triggerHomeAssistantAction(const QString &actionName)
{
    ensureModules();
    if (m_homeAssistantModule != nullptr) {
        m_homeAssistantModule->triggerAction(actionName);
    }
}

void ModuleManager::callHomeAssistantActionService(const QString &actionName, const QString &service)
{
    ensureModules();
    if (m_homeAssistantModule != nullptr) {
        m_homeAssistantModule->callActionService(actionName, service);
    }
}

void ModuleManager::callHomeAssistantCustomService(const QString &domain, const QString &service, const QVariantMap &data)
{
    ensureModules();
    if (m_homeAssistantModule != nullptr) {
        m_homeAssistantModule->callCustomService(domain, service, data);
    }
}

void ModuleManager::setHomeAssistantLightBrightness(const QString &actionName, qreal brightness)
{
    ensureModules();
    if (m_homeAssistantModule != nullptr) {
        m_homeAssistantModule->setLightBrightness(actionName, brightness);
    }
}

void ModuleManager::setHomeAssistantCoverPosition(const QString &actionName, qreal position)
{
    ensureModules();
    if (m_homeAssistantModule != nullptr) {
        m_homeAssistantModule->setCoverPosition(actionName, position);
    }
}

void ModuleManager::setHomeAssistantMediaVolume(const QString &actionName, qreal volume)
{
    ensureModules();
    if (m_homeAssistantModule != nullptr) {
        m_homeAssistantModule->setMediaPlayerVolume(actionName, volume);
    }
}

void ModuleManager::scanWifiNetworks()
{
    ensureModules();
    if (m_wifiModule != nullptr) {
        m_wifiModule->scanNetworks();
    }
}

void ModuleManager::connectWifiNetwork(const QString &ssid, const QString &password)
{
    ensureModules();
    if (m_wifiModule != nullptr) {
        m_wifiModule->connectToNetwork(ssid, password);
    }
}

void ModuleManager::forgetWifiNetwork(const QString &ssid)
{
    ensureModules();
    if (m_wifiModule != nullptr) {
        m_wifiModule->forgetNetwork(ssid);
    }
}

void ModuleManager::wakeXiaozhi()
{
    ensureModules();
    if (m_xiaozhiModule != nullptr) {
        m_xiaozhiModule->wake();
    }
}

void ModuleManager::hideXiaozhiOverlay()
{
    ensureModules();
    if (m_xiaozhiModule != nullptr) {
        m_xiaozhiModule->hideOverlay();
    }
}

void ModuleManager::ensureModules()
{
    if (!m_modules.isEmpty()) {
        return;
    }

    // worker 线程创建放到首帧之后，避免启动阶段抢占 QML 第一帧渲染。
    m_videoModule = new VideoModule(m_globalState, this);
    m_homeAssistantModule = new HomeAssistantModule(m_globalState, this);
    m_wifiModule = new WifiModule(m_globalState, this);
    m_xiaozhiModule = new XiaozhiModule(m_globalState, this);

    m_modules.append(m_videoModule);
    m_modules.append(m_homeAssistantModule);
    m_modules.append(m_wifiModule);
    m_modules.append(m_xiaozhiModule);
}

void ModuleManager::applyPlatformState(const AppConfig &config)
{
    // 这里把平台能力折算成前端可直接展示/判断的状态。
    const QString label = QStringLiteral("%1 %2x%3 (%4)")
            .arg(config.platform.chip)
            .arg(config.platform.width)
            .arg(config.platform.height)
            .arg(config.platform.renderMode);

    m_globalState->setPlatform(label);
    const bool softwareRendering = config.platform.renderMode.compare(QStringLiteral("linuxfb"), Qt::CaseInsensitive) == 0;
    m_globalState->setReducedEffects(config.platform.reducedEffects || softwareRendering);
}

VideoConfig ModuleManager::activeVideoConfig(const AppConfig &config) const
{
    if (config.cameras.isEmpty()) {
        return config.video;
    }

    for (const VideoConfig &cameraConfig : config.cameras) {
        if (!m_activeCameraUrl.isEmpty()
            && cameraConfig.url == m_activeCameraUrl
            && cameraConfig.cameraName == m_activeCameraName) {
            return cameraConfig;
        }
    }

    return config.cameras.first();
}

int ModuleManager::currentCameraIndex() const
{
    return activeCameraIndex(m_configManager->config());
}

int ModuleManager::activeCameraIndex(const AppConfig &config) const
{
    if (config.cameras.isEmpty()) {
        return 0;
    }

    for (int index = 0; index < config.cameras.size(); ++index) {
        const VideoConfig &cameraConfig = config.cameras.at(index);
        if (!m_activeCameraUrl.isEmpty()
            && cameraConfig.url == m_activeCameraUrl
            && cameraConfig.cameraName == m_activeCameraName) {
            return index;
        }
    }

    return 0;
}
