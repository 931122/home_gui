#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QMetaType>
#include <QJsonObject>
#include <QList>

// 平台级配置，既影响启动阶段的环境设置，也影响界面的分辨率缩放。
struct PlatformConfig
{
    QString chip;
    int width = 800;
    int height = 480;
    QString renderMode;
    bool reducedEffects = false;
};

// 单路摄像头配置。
// 可以直接给 RTSP 地址，也可以只给 ONVIF 参数让程序自动解析流地址。
struct VideoConfig
{
    bool enabled = false;
    QString url;
    QString backend;
    QString decoder;
    QString renderer;
    QString cameraName;
    QString username;
    QString password;
    QString onvifProfile;
};

struct HomeAssistantActionConfig
{
    QString name;
    QString domain;
    QString service;
    QString entityId;
    QJsonObject data;
    bool slideToTurnOff = false;
    bool isSteamer = false;
    QString socketEntity;
    QString timerEntity;
    QString modeEntity;
    QString stopService;
};

// Home Assistant 相关配置，使用 REST API 调用服务控制实体。
struct HomeAssistantConfig
{
    bool enabled = false;
    QString baseUrl;
    QString token;
    QList<HomeAssistantActionConfig> actions;
};

// 天气服务配置，支持中国天气网（china_weather）数据源。
struct WeatherConfig
{
    bool enabled = false;
    QString provider = QStringLiteral("china_weather");
    QString city = QStringLiteral("北京");
    QString areaId = QStringLiteral("101010100");
    QString apiKey;
    QString cityAdcode;
    int refreshMinutes = 30;
};

struct WifiConfig
{
    bool enabled = true;
    QString interfaceName;
    QString ssid;
    QString password;
};

struct ScreenPowerConfig
{
    bool enabled = false;
    QString blankStart;
    QString blankEnd;
    int temporaryWakeMinutes = 5;
    QString panelConfig = QStringLiteral("auto"); // "auto", "oled", "lcd"
    int idleTimeoutSeconds = 180;                 // 空闲自动灭屏超时秒数 (0 为从不灭屏)
};

struct XiaozhiConfig
{
    bool enabled = false;
    bool autoStartAudio = true;
    bool autoListenOnConnect = false;
    QString soundAppPath;
    QString configPath;
    QString otaUrl;
    QString websocketHost;
    int websocketPort = 443;
    QString websocketPath;
    QString authToken;
    QString userAgent;
    QString language;
    QString applicationName;
    QString applicationVersion;
    QString boardType;
    QString boardName;
    int wakeGpioLine = -1;
    bool wakeGpioActiveLow = false;
    int wakeGpioDebounceMs = 80;
};

struct UiConfig
{
    QString videoBottomCardMode = QStringLiteral("camera"); // "camera" (A: 多路监控), "scene" (B: 快捷场景), "security" (C: 安防速览)
};

// 整个程序运行时使用的配置对象，对应 config.yaml（模板见 config.yaml.example）。
struct AppConfig
{
    PlatformConfig platform;
    UiConfig ui;
    VideoConfig video;
    QList<VideoConfig> cameras;
    HomeAssistantConfig homeAssistant;
    WeatherConfig weather;
    WifiConfig wifi;
    ScreenPowerConfig screenPower;
    XiaozhiConfig xiaozhi;
};

Q_DECLARE_METATYPE(PlatformConfig)
Q_DECLARE_METATYPE(UiConfig)
Q_DECLARE_METATYPE(VideoConfig)
Q_DECLARE_METATYPE(HomeAssistantActionConfig)
Q_DECLARE_METATYPE(HomeAssistantConfig)
Q_DECLARE_METATYPE(WeatherConfig)
Q_DECLARE_METATYPE(WifiConfig)
Q_DECLARE_METATYPE(ScreenPowerConfig)
Q_DECLARE_METATYPE(XiaozhiConfig)
Q_DECLARE_METATYPE(AppConfig)

#endif
