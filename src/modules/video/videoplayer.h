#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QUrl>

#include "core/appconfig.h"

class AbstractVideoBackend;

class VideoPlayer : public QObject
{
    Q_OBJECT

public:
    explicit VideoPlayer(QObject *parent = nullptr);
    ~VideoPlayer() override;

    QUrl source() const;
    void setSource(const QUrl &source);

    QString backend() const;
    void setBackend(const QString &backend);

    QString decoder() const;
    void setDecoder(const QString &decoder);

    bool autoPlay() const;
    void setAutoPlay(bool autoPlay);

    bool networkOnline() const;
    void setNetworkOnline(bool networkOnline);

    bool framePresented() const;
    QString activeDecoder() const;

public slots:
    void play(const QSize &targetSize);
    void stop();

signals:
    void sourceChanged();
    void backendChanged();
    void decoderChanged();
    void activeDecoderChanged(const QString &activeDecoder);
    void autoPlayChanged();
    void networkOnlineChanged();
    void framePresentedChanged();
    void frameReady(const QImage &image);

private slots:
    void restartPlayback();
    void handleBackendFrame(const QImage &image);
    void handleBackendFinished(bool retryable);
    void handleBackendDecoderChanged(const QString &decoderName);

private:
    void ensureBackend();
    void resetFrameState();
    void setFramePresented(bool framePresented);

    QUrl m_source;
    QString m_backend = QStringLiteral("auto");
    QString m_decoder = QStringLiteral("auto");
    QString m_activeDecoder;
    bool m_autoPlay = true;
    bool m_networkOnline = true;
    bool m_framePresented = false;
    QSize m_targetSize = QSize(640, 360);
    QTimer m_restartTimer;
    AbstractVideoBackend *m_videoBackend = nullptr;
    bool m_stopRequested = true;
};

#endif
