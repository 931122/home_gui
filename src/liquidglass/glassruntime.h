#ifndef GLASSRUNTIME_H
#define GLASSRUNTIME_H

#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>
#include <QVector2D>

class GlassRuntime : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *backdropSource READ backdropSource NOTIFY backdropSourceChanged)
    Q_PROPERTY(QVector2D tilt READ tilt NOTIFY tiltChanged)
    Q_PROPERTY(qreal backdropLuminance READ backdropLuminance NOTIFY backdropLuminanceChanged)
    Q_PROPERTY(bool highContrast READ highContrast NOTIFY accessibilityStateChanged)
    Q_PROPERTY(bool reduceMotion READ reduceMotion NOTIFY accessibilityStateChanged)
    Q_PROPERTY(bool batterySaver READ batterySaver NOTIFY accessibilityStateChanged)
    Q_PROPERTY(bool accessibilityFallback READ accessibilityFallback NOTIFY accessibilityStateChanged)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled NOTIFY accessibilityStateChanged)

public:
    explicit GlassRuntime(QObject *parent = nullptr);

    QVector2D tilt() const;
    QQuickItem *backdropSource() const;
    qreal backdropLuminance() const;
    bool highContrast() const;
    bool reduceMotion() const;
    bool batterySaver() const;
    bool accessibilityFallback() const;
    bool animationsEnabled() const;

    Q_INVOKABLE void setBackdropSource(QQuickItem *source);
    Q_INVOKABLE void refreshAccessibilityState();

signals:
    void tiltChanged();
    void backdropSourceChanged();
    void backdropLuminanceChanged();
    void accessibilityStateChanged();

private:
    void captureBackdrop();
    void setTilt(const QVector2D &tilt);
    void setHighContrast(bool enabled);
    void setReduceMotion(bool enabled);
    void setBatterySaver(bool enabled);

    QPointer<QQuickItem> m_backdropSource;
    QTimer m_backdropTimer;
    QTimer m_accessibilityTimer;
    QVector2D m_tilt;
    qreal m_backdropLuminance = 0.18;
    bool m_highContrast = false;
    bool m_reduceMotion = false;
    bool m_batterySaver = false;
    bool m_capturePending = false;
};

#endif
