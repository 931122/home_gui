#include "glassruntime.h"

#include <QGuiApplication>
#include <QQuickItemGrabResult>
#include <QImage>

#if defined(Q_OS_ANDROID)
#include <QtCore/QJniObject>
#include <QtCore/qnativeinterface.h>
#endif

namespace {

bool environmentFlag(const char *name)
{
    const QByteArray value = qgetenv(name).trimmed().toLower();
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

qreal imageLuminance(const QImage &image)
{
    if (image.isNull()) {
        return -1.0;
    }

    const QImage pixels = image.convertToFormat(QImage::Format_RGB32);
    quint64 total = 0;
    for (int y = 0; y < pixels.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(pixels.constScanLine(y));
        for (int x = 0; x < pixels.width(); ++x) {
            total += qRound((0.2126 * qRed(line[x]) + 0.7152 * qGreen(line[x]) + 0.0722 * qBlue(line[x])) / 255.0 * 10000.0);
        }
    }

    const int count = pixels.width() * pixels.height();
    return count > 0 ? qreal(total) / (qreal(count) * 10000.0) : -1.0;
}

}

GlassRuntime::GlassRuntime(QObject *parent)
    : QObject(parent)
{
    m_backdropTimer.setInterval(10000);
    connect(&m_backdropTimer, &QTimer::timeout, this, &GlassRuntime::captureBackdrop);

    refreshAccessibilityState();
#if defined(Q_OS_ANDROID)
    m_accessibilityTimer.setInterval(5000);
    connect(&m_accessibilityTimer, &QTimer::timeout, this, &GlassRuntime::refreshAccessibilityState);
    m_accessibilityTimer.start();

    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        QJniObject::callStaticMethod<void>("org/qtproject/example/home_gui/AndroidGlassRuntime",
                                           "start",
                                           "(Landroid/app/Activity;)V",
                                           activity.object<jobject>());
    }

    QTimer *sensorTimer = new QTimer(this);
    sensorTimer->setInterval(50);
    connect(sensorTimer, &QTimer::timeout, this, [this]() {
        const jfloat x = QJniObject::callStaticMethod<jfloat>("org/qtproject/example/home_gui/AndroidGlassRuntime", "tiltX", "()F");
        const jfloat y = QJniObject::callStaticMethod<jfloat>("org/qtproject/example/home_gui/AndroidGlassRuntime", "tiltY", "()F");
        setTilt(QVector2D(x, y));
    });
    sensorTimer->start();
#else
    const bool isLinuxFb = QGuiApplication::platformName().compare(QStringLiteral("linuxfb"), Qt::CaseInsensitive) == 0;
    setHighContrast(environmentFlag("HOME_GUI_HIGH_CONTRAST"));
    setReduceMotion(isLinuxFb || environmentFlag("HOME_GUI_REDUCE_MOTION"));
    setBatterySaver(isLinuxFb || environmentFlag("HOME_GUI_BATTERY_SAVER"));
#endif
}

QVector2D GlassRuntime::tilt() const
{
    return m_tilt;
}

QQuickItem *GlassRuntime::backdropSource() const
{
    return m_backdropSource.data();
}

qreal GlassRuntime::backdropLuminance() const
{
    return m_backdropLuminance;
}

bool GlassRuntime::highContrast() const
{
    return m_highContrast;
}

bool GlassRuntime::reduceMotion() const
{
    return m_reduceMotion;
}

bool GlassRuntime::batterySaver() const
{
    return m_batterySaver;
}

bool GlassRuntime::accessibilityFallback() const
{
    return m_highContrast || m_batterySaver;
}

bool GlassRuntime::animationsEnabled() const
{
    return !m_reduceMotion && !m_batterySaver;
}

void GlassRuntime::setBackdropSource(QQuickItem *source)
{
    if (m_backdropSource == source) {
        return;
    }
    m_backdropSource = source;
    if (source) {
        connect(source, &QObject::destroyed, this, [this]() {
            if (m_backdropSource.isNull()) {
                m_backdropTimer.stop();
                emit backdropSourceChanged();
            }
        });
    }
    emit backdropSourceChanged();
    if (m_backdropSource) {
        QTimer::singleShot(500, this, &GlassRuntime::captureBackdrop);
    } else {
        m_backdropTimer.stop();
    }
}

void GlassRuntime::refreshAccessibilityState()
{
#if defined(Q_OS_ANDROID)
    const jboolean contrast = QJniObject::callStaticMethod<jboolean>("org/qtproject/example/home_gui/AndroidGlassRuntime", "highContrast", "()Z");
    const jboolean motion = QJniObject::callStaticMethod<jboolean>("org/qtproject/example/home_gui/AndroidGlassRuntime", "reduceMotion", "()Z");
    const jboolean saver = QJniObject::callStaticMethod<jboolean>("org/qtproject/example/home_gui/AndroidGlassRuntime", "batterySaver", "()Z");
    setHighContrast(contrast);
    setReduceMotion(motion);
    setBatterySaver(saver);
#else
    setHighContrast(environmentFlag("HOME_GUI_HIGH_CONTRAST"));
    setReduceMotion(environmentFlag("HOME_GUI_REDUCE_MOTION"));
    setBatterySaver(environmentFlag("HOME_GUI_BATTERY_SAVER"));
#endif
}

void GlassRuntime::captureBackdrop()
{
    if (!m_backdropSource || m_capturePending || !m_backdropSource->window()) {
        return;
    }

    m_capturePending = true;
    const auto result = m_backdropSource->grabToImage(QSize(24, 24));
    if (!result) {
        m_capturePending = false;
        return;
    }

    connect(result.data(), &QQuickItemGrabResult::ready, this, [this, result]() {
        m_capturePending = false;
        const qreal luminance = imageLuminance(result->image());
        if (luminance >= 0.0 && qAbs(luminance - m_backdropLuminance) > 0.015) {
            m_backdropLuminance = luminance;
            emit backdropLuminanceChanged();
        }
    });
}

void GlassRuntime::setTilt(const QVector2D &tilt)
{
    const QVector2D clamped(qBound(-1.0f, tilt.x(), 1.0f), qBound(-1.0f, tilt.y(), 1.0f));
    if ((clamped - m_tilt).lengthSquared() < 0.0025f) {
        return;
    }
    m_tilt = clamped;
    emit tiltChanged();
}

void GlassRuntime::setHighContrast(bool enabled)
{
    if (m_highContrast == enabled) {
        return;
    }
    m_highContrast = enabled;
    emit accessibilityStateChanged();
}

void GlassRuntime::setReduceMotion(bool enabled)
{
    if (m_reduceMotion == enabled) {
        return;
    }
    m_reduceMotion = enabled;
    emit accessibilityStateChanged();
}

void GlassRuntime::setBatterySaver(bool enabled)
{
    if (m_batterySaver == enabled) {
        return;
    }
    m_batterySaver = enabled;
    emit accessibilityStateChanged();
}
