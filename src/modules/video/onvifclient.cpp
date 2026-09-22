#include "modules/video/onvifclient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QEventLoop>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QTimer>
#include <QXmlStreamReader>

namespace {

QString escapeXml(const QString &value)
{
    QString result = value;
    result.replace('&', QStringLiteral("&amp;"));
    result.replace('<', QStringLiteral("&lt;"));
    result.replace('>', QStringLiteral("&gt;"));
    result.replace('"', QStringLiteral("&quot;"));
    result.replace('\'', QStringLiteral("&apos;"));
    return result;
}

QString buildSecurityHeader(const QString &username, const QString &password)
{
    QByteArray nonceBytes;
    nonceBytes.resize(16);
    for (int index = 0; index < nonceBytes.size(); ++index) {
        nonceBytes[index] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }

    const QString created = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ssZ"));
    QByteArray digestInput = nonceBytes;
    digestInput.append(created.toUtf8());
    digestInput.append(password.toUtf8());

    return QStringLiteral(
                "<wsse:Security s:mustUnderstand=\"1\" "
                "xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" "
                "xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">"
                "<wsse:UsernameToken>"
                "<wsse:Username>%1</wsse:Username>"
                "<wsse:Password Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">%2</wsse:Password>"
                "<wsse:Nonce EncodingType=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary\">%3</wsse:Nonce>"
                "<wsu:Created>%4</wsu:Created>"
                "</wsse:UsernameToken>"
                "</wsse:Security>")
            .arg(escapeXml(username),
                 QString::fromUtf8(QCryptographicHash::hash(digestInput, QCryptographicHash::Sha1).toBase64()),
                 QString::fromUtf8(nonceBytes.toBase64()),
                 created);
}

QByteArray buildSoapEnvelope(const QString &securityHeader, const QString &body)
{
    const QString envelope = QStringLiteral(
                "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" "
                "xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" "
                "xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" "
                "xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" "
                "xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
                "<s:Header>%1</s:Header>"
                "<s:Body>%2</s:Body>"
                "</s:Envelope>")
            .arg(securityHeader, body);
    return envelope.toUtf8();
}

QString findElementText(const QByteArray &xml, const QString &localName)
{
    QXmlStreamReader reader(xml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name().toString() == localName) {
            return reader.readElementText();
        }
    }
    return QString();
}

QHash<QString, QString> parseCapabilities(const QByteArray &xml)
{
    QHash<QString, QString> result;
    QXmlStreamReader reader(xml);
    QString currentService;

    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement()) {
            continue;
        }

        const QString name = reader.name().toString();
        if (name == QStringLiteral("Media")
            || name == QStringLiteral("Media2")
            || name == QStringLiteral("PTZ")) {
            currentService = name;
        } else if (name == QStringLiteral("XAddr") && !currentService.isEmpty()) {
            result.insert(currentService, reader.readElementText());
            currentService.clear();
        }
    }

    return result;
}

struct OnvifProfileInfo
{
    QString token;
    QString name;
    int width = 0;
    int height = 0;
};

struct SelectedOnvifProfile
{
    QString token;
    QString uiProfile;
};

QList<OnvifProfileInfo> parseProfiles(const QByteArray &xml)
{
    QList<OnvifProfileInfo> profiles;
    QXmlStreamReader reader(xml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement() || reader.name().toString() != QStringLiteral("Profiles")) {
            continue;
        }

        OnvifProfileInfo profile;
        profile.token = reader.attributes().value(QStringLiteral("token")).toString().trimmed();
        int depth = 0;
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.isEndElement()) {
                if (reader.name().toString() == QStringLiteral("Profiles") && depth == 0) {
                    break;
                }
                if (depth > 0) {
                    --depth;
                }
                continue;
            }
            if (!reader.isStartElement()) {
                continue;
            }

            ++depth;
            const QString elementName = reader.name().toString();
            if (depth == 1 && elementName == QStringLiteral("Name")) {
                profile.name = reader.readElementText().trimmed();
                --depth;
            } else if (elementName == QStringLiteral("Width")) {
                profile.width = reader.readElementText().trimmed().toInt();
                --depth;
            } else if (elementName == QStringLiteral("Height")) {
                profile.height = reader.readElementText().trimmed().toInt();
                --depth;
            }
        }

        if (!profile.token.isEmpty()) {
            profiles.append(profile);
        }
    }
    return profiles;
}

bool profileNameLooksMinor(const QString &name)
{
    const QString lowerName = name.trimmed().toLower();
    return lowerName.contains(QStringLiteral("minor"))
            || lowerName.contains(QStringLiteral("sub"))
            || lowerName.contains(QStringLiteral("secondary"));
}

bool profileNameLooksMain(const QString &name)
{
    const QString lowerName = name.trimmed().toLower();
    return lowerName.contains(QStringLiteral("main"))
            || lowerName.contains(QStringLiteral("primary"));
}

qint64 profileArea(const OnvifProfileInfo &profile)
{
    return qint64(qMax(0, profile.width)) * qMax(0, profile.height);
}

SelectedOnvifProfile selectLargestProfile(const QList<OnvifProfileInfo> &profiles)
{
    int selectedIndex = 0;
    qint64 selectedArea = profileArea(profiles.first());
    for (int index = 1; index < profiles.size(); ++index) {
        const qint64 area = profileArea(profiles.at(index));
        if (area > selectedArea) {
            selectedArea = area;
            selectedIndex = index;
        }
    }
    return {profiles.at(selectedIndex).token, QStringLiteral("main")};
}

SelectedOnvifProfile selectSmallestProfile(const QList<OnvifProfileInfo> &profiles)
{
    int selectedIndex = profiles.size() - 1;
    qint64 selectedArea = profileArea(profiles.at(selectedIndex));
    for (int index = 0; index < profiles.size(); ++index) {
        const qint64 area = profileArea(profiles.at(index));
        if (area <= 0) {
            continue;
        }
        if (selectedArea <= 0 || area < selectedArea) {
            selectedArea = area;
            selectedIndex = index;
        }
    }
    return {profiles.at(selectedIndex).token, QStringLiteral("minor")};
}

SelectedOnvifProfile selectProfileToken(const QList<OnvifProfileInfo> &profiles, const QString &profilePreference)
{
    if (profiles.isEmpty()) {
        return {};
    }

    const QString preference = profilePreference.trimmed().toLower();
    if (preference.isEmpty() || preference == QStringLiteral("auto")) {
        // 对于嵌入式平台，默认使用辅码流（分辨率最小的）以节省带宽和 CPU。
        return selectSmallestProfile(profiles);
    }

    for (const OnvifProfileInfo &profile : profiles) {
        if (profile.token.trimmed().compare(profilePreference.trimmed(), Qt::CaseInsensitive) == 0
            || profile.name.trimmed().compare(profilePreference.trimmed(), Qt::CaseInsensitive) == 0) {
            return {profile.token, profileNameLooksMinor(profile.name) ? QStringLiteral("minor") : QStringLiteral("main")};
        }
    }

    const auto matchesName = [&](const QStringList &keywords) -> SelectedOnvifProfile {
        for (const OnvifProfileInfo &profile : profiles) {
            const QString name = profile.name.trimmed().toLower();
            for (const QString &keyword : keywords) {
                if (name.contains(keyword)) {
                    const QString uiProfile = (keyword == QStringLiteral("minor")
                                               || keyword == QStringLiteral("sub")
                                               || keyword == QStringLiteral("secondary"))
                            ? QStringLiteral("minor")
                            : QStringLiteral("main");
                    return {profile.token, uiProfile};
                }
            }
        }
        return {};
    };

    if (preference == QStringLiteral("main") || preference == QStringLiteral("mainstream")) {
        const SelectedOnvifProfile matched = matchesName({QStringLiteral("main"), QStringLiteral("primary")});
        return matched.token.isEmpty() ? selectLargestProfile(profiles) : matched;
    }

    if (preference == QStringLiteral("minor")
        || preference == QStringLiteral("sub")
        || preference == QStringLiteral("substream")
        || preference == QStringLiteral("secondary")) {
        const SelectedOnvifProfile matched = matchesName({QStringLiteral("minor"), QStringLiteral("sub"), QStringLiteral("secondary")});
        return matched.token.isEmpty() ? selectSmallestProfile(profiles) : matched;
    }

    return selectLargestProfile(profiles);
}

bool supportsProfileSwitch(const QList<OnvifProfileInfo> &profiles)
{
    bool hasMain = false;
    bool hasMinor = false;
    for (const OnvifProfileInfo &profile : profiles) {
        hasMinor = hasMinor || profileNameLooksMinor(profile.name);
        hasMain = hasMain || profileNameLooksMain(profile.name);
    }
    return (hasMain && hasMinor) || profiles.size() > 1;
}

QUrl cameraUrlFromConfig(const VideoConfig &config)
{
    return QUrl::fromUserInput(config.url.trimmed());
}

} // namespace

OnvifClient::OnvifClient(QObject *parent)
    : QObject(parent)
{
}

bool OnvifClient::isOnvifUrl(const QUrl &url)
{
    const QString scheme = url.scheme().trimmed().toLower();
    return scheme == QStringLiteral("onvif")
            || scheme == QStringLiteral("http")
            || scheme == QStringLiteral("https");
}

QString OnvifClient::deviceServiceUrl(const VideoConfig &config)
{
    const QUrl url = cameraUrlFromConfig(config);
    if (!url.isValid() || url.isEmpty()) {
        return QString();
    }

    if (url.scheme().compare(QStringLiteral("onvif"), Qt::CaseInsensitive) == 0) {
        if (url.host().trimmed().isEmpty()) {
            return QString();
        }

        QString path = url.path().trimmed();
        if (path.isEmpty() || path == QStringLiteral("/")) {
            path = QStringLiteral("/onvif/device_service");
        }

        QUrl deviceUrl;
        deviceUrl.setScheme(QStringLiteral("http"));
        deviceUrl.setHost(url.host());
        deviceUrl.setPort(url.port(80));
        deviceUrl.setPath(path);
        return deviceUrl.toString();
    }

    if (url.scheme().compare(QStringLiteral("http"), Qt::CaseInsensitive) == 0
        || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0) {
        return url.toString();
    }

    return QString();
}

bool OnvifClient::resolveStream(const VideoConfig &config, OnvifStreamInfo &streamInfo)
{
    const QString deviceUrl = deviceServiceUrl(config);
    if (deviceUrl.isEmpty()) {
        return false;
    }

    const QString securityHeader = buildSecurityHeader(config.username, config.password);
    QByteArray response;
    QString errorMessage;
    if (!sendSoapRequest(QUrl(deviceUrl),
                         QStringLiteral("http://www.onvif.org/ver10/device/wsdl/GetCapabilities"),
                         securityHeader,
                         QStringLiteral("<tds:GetCapabilities><tds:Category>All</tds:Category></tds:GetCapabilities>"),
                         response,
                         errorMessage)) {
        return false;
    }

    const QHash<QString, QString> capabilities = parseCapabilities(response);
    const QString mediaServiceUrl = capabilities.value(QStringLiteral("Media"), capabilities.value(QStringLiteral("Media2")));
    streamInfo.ptzServiceUrl = capabilities.value(QStringLiteral("PTZ"));
    if (mediaServiceUrl.isEmpty()) {
        return false;
    }

    if (!sendSoapRequest(QUrl(mediaServiceUrl),
                         QStringLiteral("http://www.onvif.org/ver10/media/wsdl/GetProfiles"),
                         securityHeader,
                         QStringLiteral("<trt:GetProfiles/>"),
                         response,
                         errorMessage)) {
        return false;
    }

    const QList<OnvifProfileInfo> profiles = parseProfiles(response);
    const SelectedOnvifProfile selectedProfile = selectProfileToken(profiles, config.onvifProfile);
    if (selectedProfile.token.isEmpty()) {
        return false;
    }

    streamInfo.profileSwitchSupported = supportsProfileSwitch(profiles);
    streamInfo.currentProfile = selectedProfile.uiProfile;
    streamInfo.profileToken = selectedProfile.token;

    const QString body = QStringLiteral(
                "<trt:GetStreamUri>"
                "<trt:StreamSetup>"
                "<tt:Stream>RTP-Unicast</tt:Stream>"
                "<tt:Transport><tt:Protocol>RTSP</tt:Protocol></tt:Transport>"
                "</trt:StreamSetup>"
                "<trt:ProfileToken>%1</trt:ProfileToken>"
                "</trt:GetStreamUri>")
            .arg(escapeXml(selectedProfile.token));

    if (!sendSoapRequest(QUrl(mediaServiceUrl),
                         QStringLiteral("http://www.onvif.org/ver10/media/wsdl/GetStreamUri"),
                         securityHeader,
                         body,
                         response,
                         errorMessage)) {
        return false;
    }

    streamInfo.streamUri = findElementText(response, QStringLiteral("Uri"));
    return !streamInfo.streamUri.isEmpty();
}

bool OnvifClient::sendPtzCommand(const VideoConfig &config,
                                 const QString &profileToken,
                                 const QString &ptzServiceUrl,
                                 const QString &command,
                                 QString &errorMessage)
{
    if (ptzServiceUrl.trimmed().isEmpty()) {
        errorMessage = QStringLiteral("PTZ service unavailable");
        return false;
    }
    if (profileToken.trimmed().isEmpty()) {
        errorMessage = QStringLiteral("PTZ profile missing");
        return false;
    }

    QString action;
    QString body;
    if (command == QStringLiteral("stop")) {
        action = QStringLiteral("http://www.onvif.org/ver20/ptz/wsdl/Stop");
        body = QStringLiteral(
                    "<tptz:Stop>"
                    "<tptz:ProfileToken>%1</tptz:ProfileToken>"
                    "<tptz:PanTilt>true</tptz:PanTilt>"
                    "<tptz:Zoom>true</tptz:Zoom>"
                    "</tptz:Stop>")
                .arg(escapeXml(profileToken));
    } else if (command == QStringLiteral("home")) {
        action = QStringLiteral("http://www.onvif.org/ver20/ptz/wsdl/GotoHomePosition");
        body = QStringLiteral(
                    "<tptz:GotoHomePosition>"
                    "<tptz:ProfileToken>%1</tptz:ProfileToken>"
                    "</tptz:GotoHomePosition>")
                .arg(escapeXml(profileToken));
    } else {
        double x = 0.0;
        double y = 0.0;
        if (command == QStringLiteral("up")) {
            y = 0.6;
        } else if (command == QStringLiteral("down")) {
            y = -0.6;
        } else if (command == QStringLiteral("left")) {
            x = -0.6;
        } else if (command == QStringLiteral("right")) {
            x = 0.6;
        } else {
            errorMessage = QStringLiteral("unknown PTZ command");
            return false;
        }

        action = QStringLiteral("http://www.onvif.org/ver20/ptz/wsdl/ContinuousMove");
        body = QStringLiteral(
                    "<tptz:ContinuousMove>"
                    "<tptz:ProfileToken>%1</tptz:ProfileToken>"
                    "<tptz:Velocity>"
                    "<tt:PanTilt x=\"%2\" y=\"%3\"/>"
                    "</tptz:Velocity>"
                    "<tptz:Timeout>PT5S</tptz:Timeout>"
                    "</tptz:ContinuousMove>")
                .arg(escapeXml(profileToken),
                     QString::number(x, 'f', 2),
                     QString::number(y, 'f', 2));
    }

    QByteArray responseBody;
    return sendSoapRequest(QUrl::fromUserInput(ptzServiceUrl),
                           action,
                           buildSecurityHeader(config.username, config.password),
                           body,
                           responseBody,
                           errorMessage);
}

bool OnvifClient::sendSoapRequest(const QUrl &url,
                                  const QString &action,
                                  const QString &securityHeader,
                                  const QString &body,
                                  QByteArray &responseBody,
                                  QString &errorMessage)
{
    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/soap+xml; charset=utf-8; action=\"%1\"").arg(action));

    QNetworkReply *reply = m_networkManager->post(request, buildSoapEnvelope(securityHeader, body));
    m_currentReply = reply;
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    bool timedOut = false;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
        timedOut = true;
        reply->abort();
        loop.quit();
    });
    timeoutTimer.start(3000);
    loop.exec();
    timeoutTimer.stop();

    if (m_currentReply == reply) {
        m_currentReply = nullptr;
    }
    if (timedOut) {
        errorMessage = QStringLiteral("ONVIF request timeout");
        reply->deleteLater();
        return false;
    }
    if (reply->error() != QNetworkReply::NoError) {
        errorMessage = reply->errorString();
        reply->deleteLater();
        return false;
    }

    responseBody = reply->readAll();
    reply->deleteLater();
    return true;
}

void OnvifClient::abortCurrentReply()
{
    if (m_currentReply) {
        m_currentReply->abort();
    }
}
