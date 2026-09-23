#include "brightnesscontroller.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QtGlobal>

#if defined(Q_OS_ANDROID)
#include <QtCore/QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

qreal BrightnessController::clampBrightness(qreal brightness)
{
    return qBound<qreal>(0.0, brightness, 1.0);
}

QString BrightnessController::firstLine(const QString &value)
{
    const int newlineIndex = value.indexOf(QLatin1Char('\n'));
    return newlineIndex >= 0 ? value.left(newlineIndex).trimmed() : value.trimmed();
}

bool BrightnessController::initialize()
{
#if defined(Q_OS_ANDROID)
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        jfloat sysBri = QJniObject::callStaticMethod<jfloat>(
            "org/qtproject/example/home_gui/AndroidBrightnessHelper",
            "getSystemBrightness",
            "(Landroid/app/Activity;)F",
            activity.object<jobject>()
        );
        if (sysBri > 0.01f) {
            m_brightness = clampBrightness(static_cast<qreal>(sysBri));
        } else {
            m_brightness = 0.8;
        }
        m_hardwareBrightnessAvailable = true;
        setBrightness(m_brightness);
        return true;
    }
#endif

    const QDir backlightRoot(QStringLiteral("/sys/class/backlight"));
    const QFileInfoList entries = backlightRoot.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &entry : entries) {
        const QString basePath = entry.absoluteFilePath();
        QFile maxFile(basePath + QStringLiteral("/max_brightness"));
        QFile currentFile(basePath + QStringLiteral("/brightness"));
        if (!maxFile.open(QIODevice::ReadOnly | QIODevice::Text)
            || !currentFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        bool maxOk = false;
        bool currentOk = false;
        const int maxBrightness = firstLine(QString::fromLocal8Bit(maxFile.readAll())).toInt(&maxOk);
        const int currentBrightness = firstLine(QString::fromLocal8Bit(currentFile.readAll())).toInt(&currentOk);
        if (!maxOk || !currentOk || maxBrightness <= 0) {
            continue;
        }

        m_backlightBrightnessPath = basePath + QStringLiteral("/brightness");
        m_backlightMaxBrightness = maxBrightness;
        m_brightness = clampBrightness(static_cast<qreal>(currentBrightness) / static_cast<qreal>(maxBrightness));
        m_hardwareBrightnessAvailable = true;
        return true;
    }

    m_backlightBrightnessPath.clear();
    m_backlightMaxBrightness = 0;
    m_brightness = 1.0;
    m_hardwareBrightnessAvailable = false;
    return false;
}

qreal BrightnessController::brightness() const
{
    return m_brightness;
}

bool BrightnessController::hardwareAvailable() const
{
    return m_hardwareBrightnessAvailable;
}

QString BrightnessController::brightnessPath() const
{
    return m_backlightBrightnessPath;
}

bool BrightnessController::setBrightness(qreal brightness)
{
    const qreal clamped = clampBrightness(brightness);

#if defined(Q_OS_ANDROID)
    m_brightness = clamped;
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        QJniObject::callStaticMethod<void>(
            "org/qtproject/example/home_gui/AndroidBrightnessHelper",
            "setWindowBrightness",
            "(Landroid/app/Activity;F)V",
            activity.object<jobject>(),
            static_cast<jfloat>(m_brightness)
        );
        return true;
    }
    return false;
#endif

    if (!m_hardwareBrightnessAvailable) {
        m_brightness = clamped;
        return true;
    }

    if (m_backlightBrightnessPath.isEmpty() || m_backlightMaxBrightness <= 0) {
        return false;
    }

    const int rawBrightness = (clamped <= 0.0)
            ? 0
            : qBound(1, qRound(clamped * m_backlightMaxBrightness), m_backlightMaxBrightness);
    QFile brightnessFile(m_backlightBrightnessPath);
    if (!brightnessFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream stream(&brightnessFile);
    stream << rawBrightness;
    stream.flush();
    if (brightnessFile.error() != QFile::NoError) {
        return false;
    }

    m_brightness = clamped;
    return true;
}

QString BrightnessController::detectPanelType() const
{
#if defined(Q_OS_ANDROID)
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        QJniObject jResult = QJniObject::callStaticObjectMethod(
            "org/qtproject/example/home_gui/AndroidBrightnessHelper",
            "detectPanelType",
            "(Landroid/app/Activity;)Ljava/lang/String;",
            activity.object<jobject>()
        );
        if (jResult.isValid()) {
            return jResult.toString();
        }
    }
#endif
    return QStringLiteral("LCD");
}
