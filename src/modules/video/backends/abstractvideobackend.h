#ifndef ABSTRACTVIDEOBACKEND_H
#define ABSTRACTVIDEOBACKEND_H

#include <QObject>
#include <QImage>
#include <QSize>
#include <QUrl>

#include "core/appconfig.h"

class AbstractVideoBackend : public QObject
{
    Q_OBJECT

public:
    explicit AbstractVideoBackend(QObject *parent = nullptr) : QObject(parent) {}
    ~AbstractVideoBackend() override = default;

    virtual QString backendName() const = 0;

public slots:
    virtual void start(const QUrl &source, const VideoConfig &config, const QSize &targetSize) = 0;
    virtual void stop() = 0;
    virtual void setNetworkOnline(bool online) = 0;

signals:
    void frameReady(const QImage &image);
    void statusTextChanged(const QString &statusText);
    void errorTextChanged(const QString &errorText);
    void framePresentedChanged(bool framePresented);
    void finished(bool retryable);
    void decoderChanged(const QString &decoderName);
};

#endif
