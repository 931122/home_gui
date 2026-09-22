#include "screenpowermanager.h"
#include "brightnesscontroller.h"

#include <QDebug>
#include <QtGlobal>
#include <QDateTime>

ScreenPowerManager::ScreenPowerManager(BrightnessController *brightnessController, QObject *parent)
    : QObject(parent)
    , m_brightnessController(brightnessController)
{
    m_idleTimer.setSingleShot(true);
    connect(&m_idleTimer, &QTimer::timeout, this, &ScreenPowerManager::requestSleep);
}

void ScreenPowerManager::initialize()
{
    if (m_brightnessController) {
        m_detectedPanelType = m_brightnessController->detectPanelType();
        m_normalBrightness = m_brightnessController->brightness();
        if (m_normalBrightness < 0.05) {
            m_normalBrightness = 0.8;
        }
    }
    qInfo() << "ScreenPowerManager: Initialized. Detected panel type:" << m_detectedPanelType
            << "Initial brightness:" << m_normalBrightness;

    resetIdleTimer();
}

void ScreenPowerManager::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    emit enabledChanged(m_enabled);
    qInfo() << "ScreenPowerManager: Enabled changed to:" << m_enabled;
    if (!m_enabled) {
        m_idleTimer.stop();
        if (m_isScreenOff) {
            requestWake(QStringLiteral("power_disabled"));
        }
    } else {
        resetIdleTimer();
    }
}

QString ScreenPowerManager::panelType() const
{
    if (m_panelConfig == QStringLiteral("auto")) {
        return m_detectedPanelType;
    }
    return m_panelConfig.toUpper();
}

void ScreenPowerManager::setPanelConfig(const QString &config)
{
    const QString normalized = config.trimmed().toLower();
    if (m_panelConfig == normalized) {
        return;
    }
    m_panelConfig = normalized;
    emit panelConfigChanged(m_panelConfig);
    emit panelTypeChanged(panelType());
    qInfo() << "ScreenPowerManager: Panel config changed to:" << m_panelConfig
            << "Effective panel type:" << panelType();
}

void ScreenPowerManager::setIdleTimeoutSeconds(int seconds)
{
    const int bounded = qMax(0, seconds);
    if (m_idleTimeoutSeconds == bounded) {
        return;
    }
    m_idleTimeoutSeconds = bounded;
    emit idleTimeoutSecondsChanged(m_idleTimeoutSeconds);
    qInfo() << "ScreenPowerManager: Idle timeout set to:" << m_idleTimeoutSeconds << "seconds";
    resetIdleTimer();
}

void ScreenPowerManager::setNormalBrightness(qreal brightness)
{
    const qreal bounded = qBound<qreal>(0.01, brightness, 1.0);
    m_normalBrightness = bounded;
    // 灭屏状态下严禁触发硬件背光点亮，仅在亮屏时单点写入硬件
    if (!m_isScreenOff && m_brightnessController) {
        m_brightnessController->setBrightness(m_normalBrightness);
    }
}

void ScreenPowerManager::setScheduledSleep(bool sleep)
{
    if (m_scheduledSleep == sleep) {
        return;
    }
    if (!m_enabled && sleep) {
        return;
    }
    m_scheduledSleep = sleep;
    qInfo() << "ScreenPowerManager: Scheduled sleep changed to:" << (sleep ? "ACTIVE" : "INACTIVE");
    if (sleep) {
        requestSleep();
    } else {
        requestWake(QStringLiteral("schedule"));
    }
}

void ScreenPowerManager::reportUserActivity(bool forceReset)
{
    if (m_isScreenOff) {
        requestWake(QStringLiteral("touch"));
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // 非强制重置（如连续滑动事件）进行 2000ms 节流，杜绝高频重启定时器风暴
    if (!forceReset && (now - m_lastActivityTime < 2000)) {
        return;
    }
    m_lastActivityTime = now;
    resetIdleTimer();
}

void ScreenPowerManager::requestSleep()
{
    if (!m_enabled || m_isScreenOff) {
        return;
    }

    // 如果当前有人体存在（后续接入人体雷达），且检测到人在，非强制计划休眠时暂不息屏
    if (!m_scheduledSleep && m_humanPresenceDetected) {
        qDebug() << "ScreenPowerManager: Sleep canceled because human presence is currently active.";
        resetIdleTimer();
        return;
    }

    m_isScreenOff = true;
    emit screenOffChanged(true);
    qInfo() << "ScreenPowerManager: Screen went to sleep. Panel:" << panelType();

    // 针对 LCD / OLED 彻底关闭物理背光（关至 0.0）达到纯黑灭屏
    if (m_brightnessController) {
        m_brightnessController->setBrightness(0.0);
    }
}

void ScreenPowerManager::requestWake(const QString &source)
{
    const bool wasOff = m_isScreenOff;
    m_isScreenOff = false;

    // 恢复正常工作亮度
    if (m_brightnessController) {
        m_brightnessController->setBrightness(m_normalBrightness);
    }

    if (wasOff) {
        emit screenOffChanged(false);
        emit screenWakeTriggered(source);
        qInfo() << "ScreenPowerManager: Screen woke up via source [" << source << "]. Restored brightness:" << m_normalBrightness;
    }

    resetIdleTimer();
}

void ScreenPowerManager::reportHumanPresence(bool detected)
{
    if (m_humanPresenceDetected == detected) {
        return;
    }
    m_humanPresenceDetected = detected;
    emit humanPresenceChanged(detected);

    qInfo() << "ScreenPowerManager: Human presence state changed ->" << (detected ? "PRESENT" : "AWAY");

    if (detected) {
        // 人体接近/存在，自动点亮屏幕！
        requestWake(QStringLiteral("human_presence"));
        // 有人期间，暂停空闲超时倒计时
        m_idleTimer.stop();
    } else {
        // 人体离开，重新开启空闲息屏倒计时
        resetIdleTimer();
    }
}

void ScreenPowerManager::resetIdleTimer()
{
    if (!m_enabled || m_isScreenOff || m_idleTimeoutSeconds <= 0 || m_humanPresenceDetected) {
        m_idleTimer.stop();
        return;
    }
    m_idleTimer.start(m_idleTimeoutSeconds * 1000);
}
