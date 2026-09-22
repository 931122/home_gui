#ifndef LIBAVVIDEOBACKEND_H
#define LIBAVVIDEOBACKEND_H

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QUrl>

#include "modules/video/backends/abstractvideobackend.h"

class LibavDecodeThread : public QThread
{
    Q_OBJECT

public:
    explicit LibavDecodeThread(QObject *parent = nullptr);

    void configure(const QUrl &source, const VideoConfig &config, const QSize &targetSize, bool networkOnline);
    void requestStop();
    void setNetworkOnline(bool online);
    bool stopRequested();

signals:
    void frameReady(const QImage &image);
    void statusTextChanged(const QString &statusText);
    void errorTextChanged(const QString &errorText);
    void framePresentedChanged(bool framePresented);
    void finished(bool retryable);
    void decoderChanged(const QString &decoderName);

protected:
    void run() override;

private:
    bool openStream(bool &retryable);

    QMutex m_mutex;
    QUrl m_source;
    VideoConfig m_config;
    QSize m_targetSize;
    bool m_stopRequested = false;
    bool m_networkOnline = true;
};

class LibavVideoBackend : public AbstractVideoBackend
{
    Q_OBJECT

public:
    explicit LibavVideoBackend(QObject *parent = nullptr);
    ~LibavVideoBackend() override;

    QString backendName() const override;

public slots:
    void start(const QUrl &source, const VideoConfig &config, const QSize &targetSize) override;
    void stop() override;
    void setNetworkOnline(bool online) override;

private:
    LibavDecodeThread m_decodeThread;
    bool m_networkOnline = true;
};

#endif
