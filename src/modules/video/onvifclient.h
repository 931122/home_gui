#ifndef ONVIFCLIENT_H
#define ONVIFCLIENT_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

#include "core/appconfig.h"

struct OnvifStreamInfo
{
    QString streamUri;
    QString profileToken;
    QString ptzServiceUrl;
    QString currentProfile;
    bool profileSwitchSupported = false;
};

class OnvifClient : public QObject
{
    Q_OBJECT

public:
    explicit OnvifClient(QObject *parent = nullptr);

    static bool isOnvifUrl(const QUrl &url);
    static QString deviceServiceUrl(const VideoConfig &config);

    bool resolveStream(const VideoConfig &config, OnvifStreamInfo &streamInfo);
    bool sendPtzCommand(const VideoConfig &config,
                        const QString &profileToken,
                        const QString &ptzServiceUrl,
                        const QString &command,
                        QString &errorMessage);
    void abortCurrentReply();

private:
    bool sendSoapRequest(const QUrl &url,
                         const QString &action,
                         const QString &securityHeader,
                         const QString &body,
                         QByteArray &responseBody,
                         QString &errorMessage);

    class QNetworkAccessManager *m_networkManager = nullptr;
    QPointer<class QNetworkReply> m_currentReply;
};

#endif
