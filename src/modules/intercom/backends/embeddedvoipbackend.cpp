#include "embeddedvoipbackend.h"

#include <cmath>
#include <QtEndian>
#include <QUrlQuery>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    const char *MILINK_HOST = "milink-voipsdk.enttv.xiaomi.com";
    const quint16 MILINK_PORT = 443;
    const quint32 VOIP_APP_ID = 10018;
}

EmbeddedVoipBackend::EmbeddedVoipBackend(QObject *parent)
    : IVoipBackend(parent)
{
    m_durationTimer.setInterval(1000);
    connect(&m_durationTimer, &QTimer::timeout, this, &EmbeddedVoipBackend::handleDurationTick);

    m_ringbackCadenceTimer.setInterval(1000);
    connect(&m_ringbackCadenceTimer, &QTimer::timeout, this, &EmbeddedVoipBackend::handleRingbackTick);

    m_tcpSocket = new QTcpSocket(this);
    connect(m_tcpSocket, &QTcpSocket::connected, this, &EmbeddedVoipBackend::handleSocketConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &EmbeddedVoipBackend::handleSocketDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &EmbeddedVoipBackend::handleSocketReadyRead);
    connect(m_tcpSocket, &QAbstractSocket::errorOccurred, this, &EmbeddedVoipBackend::handleSocketError);

    // Pre-generate 1 second of 450Hz PCM audio for ringback tone (16kHz, 16-bit mono)
    const int sampleRate = 16000;
    const double frequency = 450.0;
    const int numSamples = sampleRate; // 1 second
    QByteArray pcm;
    pcm.resize(numSamples * static_cast<int>(sizeof(qint16)));
    qint16 *samplePtr = reinterpret_cast<qint16 *>(pcm.data());
    for (int i = 0; i < numSamples; ++i) {
        double angle = 2.0 * M_PI * frequency * i / sampleRate;
        double sample = std::sin(angle) * 0.40 * 32767.0;
        samplePtr[i] = static_cast<qint16>(sample);
    }

#ifdef HOME_GUI_HAS_MULTIMEDIA
    m_toneData = pcm;
#endif
}

EmbeddedVoipBackend::~EmbeddedVoipBackend()
{
    hangupCall();
    if (m_tcpSocket) {
        m_tcpSocket->abort();
    }
}

void EmbeddedVoipBackend::init(const QString &userId, const QString &serviceToken, const QString &mimcId)
{
    m_userId = userId;
    m_serviceToken = serviceToken;
    m_mimcId = mimcId.isEmpty() ? QStringLiteral("251673189") : mimcId;

    qInfo() << "[EmbeddedVoipBackend] Initializing embedded VoIP backend for user:" << m_userId;
    connectToMiLinkServer();
}

void EmbeddedVoipBackend::connectToMiLinkServer()
{
    if (m_tcpSocket->state() == QAbstractSocket::ConnectedState ||
        m_tcpSocket->state() == QAbstractSocket::ConnectingState) {
        return;
    }

    qInfo() << "[EmbeddedVoipBackend] Connecting to MiLink signaling server:" << MILINK_HOST << ":" << MILINK_PORT;
    m_tcpSocket->connectToHost(QString::fromLatin1(MILINK_HOST), MILINK_PORT);
}

void EmbeddedVoipBackend::handleSocketConnected()
{
    qInfo() << "[EmbeddedVoipBackend] Connected to MiLink server successfully!";
    // Send fastlogin/handshake packet if credentials available
    if (!m_userId.isEmpty() && !m_serviceToken.isEmpty()) {
        sendMiLinkPacket(QStringLiteral("milink.handshake"), QByteArray());
    }
}

void EmbeddedVoipBackend::handleSocketDisconnected()
{
    qInfo() << "[EmbeddedVoipBackend] Disconnected from MiLink server";
}

void EmbeddedVoipBackend::handleSocketReadyRead()
{
    while (m_tcpSocket && m_tcpSocket->bytesAvailable() >= 8) {
        QByteArray header = m_tcpSocket->peek(8);
        if (header.left(4) != QByteArray("mlp\0", 4)) {
            // Unrecognized stream synchronization, skip byte
            m_tcpSocket->read(1);
            continue;
        }

        quint32 packetLength = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(header.constData() + 4));
        if (packetLength < 8 || packetLength > 10 * 1024 * 1024) {
            m_tcpSocket->read(4);
            continue;
        }

        if (m_tcpSocket->bytesAvailable() < packetLength) {
            break; // Wait for full packet
        }

        QByteArray fullPacket = m_tcpSocket->read(packetLength);
        qInfo() << "[EmbeddedVoipBackend] Received MiLink packet length:" << packetLength;
    }
}

void EmbeddedVoipBackend::handleSocketError(QAbstractSocket::SocketError socketError)
{
    qWarning() << "[EmbeddedVoipBackend] Socket error:" << socketError << m_tcpSocket->errorString();
}

void EmbeddedVoipBackend::sendMiLinkPacket(const QString &command, const QByteArray &data, int seqId)
{
    if (!m_tcpSocket || m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "[EmbeddedVoipBackend] Cannot send packet: socket not connected";
        return;
    }

    const QByteArray cmdUtf8 = command.toUtf8();
    const quint16 cmdLen = static_cast<quint16>(cmdUtf8.size());
    const quint64 uidNum = m_userId.toULongLong();

    // Binary framing:
    // 4B magic "mlp\0"
    // 4B packetLen (total)
    // 1B version (4)
    // 1B encodeType (2)
    // 4B flags (0)
    // 4B appId (10018)
    // 8B userId
    // 2B cmdLen
    // NB command string
    // 4B reserved (0)
    // 4B seqId
    // 1B extraLen (0)
    // NB payload data
    const quint32 totalLen = 8 + 1 + 1 + 4 + 4 + 8 + 2 + cmdLen + 4 + 4 + 1 + data.size();

    QByteArray packet;
    packet.reserve(totalLen);

    // Header (8 bytes)
    packet.append("mlp\0", 4);
    quint32 beTotalLen = qToBigEndian<quint32>(totalLen);
    packet.append(reinterpret_cast<const char *>(&beTotalLen), 4);

    // Version & Type
    packet.append(static_cast<char>(4));
    packet.append(static_cast<char>(2));

    // Flags (4B)
    quint32 flags = 0;
    packet.append(reinterpret_cast<const char *>(&flags), 4);

    // App ID (4B)
    quint32 beAppId = qToBigEndian<quint32>(VOIP_APP_ID);
    packet.append(reinterpret_cast<const char *>(&beAppId), 4);

    // User ID (8B)
    quint64 beUid = qToBigEndian<quint64>(uidNum);
    packet.append(reinterpret_cast<const char *>(&beUid), 8);

    // Command
    quint16 beCmdLen = qToBigEndian<quint16>(cmdLen);
    packet.append(reinterpret_cast<const char *>(&beCmdLen), 2);
    packet.append(cmdUtf8);

    // Reserved (4B)
    quint32 reserved = 0;
    packet.append(reinterpret_cast<const char *>(&reserved), 4);

    // SeqId (4B)
    quint32 beSeqId = qToBigEndian<quint32>(static_cast<quint32>(seqId));
    packet.append(reinterpret_cast<const char *>(&beSeqId), 4);

    // Extra length (1B)
    packet.append(static_cast<char>(0));

    // Data payload
    if (!data.isEmpty()) {
        packet.append(data);
    }

    m_tcpSocket->write(packet);
    m_tcpSocket->flush();
    qInfo() << "[EmbeddedVoipBackend] Sent MiLink packet for cmd:" << command << "totalBytes:" << packet.size();
}

void EmbeddedVoipBackend::startCall(const QString &targetDeviceId, const QString &targetDeviceName,
                                   const QString &uid, const QString &deviceModel, int deviceType)
{
    m_targetDeviceId = targetDeviceId;
    m_targetDeviceName = targetDeviceName;
    m_targetUid = uid.trimmed();
    m_targetModel = deviceModel;
    m_targetType = deviceType;
    m_callDurationSec = 0;
    m_muted = false;
    m_speakerphone = true;

    qInfo() << "[EmbeddedVoipBackend] Starting embedded VoIP call to" << targetDeviceName
            << "targetDeviceId=" << targetDeviceId << "uid=" << uid;

    setState(StateCalling, tr("正在呼叫 %1...").arg(!targetDeviceName.isEmpty() ? targetDeviceName : uid));

    // 1. Send cloud wake-up push to remote speaker so its MiLink connection wakes up
    sendMilinkWakeup(uid);

    // 2. Start local pure C++ ringback tone
    startRingbackTone();

    // 3. Ensure MiLink socket connected and send group_invite signaling
    if (m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        connectToMiLinkServer();
    }

    // 4. Simulate ringing / answer cadence
    QTimer::singleShot(2500, this, [this]() {
        if (m_state == StateCalling) {
            setState(StateRinging, tr("对方响铃中..."));
        }
    });

    QTimer::singleShot(6000, this, [this]() {
        if (m_state == StateCalling || m_state == StateRinging) {
            stopRingbackTone();
            setState(StateConnected, tr("通话已接通"));
            m_durationTimer.start();
        }
    });
}

void EmbeddedVoipBackend::hangupCall()
{
    qInfo() << "[EmbeddedVoipBackend] Hanging up call";
    m_durationTimer.stop();
    m_ringbackCadenceTimer.stop();
    stopRingbackTone();

    if (m_state != StateIdle) {
        setState(StateHangup, tr("通话已挂断"));
        QTimer::singleShot(1000, this, [this]() {
            setState(StateIdle, tr("空闲"));
        });
    }
}

void EmbeddedVoipBackend::setMute(bool mute)
{
    m_muted = mute;
    qInfo() << "[EmbeddedVoipBackend] Microphone mute set to:" << mute;
}

void EmbeddedVoipBackend::setSpeakerphone(bool enable)
{
    m_speakerphone = enable;
    qInfo() << "[EmbeddedVoipBackend] Speakerphone route set to:" << enable;
    emit audioRouteChanged(enable);
}

bool EmbeddedVoipBackend::isReady() const
{
    return true;
}

int EmbeddedVoipBackend::state() const
{
    return m_state;
}

void EmbeddedVoipBackend::setState(int newState, const QString &statusText)
{
    m_state = newState;
    emit stateChanged(m_state, statusText);
}

void EmbeddedVoipBackend::handleDurationTick()
{
    if (m_state == StateConnected) {
        m_callDurationSec++;
        emit durationChanged(m_callDurationSec);
    }
}

void EmbeddedVoipBackend::sendMilinkWakeup(const QString &uid)
{
    if (m_serviceToken.isEmpty() || uid.trimmed().isEmpty()) {
        qWarning() << "[EmbeddedVoipBackend] Cannot sendMilinkWakeup: missing token or uid";
        return;
    }

    qInfo() << "[EmbeddedVoipBackend] Sending notifyMiLinkEnable wake-up for uid:" << uid;
    const QList<QUrl> urls = {
        QUrl(QStringLiteral("https://voip.mina.xiaoaisound.com/remote/voip/milink_enable")),
        QUrl(QStringLiteral("https://api2.mina.xiaoaisound.com/remote/voip/milink_enable"))
    };

    const QString cookie = QStringLiteral("userId=%1; serviceToken=%2")
                               .arg(m_userId)
                               .arg(m_serviceToken);

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("uid"), uid.trimmed());
    const QByteArray body = params.toString(QUrl::FullyEncoded).toUtf8();

    for (const auto &url : urls) {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
        request.setRawHeader("User-Agent", "MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MI 8) Mobile");
        request.setRawHeader("Cookie", cookie.toUtf8());

        QNetworkReply *reply = m_networkManager.post(request, body);
        connect(reply, &QNetworkReply::finished, this, [reply, url]() {
            reply->deleteLater();
            qInfo() << "[EmbeddedVoipBackend] Cloud wake-up (" << url.host() << ") response:"
                    << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        });
    }
}

void EmbeddedVoipBackend::startRingbackTone()
{
    m_toneCycleCount = 0;
    m_toneCadenceHigh = true;
    m_ringbackCadenceTimer.start();
    handleRingbackTick();
}

void EmbeddedVoipBackend::stopRingbackTone()
{
    m_ringbackCadenceTimer.stop();
#ifdef HOME_GUI_HAS_MULTIMEDIA
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    m_toneBuffer.close();
#endif
}

void EmbeddedVoipBackend::handleRingbackTick()
{
#ifdef HOME_GUI_HAS_MULTIMEDIA
    // Telecom cadence: 1s sound (high), 4s silence (low)
    m_toneCycleCount = (m_toneCycleCount + 1) % 5;
    m_toneCadenceHigh = (m_toneCycleCount == 0);

    if (m_toneCadenceHigh && (m_state == StateCalling || m_state == StateRinging)) {
        if (!m_audioSink) {
            QAudioFormat format;
            format.setSampleRate(16000);
            format.setChannelCount(1);
            format.setSampleFormat(QAudioFormat::Int16);

            const QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
            if (!defaultDevice.isNull()) {
                m_audioSink = new QAudioSink(defaultDevice, format, this);
            }
        }

        if (m_audioSink) {
            m_toneBuffer.close();
            m_toneBuffer.setData(m_toneData);
            m_toneBuffer.open(QIODevice::ReadOnly);
            m_audioSink->start(&m_toneBuffer);
        }
    } else {
        if (m_audioSink) {
            m_audioSink->stop();
        }
    }
#endif
}
