#ifndef SCREENPOWERMANAGER_H
#define SCREENPOWERMANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>

class BrightnessController;

/**
 * 屏幕电源与待机管理器
 * 
 * 核心职责：
 * 1. 自动探测屏幕材质 (LCD vs OLED)，针对 LCD 提供背光物理关断 (0 亮度) 真正灭屏；
 * 2. 息屏后轻触屏幕任意位置即刻唤醒恢复 (Tap to Wake)；
 * 3. 插件式唤醒源架构 (Plug-in Wake Source Architecture)，原生预留人体传感器 / 雷达存在感知接口；
 * 4. 自动空闲息屏计时管理 (支持自定义超时或保持常亮)。
 */
class ScreenPowerManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool isScreenOff READ isScreenOff NOTIFY screenOffChanged)
    Q_PROPERTY(QString panelType READ panelType NOTIFY panelTypeChanged)
    Q_PROPERTY(QString panelConfig READ panelConfig WRITE setPanelConfig NOTIFY panelConfigChanged)
    Q_PROPERTY(QString detectedPanelType READ detectedPanelType CONSTANT)
    Q_PROPERTY(int idleTimeoutSeconds READ idleTimeoutSeconds WRITE setIdleTimeoutSeconds NOTIFY idleTimeoutSecondsChanged)
    Q_PROPERTY(bool humanPresenceDetected READ humanPresenceDetected NOTIFY humanPresenceChanged)

public:
    explicit ScreenPowerManager(BrightnessController *brightnessController, QObject *parent = nullptr);
    ~ScreenPowerManager() override = default;

    void initialize();

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool isScreenOff() const { return m_isScreenOff; }
    QString panelType() const;
    QString panelConfig() const { return m_panelConfig; }
    void setPanelConfig(const QString &config);
    QString detectedPanelType() const { return m_detectedPanelType; }

    int idleTimeoutSeconds() const { return m_idleTimeoutSeconds; }
    void setIdleTimeoutSeconds(int seconds);

    bool humanPresenceDetected() const { return m_humanPresenceDetected; }

    // 工作背光记录（用户设置的正常背光）
    qreal normalBrightness() const { return m_normalBrightness; }
    void setNormalBrightness(qreal brightness);

    bool scheduledSleep() const { return m_scheduledSleep; }
    void setScheduledSleep(bool sleep);

public slots:
    // 报告用户活动（点击、滑动等），重置息屏计时器；forceReset 为 false 时对高频移动节流
    void reportUserActivity(bool forceReset = false);

    // 请求唤醒屏幕（可指明唤醒源："touch", "human_presence", "voice", "manual" 等）
    void requestWake(const QString &source = QStringLiteral("touch"));

    // 请求立即进入灭屏
    void requestSleep();

    // ==========================================
    // 预留人体传感器 / 毫米波雷达存在感应统一插槽
    // 当后续接入 Home Assistant 人体传感器实体或硬件雷达时，
    // 只需调用此槽或连接信号即可全自动协同！
    // ==========================================
    void reportHumanPresence(bool detected);

signals:
    void enabledChanged(bool enabled);
    void screenOffChanged(bool isOff);
    void panelTypeChanged(const QString &type);
    void panelConfigChanged(const QString &config);
    void idleTimeoutSecondsChanged(int seconds);
    void humanPresenceChanged(bool detected);
    void screenWakeTriggered(const QString &source);

private:
    void evaluatePanelType();
    void resetIdleTimer();

    BrightnessController *m_brightnessController = nullptr;

    bool m_enabled = false;
    bool m_isScreenOff = false;
    bool m_scheduledSleep = false;
    QString m_detectedPanelType = QStringLiteral("LCD");
    QString m_panelConfig = QStringLiteral("auto"); // "auto", "lcd", "oled"
    int m_idleTimeoutSeconds = 180; // 默认 3 分钟无操作自动灭屏 (0 表示从不)
    bool m_humanPresenceDetected = false;

    qreal m_normalBrightness = 1.0;
    qint64 m_lastActivityTime = 0;

    QTimer m_idleTimer;
};

#endif // SCREENPOWERMANAGER_H
