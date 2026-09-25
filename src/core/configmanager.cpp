#include "configmanager.h"

#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QTimer>

#include <sstream>
#include <string>

#include <fkYAML/node.hpp>

namespace {

QJsonValue yamlNodeToJsonValue(const fkyaml::node &node)
{
    if (node.is_null()) {
        return QJsonValue(QJsonValue::Null);
    }
    if (node.is_boolean()) {
        return QJsonValue(node.get_value<bool>());
    }
    if (node.is_integer()) {
        return QJsonValue(static_cast<qint64>(node.get_value<int64_t>()));
    }
    if (node.is_float_number()) {
        return QJsonValue(node.get_value<double>());
    }
    if (node.is_string()) {
        return QJsonValue(QString::fromStdString(node.get_value<std::string>()));
    }
    if (node.is_sequence()) {
        QJsonArray arr;
        for (const auto &item : node) {
            arr.append(yamlNodeToJsonValue(item));
        }
        return arr;
    }
    if (node.is_mapping()) {
        QJsonObject obj;
        for (auto it = node.begin(); it != node.end(); ++it) {
            const std::string key = it.key().get_value<std::string>();
            obj.insert(QString::fromStdString(key), yamlNodeToJsonValue(it.value()));
        }
        return obj;
    }
    return QJsonValue();
}

QString getYamlString(const fkyaml::node &node, const char *key, const QString &defVal = QString())
{
    if (!node.is_mapping() || !node.contains(key)) {
        return defVal;
    }
    const auto &val = node[key];
    if (val.is_string()) {
        return QString::fromStdString(val.get_value<std::string>());
    }
    if (val.is_integer()) {
        return QString::number(val.get_value<int64_t>());
    }
    if (val.is_boolean()) {
        return val.get_value<bool>() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (val.is_float_number()) {
        return QString::number(val.get_value<double>());
    }
    return defVal;
}

bool writeYamlAtomically(const QString &path, const std::string &yaml, QString &errorMessage)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        errorMessage = QObject::tr("Failed to write config file: %1").arg(path);
        return false;
    }
    const QByteArray data(yaml.data(), static_cast<int>(yaml.size()));
    if (file.write(data) != data.size() || !file.commit()) {
        errorMessage = QObject::tr("Failed to commit config file: %1").arg(path);
        return false;
    }
    return true;
}

int getYamlInt(const fkyaml::node &node, const char *key, int defVal = 0)
{
    if (!node.is_mapping() || !node.contains(key)) {
        return defVal;
    }
    const auto &val = node[key];
    if (val.is_integer()) {
        return val.get_value<int>();
    }
    if (val.is_float_number()) {
        return static_cast<int>(val.get_value<double>());
    }
    if (val.is_string()) {
        bool ok = false;
        const int parsed = QString::fromStdString(val.get_value<std::string>()).toInt(&ok);
        if (ok) {
            return parsed;
        }
    }
    return defVal;
}

bool getYamlBool(const fkyaml::node &node, const char *key, bool defVal = false)
{
    if (!node.is_mapping() || !node.contains(key)) {
        return defVal;
    }
    const auto &val = node[key];
    if (val.is_boolean()) {
        return val.get_value<bool>();
    }
    if (val.is_integer()) {
        return val.get_value<int>() != 0;
    }
    if (val.is_string()) {
        const QString s = QString::fromStdString(val.get_value<std::string>()).trimmed().toLower();
        if (s == QStringLiteral("true") || s == QStringLiteral("1") || s == QStringLiteral("yes") || s == QStringLiteral("on")) {
            return true;
        }
        if (s == QStringLiteral("false") || s == QStringLiteral("0") || s == QStringLiteral("no") || s == QStringLiteral("off")) {
            return false;
        }
    }
    return defVal;
}

void parseVideoConfigNode(const fkyaml::node &video, VideoConfig &videoConfig)
{
    videoConfig.enabled = getYamlBool(video, "enabled", false);
    videoConfig.url = getYamlString(video, "url").trimmed();
    videoConfig.backend = getYamlString(video, "backend", QStringLiteral("auto"));
    videoConfig.decoder = getYamlString(video, "decoder", QStringLiteral("auto"));
    videoConfig.renderer = getYamlString(video, "renderer", QStringLiteral("video"));
    videoConfig.cameraName = getYamlString(video, "cameraName", QStringLiteral("Front Gate"));
    videoConfig.username = getYamlString(video, "username");
    videoConfig.password = getYamlString(video, "password");
    if (videoConfig.password.isEmpty()) {
        const QString envCamPass = qEnvironmentVariable("HOME_GUI_CAMERA_PASSWORD").trimmed();
        if (!envCamPass.isEmpty()) {
            videoConfig.password = envCamPass;
        }
    }
    videoConfig.onvifProfile = getYamlString(video, "onvifProfile", QStringLiteral("auto")).trimmed();
    // 兼容旧配置：优先吃新的 url，老字段只作为迁移兜底
    if (videoConfig.url.isEmpty()) {
        const QString rtspUrl = getYamlString(video, "rtspUrl").trimmed();
        if (!rtspUrl.isEmpty()) {
            videoConfig.url = rtspUrl;
        } else {
            const QString host = getYamlString(video, "host").trimmed();
            if (!host.isEmpty()) {
                const int onvifPort = getYamlInt(video, "onvifPort", 80);
                videoConfig.url = QStringLiteral("onvif://%1:%2").arg(host).arg(onvifPort);
            }
        }
    }
}

} // namespace

ConfigManager::ConfigManager(const QString &configPath, QObject *parent)
    : QObject(parent)
    , m_configPath(configPath)
    , m_watcher(new QFileSystemWatcher(this))
{
    // 配置文件被外部改动后，直接触发重新加载。
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &ConfigManager::reloadFromDisk);
}

bool ConfigManager::load()
{
    if (m_configPath.trimmed().isEmpty()) {
        m_config = AppConfig();
        m_isLoaded = false;
        emit configLoadFailed(tr("未指定配置文件"));
        return false;
    }

    // 每次加载都重新生成一个新配置，只有解析成功才整体替换旧值。
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_isLoaded = false;
        emit configLoadFailed(tr("无法打开配置文件: %1").arg(m_configPath));
        return false;
    }

    AppConfig nextConfig;
    QString errorMessage;
    if (!parseConfig(file.readAll(), nextConfig, errorMessage)) {
        m_isLoaded = false;
        emit configLoadFailed(errorMessage);
        return false;
    }

    m_config = nextConfig;
    m_isLoaded = true;
    ensureWatch();
    emit configReloaded(m_config);
    return true;
}

const AppConfig &ConfigManager::config() const
{
    return m_config;
}

QString ConfigManager::configPath() const
{
    return m_configPath;
}

bool ConfigManager::isLoaded() const
{
    return m_isLoaded;
}

bool ConfigManager::switchConfigFile(const QString &newPath)
{
    const QString trimmedPath = newPath.trimmed();
    if (trimmedPath.isEmpty()) {
        resetConfig();
        return true;
    }

    const QFileInfo oldInfo(m_configPath);
    if (!m_configPath.isEmpty() && m_watcher->files().contains(oldInfo.absoluteFilePath())) {
        m_watcher->removePath(oldInfo.absoluteFilePath());
    }

    m_configPath = trimmedPath;
    const bool success = load();
    if (success) {
        QSettings settings;
        settings.setValue(QStringLiteral("customConfigPath"), m_configPath);
    }
    return success;
}

void ConfigManager::resetConfig()
{
    const QFileInfo oldInfo(m_configPath);
    if (!m_configPath.isEmpty() && m_watcher->files().contains(oldInfo.absoluteFilePath())) {
        m_watcher->removePath(oldInfo.absoluteFilePath());
    }

    m_configPath.clear();
    m_config = AppConfig();
    m_isLoaded = false;

    QSettings settings;
    settings.remove(QStringLiteral("customConfigPath"));

    emit configReloaded(m_config);
}

bool ConfigManager::updateCameraOnvifProfile(int cameraIndex, const QString &profile)
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit configLoadFailed(tr("Failed to open config file: %1").arg(m_configPath));
        return false;
    }

    const QByteArray rawData = file.readAll();
    file.close();

    fkyaml::node root;
    try {
        root = fkyaml::node::deserialize(std::string(rawData.constData(), rawData.size()));
    } catch (const std::exception &e) {
        emit configLoadFailed(tr("Invalid config YAML: %1").arg(QString::fromUtf8(e.what())));
        return false;
    }

    if (!root.is_mapping() || !root.contains("cameras") || !root["cameras"].is_sequence()) {
        emit configLoadFailed(tr("Invalid camera configuration in YAML"));
        return false;
    }

    if (cameraIndex < 0 || cameraIndex >= static_cast<int>(root["cameras"].size()) || !root["cameras"][cameraIndex].is_mapping()) {
        emit configLoadFailed(tr("Invalid camera index: %1").arg(cameraIndex));
        return false;
    }

    root["cameras"][cameraIndex]["onvifProfile"] = profile.trimmed().toLower().toStdString();

    std::stringstream ss;
    ss << root;
    const std::string yamlStr = ss.str();

    QString writeError;
    m_ignoreNextFileChange = true;
    if (!writeYamlAtomically(m_configPath, yamlStr, writeError)) {
        m_ignoreNextFileChange = false;
        emit configLoadFailed(writeError);
        return false;
    }

    const bool loaded = load();
    QTimer::singleShot(1000, this, [this]() {
        m_ignoreNextFileChange = false;
        ensureWatch();
    });
    return loaded;
}

bool ConfigManager::updateScreenPowerSettings(int idleTimeoutSeconds, const QString &panelConfig)
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit configLoadFailed(tr("Failed to open config file: %1").arg(m_configPath));
        return false;
    }

    const QByteArray rawData = file.readAll();
    file.close();

    fkyaml::node root;
    try {
        root = fkyaml::node::deserialize(std::string(rawData.constData(), rawData.size()));
    } catch (const std::exception &e) {
        emit configLoadFailed(tr("Invalid config YAML: %1").arg(QString::fromUtf8(e.what())));
        return false;
    }

    if (!root.is_mapping()) {
        emit configLoadFailed(tr("Invalid root configuration in YAML"));
        return false;
    }

    if (!root.contains("screenPower") || !root["screenPower"].is_mapping()) {
        root["screenPower"] = fkyaml::node::mapping();
    }

    root["screenPower"]["idleTimeoutSeconds"] = qMax(0, idleTimeoutSeconds);
    root["screenPower"]["panelConfig"] = panelConfig.trimmed().toLower().toStdString();

    std::stringstream ss;
    ss << root;
    const std::string yamlStr = ss.str();

    QString writeError;
    m_ignoreNextFileChange = true;
    if (!writeYamlAtomically(m_configPath, yamlStr, writeError)) {
        m_ignoreNextFileChange = false;
        emit configLoadFailed(writeError);
        return false;
    }

    const bool loaded = load();
    QTimer::singleShot(1000, this, [this]() {
        m_ignoreNextFileChange = false;
        ensureWatch();
    });
    return loaded;
}

PlatformConfig ConfigManager::loadBootPlatformConfig(const QString &configPath)
{
    PlatformConfig config;
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return config;
    }

    const QByteArray rawData = file.readAll();
    file.close();

    try {
        const auto root = fkyaml::node::deserialize(std::string(rawData.constData(), rawData.size()));
        if (!root.is_mapping() || !root.contains("platform")) {
            return config;
        }
        const auto &platform = root["platform"];
        config.chip = getYamlString(platform, "chip", QStringLiteral("RK3568"));
        config.width = getYamlInt(platform, "width", 800);
        config.height = getYamlInt(platform, "height", 480);
        config.renderMode = getYamlString(platform, "renderMode", QStringLiteral("eglfs"));
        config.reducedEffects = getYamlBool(platform, "reducedEffects", false);
    } catch (...) {
        return config;
    }

    return config;
}

void ConfigManager::ensureConfigFile(const QString &writableConfigPath, const QString &builtinResourcePath)
{
    QFile defaultFile(builtinResourcePath);
    if (!defaultFile.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray defaultContent = defaultFile.readAll();
    defaultFile.close();

    if (!QFile::exists(writableConfigPath)) {
        QFile targetFile(writableConfigPath);
        if (targetFile.open(QIODevice::WriteOnly)) {
            targetFile.write(defaultContent);
            targetFile.close();
        }
        return;
    }

    // 若本地可写配置存在，检查是否需要合并内置资源的凭据（token / 摄像头密码）
    QFile existingFile(writableConfigPath);
    if (!existingFile.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray existingContent = existingFile.readAll();
    existingFile.close();

    try {
        auto existingRoot = fkyaml::node::deserialize(std::string(existingContent.constData(), existingContent.size()));
        const auto defaultRoot = fkyaml::node::deserialize(std::string(defaultContent.constData(), defaultContent.size()));
        if (!existingRoot.is_mapping() || !defaultRoot.is_mapping()) {
            return;
        }

        bool modified = false;

        // 1. Home Assistant Token 补充
        if (defaultRoot.contains("homeAssistant") && defaultRoot["homeAssistant"].is_mapping()) {
            const std::string defToken = getYamlString(defaultRoot["homeAssistant"], "token").trimmed().toStdString();
            if (!defToken.empty()) {
                if (!existingRoot.contains("homeAssistant") || !existingRoot["homeAssistant"].is_mapping()) {
                    existingRoot["homeAssistant"] = fkyaml::node::mapping();
                }
                const std::string existToken = getYamlString(existingRoot["homeAssistant"], "token").trimmed().toStdString();
                if (existToken.empty()) {
                    existingRoot["homeAssistant"]["token"] = defToken;
                    modified = true;
                }
            }
        }

        // 2. 摄像头密码补充：按匹配的 URL/名称补充密码
        if (defaultRoot.contains("cameras") && defaultRoot["cameras"].is_sequence() &&
            existingRoot.contains("cameras") && existingRoot["cameras"].is_sequence()) {
            auto cameraUrl = [](const fkyaml::node &cam) -> QString {
                QString url = getYamlString(cam, "url").trimmed();
                if (url.isEmpty()) {
                    url = getYamlString(cam, "rtspUrl").trimmed();
                }
                return url;
            };

            for (size_t i = 0; i < existingRoot["cameras"].size(); ++i) {
                auto &existCam = existingRoot["cameras"][i];
                if (!existCam.is_mapping()) {
                    continue;
                }
                if (!getYamlString(existCam, "password").isEmpty()) {
                    continue;
                }

                const QString existUrl = cameraUrl(existCam);
                const QString existName = getYamlString(existCam, "cameraName").trimmed();

                for (const auto &defCam : defaultRoot["cameras"]) {
                    if (!defCam.is_mapping()) {
                        continue;
                    }
                    const QString defUrl = cameraUrl(defCam);
                    const QString defName = getYamlString(defCam, "cameraName").trimmed();
                    const bool sameUrl = !existUrl.isEmpty() && existUrl == defUrl;
                    const bool sameName = existUrl.isEmpty() && !existName.isEmpty() && existName == defName;
                    if (!sameUrl && !sameName) {
                        continue;
                    }

                    const std::string defPass = getYamlString(defCam, "password").toStdString();
                    if (!defPass.empty()) {
                        existCam["password"] = defPass;
                        modified = true;
                    }
                    break;
                }
            }
        }

        if (modified) {
            std::stringstream ss;
            ss << existingRoot;
            const std::string yamlStr = ss.str();
            QString ignoredError;
            writeYamlAtomically(writableConfigPath, yamlStr, ignoredError);
        }
    } catch (...) {
        // 若解析失败，保留现有文件
    }
}

void ConfigManager::reloadFromDisk()
{
    if (m_ignoreNextFileChange) {
        ensureWatch();
        return;
    }
    load();
}

bool ConfigManager::parseConfig(const QByteArray &payload, AppConfig &nextConfig, QString &errorMessage) const
{
    fkyaml::node root;
    try {
        root = fkyaml::node::deserialize(std::string(payload.constData(), payload.size()));
    } catch (const std::exception &e) {
        errorMessage = tr("Invalid config YAML: %1").arg(QString::fromUtf8(e.what()));
        return false;
    }

    if (!root.is_mapping()) {
        errorMessage = tr("Config root must be a YAML mapping/object");
        return false;
    }

    const auto &platform = root.contains("platform") ? root["platform"] : fkyaml::node::mapping();
    const auto &ui = root.contains("ui") ? root["ui"] : fkyaml::node::mapping();
    const auto &video = root.contains("video") ? root["video"] : fkyaml::node::mapping();
    const auto &cameras = root.contains("cameras") ? root["cameras"] : fkyaml::node::sequence();
    const auto &homeAssistant = root.contains("homeAssistant") ? root["homeAssistant"] : fkyaml::node::mapping();
    const auto &weather = root.contains("weather") ? root["weather"] : fkyaml::node::mapping();
    const auto &wifi = root.contains("wifi") ? root["wifi"] : fkyaml::node::mapping();
    const auto &screenPower = root.contains("screenPower") ? root["screenPower"] : fkyaml::node::mapping();
    const auto &xiaozhi = root.contains("xiaozhi") ? root["xiaozhi"] : fkyaml::node::mapping();

    nextConfig.platform.chip = getYamlString(platform, "chip", QStringLiteral("RK3568"));
    nextConfig.platform.width = getYamlInt(platform, "width", 800);
    nextConfig.platform.height = getYamlInt(platform, "height", 480);
    nextConfig.platform.renderMode = getYamlString(platform, "renderMode", QStringLiteral("eglfs"));
    nextConfig.platform.reducedEffects = getYamlBool(platform, "reducedEffects", false);

    QString bottomMode = getYamlString(ui, "videoBottomCardMode",
        getYamlString(ui, "video_bottom_card_mode", QStringLiteral("camera"))
    ).trimmed().toLower();
    if (bottomMode == QStringLiteral("a") || bottomMode == QStringLiteral("camera_switcher")) {
        bottomMode = QStringLiteral("camera");
    } else if (bottomMode == QStringLiteral("b") || bottomMode == QStringLiteral("scenes")) {
        bottomMode = QStringLiteral("scene");
    } else if (bottomMode == QStringLiteral("c") || bottomMode == QStringLiteral("status")) {
        bottomMode = QStringLiteral("security");
    }
    if (bottomMode != QStringLiteral("camera") && bottomMode != QStringLiteral("scene") &&
        bottomMode != QStringLiteral("security")) {
        bottomMode = QStringLiteral("camera");
    }
    nextConfig.ui.videoBottomCardMode = bottomMode;

    // 兼容单路 video 和多路 cameras 两种配置形式。
    parseVideoConfigNode(video, nextConfig.video);
    nextConfig.cameras.clear();
    if (cameras.is_sequence()) {
        for (const auto &cameraValue : cameras) {
            if (!cameraValue.is_mapping()) {
                continue;
            }
            VideoConfig cameraConfig;
            parseVideoConfigNode(cameraValue, cameraConfig);
            nextConfig.cameras.append(cameraConfig);
        }
    }
    // 如果配置了多路摄像头，默认把第一路作为当前激活视频。
    if (!nextConfig.cameras.isEmpty()) {
        nextConfig.video = nextConfig.cameras.first();
    }

    nextConfig.homeAssistant.enabled = getYamlBool(homeAssistant, "enabled", false);
    nextConfig.homeAssistant.baseUrl = getYamlString(homeAssistant, "baseUrl").trimmed();
    nextConfig.homeAssistant.token = getYamlString(homeAssistant, "token");
    const QString envHaToken = qEnvironmentVariable("HOME_GUI_HA_TOKEN").trimmed();
    if (!envHaToken.isEmpty()) {
        nextConfig.homeAssistant.token = envHaToken;
    } else if (nextConfig.homeAssistant.token.isEmpty()) {
        const QString fallbackHaToken = qEnvironmentVariable("HA_TOKEN").trimmed();
        if (!fallbackHaToken.isEmpty()) {
            nextConfig.homeAssistant.token = fallbackHaToken;
        }
    }
    nextConfig.homeAssistant.actions.clear();
    if (homeAssistant.is_mapping() && homeAssistant.contains("actions") && homeAssistant["actions"].is_sequence()) {
        for (const auto &actionValue : homeAssistant["actions"]) {
            if (!actionValue.is_mapping()) {
                continue;
            }
            HomeAssistantActionConfig actionConfig;
            actionConfig.name = getYamlString(actionValue, "name").trimmed();
            actionConfig.domain = getYamlString(actionValue, "domain").trimmed();
            actionConfig.service = getYamlString(actionValue, "service").trimmed();
            actionConfig.entityId = getYamlString(actionValue, "entityId").trimmed();
            if (actionValue.contains("data")) {
                actionConfig.data = yamlNodeToJsonValue(actionValue["data"]).toObject();
            }
            actionConfig.slideToTurnOff = getYamlBool(actionValue, "slideToTurnOff",
                getYamlBool(actionValue, "slide_to_turn_off",
                    getYamlBool(actionValue, "slideToClose",
                        getYamlBool(actionValue, "slide_to_close", false)
                    )
                )
            );
            actionConfig.isSteamer = getYamlBool(actionValue, "isSteamer",
                getYamlBool(actionValue, "is_steamer", false)
            );
            actionConfig.socketEntity = getYamlString(actionValue, "socketEntity",
                getYamlString(actionValue, "socket_entity")
            ).trimmed();
            if (actionConfig.socketEntity.isEmpty() && actionConfig.data.contains(QStringLiteral("socket_entity"))) {
                actionConfig.socketEntity = actionConfig.data.value(QStringLiteral("socket_entity")).toString().trimmed();
            }
            actionConfig.timerEntity = getYamlString(actionValue, "timerEntity",
                getYamlString(actionValue, "timer_entity")
            ).trimmed();
            if (actionConfig.timerEntity.isEmpty() && actionConfig.data.contains(QStringLiteral("timer_entity"))) {
                actionConfig.timerEntity = actionConfig.data.value(QStringLiteral("timer_entity")).toString().trimmed();
            }
            actionConfig.modeEntity = getYamlString(actionValue, "modeEntity",
                getYamlString(actionValue, "mode_entity")
            ).trimmed();
            if (actionConfig.modeEntity.isEmpty() && actionConfig.data.contains(QStringLiteral("mode_entity"))) {
                actionConfig.modeEntity = actionConfig.data.value(QStringLiteral("mode_entity")).toString().trimmed();
            }
            actionConfig.stopService = getYamlString(actionValue, "stopService",
                getYamlString(actionValue, "stop_service")
            ).trimmed();
            if (actionConfig.stopService.isEmpty() && actionConfig.data.contains(QStringLiteral("stop_service"))) {
                actionConfig.stopService = actionConfig.data.value(QStringLiteral("stop_service")).toString().trimmed();
            }
            if (!actionConfig.name.isEmpty() && !actionConfig.domain.isEmpty() && !actionConfig.service.isEmpty()) {
                nextConfig.homeAssistant.actions.append(actionConfig);
            }
        }
    }

    nextConfig.weather.enabled = getYamlBool(weather, "enabled", false);
    nextConfig.weather.provider = getYamlString(weather, "provider", QStringLiteral("china_weather"));
    nextConfig.weather.city = getYamlString(weather, "city",
        getYamlString(weather, "cityName", QStringLiteral("北京"))
    ).trimmed();
    nextConfig.weather.areaId = getYamlString(weather, "areaId").trimmed();
    nextConfig.weather.apiKey = getYamlString(weather, "apiKey");
    nextConfig.weather.cityAdcode = getYamlString(weather, "cityAdcode");
    nextConfig.weather.refreshMinutes = getYamlInt(weather, "refreshMinutes", 30);

    const QString envCity = qEnvironmentVariable("WEATHER_CITY").trimmed();
    if (!envCity.isEmpty()) {
        nextConfig.weather.city = envCity;
    }

    nextConfig.wifi.enabled = getYamlBool(wifi, "enabled", true);
    nextConfig.wifi.interfaceName = getYamlString(wifi, "interface");
    nextConfig.wifi.ssid = getYamlString(wifi, "ssid");
    nextConfig.wifi.password = getYamlString(wifi, "password");

    nextConfig.screenPower.enabled = getYamlBool(screenPower, "enabled", false);
    nextConfig.screenPower.blankStart = getYamlString(screenPower, "blankStart", QStringLiteral("23:00")).trimmed();
    nextConfig.screenPower.blankEnd = getYamlString(screenPower, "blankEnd", QStringLiteral("07:00")).trimmed();
    nextConfig.screenPower.temporaryWakeMinutes = qBound(1,
                                                         getYamlInt(screenPower, "temporaryWakeMinutes", 5),
                                                         240);
    nextConfig.screenPower.panelConfig = getYamlString(screenPower, "panelConfig", QStringLiteral("auto")).trimmed().toLower();
    nextConfig.screenPower.idleTimeoutSeconds = qMax(0, getYamlInt(screenPower, "idleTimeoutSeconds", 180));

    nextConfig.xiaozhi.enabled = getYamlBool(xiaozhi, "enabled", false);
    nextConfig.xiaozhi.autoStartAudio = getYamlBool(xiaozhi, "autoStartAudio", true);
    nextConfig.xiaozhi.autoListenOnConnect = getYamlBool(xiaozhi, "autoListenOnConnect", false);
    nextConfig.xiaozhi.soundAppPath = getYamlString(xiaozhi, "soundAppPath", QStringLiteral("xiaozhi_sound_app")).trimmed();
    nextConfig.xiaozhi.configPath = getYamlString(xiaozhi, "configPath", QStringLiteral("/etc/xiaozhi.cfg")).trimmed();
    nextConfig.xiaozhi.otaUrl = getYamlString(xiaozhi, "otaUrl", QStringLiteral("https://api.tenclass.net/xiaozhi/ota/")).trimmed();
    nextConfig.xiaozhi.websocketHost = getYamlString(xiaozhi, "websocketHost", QStringLiteral("api.tenclass.net")).trimmed();
    nextConfig.xiaozhi.websocketPort = getYamlInt(xiaozhi, "websocketPort", 443);
    nextConfig.xiaozhi.websocketPath = getYamlString(xiaozhi, "websocketPath", QStringLiteral("/xiaozhi/v1/")).trimmed();
    nextConfig.xiaozhi.authToken = getYamlString(xiaozhi, "authToken").trimmed();
    nextConfig.xiaozhi.userAgent = getYamlString(xiaozhi, "userAgent", QStringLiteral("weidongshan1")).trimmed();
    nextConfig.xiaozhi.language = getYamlString(xiaozhi, "language", QStringLiteral("zh-CN")).trimmed();
    nextConfig.xiaozhi.applicationName = getYamlString(xiaozhi, "applicationName", QStringLiteral("xiaozhi_linux_100ask")).trimmed();
    nextConfig.xiaozhi.applicationVersion = getYamlString(xiaozhi, "applicationVersion", QStringLiteral("1.0.0")).trimmed();
    nextConfig.xiaozhi.boardType = getYamlString(xiaozhi, "boardType", QStringLiteral("rk3506_linux_board")).trimmed();
    nextConfig.xiaozhi.boardName = getYamlString(xiaozhi, "boardName", QStringLiteral("rk3506_home_gui")).trimmed();
    nextConfig.xiaozhi.wakeGpioLine = getYamlInt(xiaozhi, "wakeGpioLine", -1);
    nextConfig.xiaozhi.wakeGpioActiveLow = getYamlBool(xiaozhi, "wakeGpioActiveLow", false);
    nextConfig.xiaozhi.wakeGpioDebounceMs = qBound(20,
                                                   getYamlInt(xiaozhi, "wakeGpioDebounceMs", 80),
                                                   2000);

    return true;
}

void ConfigManager::ensureWatch()
{
    const QFileInfo info(m_configPath);
    if (!info.exists()) {
        return;
    }

    // QFileSystemWatcher 需要监听绝对路径，否则热加载不稳定。
    if (!m_watcher->files().contains(info.absoluteFilePath())) {
        m_watcher->addPath(info.absoluteFilePath());
    }
}
