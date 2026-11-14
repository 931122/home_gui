#include "embeddedvoipbackend.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QUrlQuery>
#include <QUuid>

#if defined(Q_OS_ANDROID)
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qnativeinterface.h>
#elif defined(Q_OS_IOS)
#include "agoramediabridge_ios.h"
#endif

#if defined(Q_OS_ANDROID)

extern "C" {

JNIEXPORT void JNICALL
Java_org_qtproject_example_home_1gui_AgoraMediaBridge_nativeOnAgoraJoined(JNIEnv *env, jclass, jstring channel, jint uid)
{
    const char *chars = channel ? env->GetStringUTFChars(channel, nullptr) : "";
    const QString ch = QString::fromUtf8(chars);
    if (channel) {
        env->ReleaseStringUTFChars(channel, chars);
    }
    const quint32 u = static_cast<quint32>(uid);
    QMetaObject::invokeMethod(qApp, [ch, u]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraJoined(ch, u);
        }
    }, Qt::QueuedConnection);
}

JNIEXPORT void JNICALL
Java_org_qtproject_example_home_1gui_AgoraMediaBridge_nativeOnAgoraUserJoined(JNIEnv *, jclass, jint uid)
{
    const quint32 u = static_cast<quint32>(uid);
    QMetaObject::invokeMethod(qApp, [u]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraUserJoined(u);
        }
    }, Qt::QueuedConnection);
}

JNIEXPORT void JNICALL
Java_org_qtproject_example_home_1gui_AgoraMediaBridge_nativeOnAgoraUserOffline(JNIEnv *, jclass, jint uid, jint reason)
{
    const quint32 u = static_cast<quint32>(uid);
    const int r = static_cast<int>(reason);
    QMetaObject::invokeMethod(qApp, [u, r]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraUserOffline(u, r);
        }
    }, Qt::QueuedConnection);
}

JNIEXPORT void JNICALL
Java_org_qtproject_example_home_1gui_AgoraMediaBridge_nativeOnAgoraError(JNIEnv *, jclass, jint errCode)
{
    const int err = static_cast<int>(errCode);
    QMetaObject::invokeMethod(qApp, [err]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraError(err);
        }
    }, Qt::QueuedConnection);
}

JNIEXPORT void JNICALL
Java_org_qtproject_example_home_1gui_AgoraMediaBridge_nativeOnAgoraAudioRoute(JNIEnv *, jclass, jboolean isSpeaker)
{
    const bool speaker = (isSpeaker != JNI_FALSE);
    QMetaObject::invokeMethod(qApp, [speaker]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraAudioRoute(speaker);
        }
    }, Qt::QueuedConnection);
}

} // extern "C"
#endif

namespace {

constexpr const char *kMiLinkHosts[] = {
    "123.125.102.91",
    "124.251.58.56",
    "111.13.141.220",
    "220.181.52.243"
};
constexpr int kMiLinkHostCount = sizeof(kMiLinkHosts) / sizeof(kMiLinkHosts[0]);
constexpr quint16 kMiLinkPort = 443;

QString micoUserAgent()
{
    return QStringLiteral("MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MIX 2S) Mobile");
}

} // namespace

EmbeddedVoipBackend *EmbeddedVoipBackend::s_instance = nullptr;

EmbeddedVoipBackend::EmbeddedVoipBackend(QObject *parent)
    : IVoipBackend(parent)
    , m_tcpSocket(new QTcpSocket(this))
{
    s_instance = this;

    connect(m_tcpSocket, &QTcpSocket::connected, this, &EmbeddedVoipBackend::handleSocketConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &EmbeddedVoipBackend::handleSocketDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &EmbeddedVoipBackend::handleSocketReadyRead);
    connect(m_tcpSocket, &QAbstractSocket::errorOccurred, this, &EmbeddedVoipBackend::handleSocketError);

    m_durationTimer.setInterval(1000);
    connect(&m_durationTimer, &QTimer::timeout, this, &EmbeddedVoipBackend::handleDurationTick);

    m_groupHeartbeatTimer.setInterval(10000);
    connect(&m_groupHeartbeatTimer, &QTimer::timeout, this, &EmbeddedVoipBackend::handleGroupHeartbeatTick);

    m_milinkKeepaliveTimer.setInterval(30000);
    connect(&m_milinkKeepaliveTimer, &QTimer::timeout, this, &EmbeddedVoipBackend::handleMiLinkKeepaliveTick);

    m_authTimeoutTimer.setSingleShot(true);
    m_authTimeoutTimer.setInterval(12000);
    connect(&m_authTimeoutTimer, &QTimer::timeout, this, &EmbeddedVoipBackend::handleAuthTimeout);

    m_resetIdleTimer.setSingleShot(true);
    m_resetIdleTimer.setInterval(1200);
    connect(&m_resetIdleTimer, &QTimer::timeout, this, [this]() {
        if (m_state == StateHangup || m_state == StateError) {
            setState(StateIdle, QString());
        }
    });
}

EmbeddedVoipBackend::~EmbeddedVoipBackend()
{
    m_durationTimer.stop();
    m_groupHeartbeatTimer.stop();
    m_milinkKeepaliveTimer.stop();
    m_authTimeoutTimer.stop();
    m_resetIdleTimer.stop();
    leaveAgoraMediaChannel();
    if (m_tcpSocket) {
        m_tcpSocket->disconnect(this);
        m_tcpSocket->abort();
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

EmbeddedVoipBackend *EmbeddedVoipBackend::instance()
{
    return s_instance;
}

void EmbeddedVoipBackend::init(const QString &userId, const QString &serviceToken, const QString &mimcId)
{
    const QString newUid = userId.trimmed();
    const QString newTok = serviceToken.trimmed();
    const QString newMimc = mimcId.trimmed();

    if (newTok != m_serviceToken || newUid != m_userId) {
        if (m_tcpSocket && m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            m_tcpSocket->disconnectFromHost();
        }
        m_authState = AuthDisconnected;
        m_channelWid = 0;
        m_callerVuid = 0;
        m_voipSecurityKey.clear();
        m_realNameGtkeyB2.clear();
        m_realNameB2.clear();
        m_suid = 0;
    }

    m_userId = newUid;
    m_serviceToken = newTok;
    m_mimcId = newMimc;

    qInfo() << "[EmbeddedVoipBackend] init userId=" << m_userId
            << "tokenLen=" << m_serviceToken.size()
            << "mimcId=" << m_mimcId;

#if defined(Q_OS_ANDROID)
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        QJniObject::callStaticMethod<void>(
            "org/qtproject/example/home_gui/AgoraMediaBridge",
            "init",
            "(Landroid/content/Context;)V",
            activity.object()
        );
    }
#elif defined(Q_OS_IOS)
    AgoraMediaBridgeIos::init();
#endif
}

void EmbeddedVoipBackend::setCallerVuid(quint64 vuid)
{
    m_callerVuid = vuid;
}

void EmbeddedVoipBackend::setMinaServiceToken(const QString &minaServiceToken)
{
    m_minaServiceToken = minaServiceToken.trimmed();
}

void EmbeddedVoipBackend::setTarget(const QString &targetDeviceId, const QString &targetVuid,
                                    const QString &targetDid, const QString &targetModel)
{
    m_targetDeviceId = targetDeviceId.trimmed();
    m_targetVuidStr = targetVuid.trimmed();
    m_targetVuid = m_targetVuidStr.toULongLong();
    m_targetDidStr = targetDid.trimmed();
    m_targetDid = m_targetDidStr.toULongLong();
    m_targetModel = targetModel.trimmed();
}

void EmbeddedVoipBackend::startCall(const QString &targetDeviceId, const QString &targetDeviceName,
                                    const QString &uid, const QString &deviceModel, int deviceType,
                                    const QString &did)
{
    beginCallFlow(1, targetDeviceId, targetDeviceName, uid, deviceModel, deviceType, did);
}

void EmbeddedVoipBackend::startDirectIntercom(const QString &targetDeviceId, const QString &targetDeviceName,
                                              const QString &uid, const QString &deviceModel, int deviceType,
                                              const QString &did)
{
    beginCallFlow(4, targetDeviceId, targetDeviceName, uid, deviceModel, deviceType, did);
}

void EmbeddedVoipBackend::beginCallFlow(int groupInviteType,
                                        const QString &targetDeviceId,
                                        const QString &targetDeviceName,
                                        const QString &uid,
                                        const QString &deviceModel,
                                        int deviceType,
                                        const QString &did)
{
    m_resetIdleTimer.stop();

    if (m_roomId > 0 || m_agoraJoinRequested || m_agoraJoined) {
        leaveAgoraMediaChannel();
        if (m_roomId > 0 && m_authState == AuthAuthenticated) {
            sendGroupCancel(m_roomId, m_targetVuid, m_pendingInviteType, m_sessionId);
        }
    }

    m_groupHeartbeatTimer.stop();
    m_durationTimer.stop();
    m_callDurationSec = 0;

    m_targetDeviceId = targetDeviceId.trimmed();
    m_targetDeviceName = targetDeviceName.trimmed();
    m_targetVuidStr = uid.trimmed();
    m_targetVuid = m_targetVuidStr.toULongLong();
    m_targetDidStr = did.trimmed();
    m_targetDid = m_targetDidStr.toULongLong();
    m_targetModel = deviceModel.trimmed().isEmpty() ? QStringLiteral("OH2P") : deviceModel.trimmed();
    m_targetDeviceType = deviceType;

    m_roomId = 0;
    m_agoraAppId.clear();
    m_agoraToken.clear();
    m_agoraJoinRequested = false;
    m_agoraJoined = false;
    m_remoteAccepted = false;
    m_remoteJoined = false;
    m_directIntercom = (groupInviteType == 4);
    m_pendingInviteType = 1; // MiLink server rejects client-originated groupInviteType=4 (err=1133); direct intercom uses type=1 + UBUS auto-answer
    ++m_callGeneration;
    m_hasPendingInvite = true;
    m_muted = false;
    m_speakerphone = true;

    const QString modeLabel = m_directIntercom ? QStringLiteral("DirectIntercom") : QStringLiteral("VoipCall");
    qInfo() << "[EmbeddedVoipBackend]" << modeLabel
            << "target=" << m_targetDeviceName << "deviceId=" << m_targetDeviceId
            << "vuid=" << m_targetVuid << "did=" << m_targetDid << "model=" << m_targetModel;

    if (!isReady() || m_targetVuid == 0) {
        const QString err = m_targetVuid == 0
            ? tr("目标音箱缺少 VUID，无法发起呼叫")
            : tr("未登录或缺少 micoapiServiceToken");
        qWarning() << "[EmbeddedVoipBackend] Cannot start call:" << err;
        m_hasPendingInvite = false;
        emit errorOccurred(err);
        finishCallWithState(StateError, err);
        return;
    }

    setState(StateCalling, m_directIntercom ? tr("正在连接免接听对讲...") : tr("正在呼叫..."));

    // Wake up remote speaker MiLink service via cloud HTTP API
    sendMilinkWakeupHttp(m_targetVuidStr);

    if (m_authState == AuthAuthenticated &&
        m_tcpSocket &&
        m_tcpSocket->state() == QAbstractSocket::ConnectedState &&
        !m_realNameB2.isEmpty() &&
        m_realNameGtkeyB2.size() == 16) {
        sendPendingGroupInvite();
    } else {
        m_reconnectAttempts = 0;
        connectToMiLinkServer(false);
    }
}

void EmbeddedVoipBackend::hangupCall()
{
    qInfo() << "[EmbeddedVoipBackend] hangupCall roomId=" << m_roomId << "state=" << m_state;

    ++m_callGeneration;
    m_hasPendingInvite = false;
    m_authTimeoutTimer.stop();
    m_groupHeartbeatTimer.stop();
    m_durationTimer.stop();

    const quint64 activeRoomId = m_roomId;
    const quint64 activeTargetVuid = m_targetVuid;
    const qint32 activeInviteType = m_pendingInviteType;
    const QString activeSessionId = m_sessionId;
    const QString activeDeviceId = m_targetDeviceId;
    const bool wasDirectOrConnected = m_directIntercom || (m_state == StateConnected);

    leaveAgoraMediaChannel();

    m_roomId = 0;
    m_remoteAccepted = false;
    m_remoteJoined = false;
    m_directIntercom = false;

    if (m_authState == AuthAuthenticated && activeRoomId > 0) {
        sendGroupCancel(activeRoomId, activeTargetVuid, activeInviteType, activeSessionId);
    }
    if (!activeDeviceId.isEmpty() && (!m_minaServiceToken.isEmpty() || !m_serviceToken.isEmpty()) &&
        (activeRoomId == 0 || wasDirectOrConnected)) {
        sendUbusHangupFallback(activeDeviceId);
    }

    finishCallWithState(StateHangup, tr("通话已结束"));
}

void EmbeddedVoipBackend::setMute(bool mute)
{
    m_muted = mute;
    qInfo() << "[EmbeddedVoipBackend] setMute:" << mute;
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/home_gui/AgoraMediaBridge",
        "setMute",
        "(Z)V",
        static_cast<jboolean>(mute)
    );
#elif defined(Q_OS_IOS)
    AgoraMediaBridgeIos::setMute(mute);
#endif
}

void EmbeddedVoipBackend::setSpeakerphone(bool enable)
{
    m_speakerphone = enable;
    qInfo() << "[EmbeddedVoipBackend] setSpeakerphone:" << enable;
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/home_gui/AgoraMediaBridge",
        "setSpeakerphone",
        "(Z)V",
        static_cast<jboolean>(enable)
    );
#elif defined(Q_OS_IOS)
    AgoraMediaBridgeIos::setSpeakerphone(enable);
#endif
    emit audioRouteChanged(enable);
}

bool EmbeddedVoipBackend::isReady() const
{
    return !m_userId.isEmpty() && !m_serviceToken.isEmpty();
}

int EmbeddedVoipBackend::state() const
{
    return m_state;
}

void EmbeddedVoipBackend::connectToMiLinkServer(bool advanceHost)
{
    resetAuthSession(true);
    if (advanceHost) {
        m_hostIndex = (m_hostIndex + 1) % kMiLinkHostCount;
    }

    const QString host = QString::fromLatin1(kMiLinkHosts[m_hostIndex]);
    qInfo() << "[EmbeddedVoipBackend] Connecting MiLink v4 TCP ->" << host << ":" << kMiLinkPort;

    m_authState = AuthTcpConnecting;
    m_authTimeoutTimer.start();
    m_tcpSocket->connectToHost(host, kMiLinkPort);
}

void EmbeddedVoipBackend::resetAuthSession(bool closeSocket)
{
    m_milinkKeepaliveTimer.stop();
    m_authTimeoutTimer.stop();
    m_rxBuffer.clear();
    m_privacyKey.clear();
    m_channelWid = 0;
    m_channelB2.clear();
    m_channelGtkeyB2.clear();
    m_voipServiceToken.clear();
    m_voipSecurityKey.clear();
    m_realNameB2.clear();
    m_realNameGtkeyB2.clear();
    m_suid.clear();
    m_authState = AuthDisconnected;

    if (closeSocket && m_tcpSocket) {
        if (m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            m_tcpSocket->abort();
        }
    }
}

void EmbeddedVoipBackend::handleSocketConnected()
{
    qInfo() << "[EmbeddedVoipBackend] TCP connected, starting milink.handshake";
    sendHandshake();
}

void EmbeddedVoipBackend::handleSocketDisconnected()
{
    qInfo() << "[EmbeddedVoipBackend] MiLink TCP disconnected (authState=" << m_authState
            << ", state=" << m_state << ", pendingInvite=" << m_hasPendingInvite << ")";
    const AuthState prevAuth = m_authState;
    resetAuthSession(false);

    if (m_hasPendingInvite && prevAuth != AuthDisconnected && m_reconnectAttempts < kMiLinkHostCount) {
        ++m_reconnectAttempts;
        qWarning() << "[EmbeddedVoipBackend] Reconnecting MiLink attempt" << m_reconnectAttempts;
        connectToMiLinkServer(true);
        return;
    }

    if (m_hasPendingInvite || m_state == StateCalling) {
        m_hasPendingInvite = false;
        const QString err = tr("MiLink 信令连接断开");
        emit errorOccurred(err);
        finishCallWithState(StateError, err);
    }
}

void EmbeddedVoipBackend::handleSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    const QString errStr = m_tcpSocket ? m_tcpSocket->errorString() : QStringLiteral("Unknown socket error");
    qWarning() << "[EmbeddedVoipBackend] Socket error:" << errStr << "authState=" << m_authState;

    if ((m_hasPendingInvite || m_state == StateCalling) && m_reconnectAttempts < kMiLinkHostCount - 1) {
        ++m_reconnectAttempts;
        connectToMiLinkServer(true);
        return;
    }

    if (m_hasPendingInvite || m_state == StateCalling || m_state == StateRinging) {
        m_hasPendingInvite = false;
        leaveAgoraMediaChannel();
        m_groupHeartbeatTimer.stop();
        m_durationTimer.stop();
        emit errorOccurred(errStr);
        finishCallWithState(StateError, tr("网络连接失败: %1").arg(errStr));
    }
}

void EmbeddedVoipBackend::handleAuthTimeout()
{
    qWarning() << "[EmbeddedVoipBackend] MiLink auth/signaling step timed out at authState=" << m_authState;
    if ((m_hasPendingInvite || m_state == StateCalling) && m_reconnectAttempts < kMiLinkHostCount - 1) {
        ++m_reconnectAttempts;
        connectToMiLinkServer(true);
        return;
    }
    if (m_hasPendingInvite || m_state == StateCalling) {
        m_hasPendingInvite = false;
        resetAuthSession(true);
        const QString err = tr("MiLink 信令握手超时");
        emit errorOccurred(err);
        finishCallWithState(StateError, err);
    }
}

void EmbeddedVoipBackend::sendHandshake()
{
    m_authState = AuthHandshaking;
    m_authTimeoutTimer.start();

    const QByteArray body = MilinkProto::Builder::buildHandshakeReq();
    sendMiLinkPacket(QStringLiteral("milink.handshake"),
                     MilinkProto::EncodeGcmHandshake,
                     body,
                     0,
                     MilinkProto::handshakeTokenBytes(),
                     MilinkProto::handshakeKeyBytes(),
                     QByteArray(),
                     QString(),
                     true,
                     &m_seqHandshake);
}

void EmbeddedVoipBackend::sendChannel()
{
    m_authState = AuthChanneling;
    m_authTimeoutTimer.start();

    m_privacyKey = MilinkProto::randomAsciiKey(16);
    m_rsaKeyIndex = QRandomGenerator::global()->bounded(4);

    const QByteArray body = MilinkProto::Builder::buildChannelReq(m_privacyKey);
    sendMiLinkPacket(QStringLiteral("milink.channel"),
                     MilinkProto::EncodeRsaChannel,
                     body,
                     0,
                     QByteArray::number(m_rsaKeyIndex),
                     QByteArray(),
                     MilinkProto::rsaPublicKeyB64(m_rsaKeyIndex),
                     QString(),
                     true,
                     &m_seqChannel);
}

void EmbeddedVoipBackend::sendDeviceLogin()
{
    m_authState = AuthDeviceLoggingIn;
    m_authTimeoutTimer.start();

    const quint64 mid = m_userId.toULongLong();
    const QByteArray body = MilinkProto::Builder::buildDeviceLoginReq(mid, m_serviceToken);
    sendMiLinkPacket(QStringLiteral("voipsdk.account.devicelogin"),
                     MilinkProto::EncodeGcmDeviceLogin,
                     body,
                     m_channelWid,
                     m_channelB2,
                     m_channelGtkeyB2,
                     QByteArray(),
                     QString::number(m_rsaKeyIndex),
                     false,
                     &m_seqDeviceLogin);
}

void EmbeddedVoipBackend::sendFastLogin()
{
    m_authState = AuthFastLoggingIn;
    m_authTimeoutTimer.start();

    const QByteArray body = MilinkProto::Builder::buildFastLoginReq(QString(), true, true);
    sendMiLinkPacket(QStringLiteral("milink.fastlogin"),
                     MilinkProto::EncodeCbcFastLogin,
                     body,
                     m_callerVuid,
                     m_voipServiceToken.toUtf8(),
                     m_voipSecurityKey,
                     QByteArray(),
                     m_voipServiceToken,
                     true,
                     &m_seqFastLogin);
}

void EmbeddedVoipBackend::sendPendingGroupInvite()
{
    if (!m_hasPendingInvite || m_authState != AuthAuthenticated || m_callerVuid == 0 || m_targetVuid == 0) {
        return;
    }
    m_hasPendingInvite = false;

    const quint64 callerMid = m_userId.toULongLong();
    const QString sigSeq = generateSigSeq(m_targetVuid);
    m_sessionId = generateSessionId(m_callerVuid);

    qInfo() << "[EmbeddedVoipBackend] Sending voipsdk.signal.group_invite type=" << m_pendingInviteType
            << "callerVuid=" << m_callerVuid << "targetVuid=" << m_targetVuid
            << "sessionId=" << m_sessionId;

    const QByteArray req = MilinkProto::Builder::buildGroupInviteReq(
        callerMid,
        m_callerVuid,
        m_targetVuid,
        m_targetDid,
        m_targetModel,
        m_pendingInviteType,
        sigSeq,
        m_sessionId
    );

    sendMiLinkPacket(QStringLiteral("voipsdk.signal.group_invite"),
                     MilinkProto::EncodeGcmMns,
                     req,
                     m_callerVuid,
                     m_realNameB2,
                     m_realNameGtkeyB2,
                     QByteArray(),
                     m_voipServiceToken,
                     false,
                     &m_seqGroupInvite);
}

void EmbeddedVoipBackend::sendGetAgoraToken(qint32 type, quint64 roomId)
{
    const QByteArray req = MilinkProto::Builder::buildGetAgoraTokenReq(m_callerVuid, type, roomId);
    quint32 *seqOut = (type == 1) ? &m_seqAgoraAppId : &m_seqAgoraToken;
    sendMiLinkPacket(QStringLiteral("voipsdk.signal.getagoratoken"),
                     MilinkProto::EncodeGcmMns,
                     req,
                     m_callerVuid,
                     m_realNameB2,
                     m_realNameGtkeyB2,
                     QByteArray(),
                     m_voipServiceToken,
                     false,
                     seqOut);
}

void EmbeddedVoipBackend::sendGroupHeartbeat()
{
    if (m_authState != AuthAuthenticated || m_roomId == 0 || m_callerVuid == 0) {
        return;
    }
    const QString sigSeq = generateSigSeq(m_targetVuid);
    const QByteArray req = MilinkProto::Builder::buildGroupHeartbeatReq(
        m_callerVuid, m_roomId, sigSeq, m_sessionId
    );
    sendMiLinkPacket(QStringLiteral("voipsdk.signal.gmemberisalive"),
                     MilinkProto::EncodeGcmMns,
                     req,
                     m_callerVuid,
                     m_realNameB2,
                     m_realNameGtkeyB2,
                     QByteArray(),
                     m_voipServiceToken,
                     false,
                     nullptr);
}

void EmbeddedVoipBackend::sendGroupCancel(quint64 roomId, quint64 targetVuid, qint32 groupInviteType, const QString &sessionId)
{
    if (m_authState != AuthAuthenticated || roomId == 0 || m_callerVuid == 0) {
        return;
    }
    qInfo() << "[EmbeddedVoipBackend] Sending voipsdk.signal.group_cancel roomId=" << roomId;
    const QString sigSeq = generateSigSeq(targetVuid);
    const QByteArray req = MilinkProto::Builder::buildGroupCancelReq(
        m_callerVuid, targetVuid, roomId, groupInviteType, sigSeq, sessionId
    );
    sendMiLinkPacket(QStringLiteral("voipsdk.signal.group_cancel"),
                     MilinkProto::EncodeGcmMns,
                     req,
                     m_callerVuid,
                     m_realNameB2,
                     m_realNameGtkeyB2,
                     QByteArray(),
                     m_voipServiceToken,
                     false,
                     &m_seqGroupCancel);
}

void EmbeddedVoipBackend::sendMiLinkKeepalive()
{
    if (m_authState != AuthAuthenticated || !m_tcpSocket || m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    sendMiLinkPacket(QStringLiteral("milink.heartbeat"),
                     MilinkProto::EncodeGcmMns,
                     QByteArray(),
                     m_callerVuid,
                     m_realNameB2,
                     m_realNameGtkeyB2,
                     QByteArray(),
                     m_voipServiceToken,
                     true,
                     nullptr);
}

bool EmbeddedVoipBackend::sendMiLinkPacket(const QString &cmd,
                                           quint8 encodeType,
                                           const QByteArray &busiBuff,
                                           quint64 miUin,
                                           const QByteArray &headerToken,
                                           const QByteArray &aesKey,
                                           const QByteArray &rsaPubB64,
                                           const QString &tokenForTokenInfo,
                                           bool isSystemCmd,
                                           quint32 *outSeq)
{
    if (!m_tcpSocket || m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "[EmbeddedVoipBackend] Cannot send" << cmd << "- socket not connected";
        return false;
    }

    const quint32 seq = ++m_seqId;
    if (outSeq) {
        *outSeq = seq;
    }

    const QByteArray upstream = MilinkProto::Builder::buildUpstreamPacket(
        seq,
        MilinkProto::MILINK_APP_ID,
        cmd,
        busiBuff,
        miUin,
        tokenForTokenInfo,
        true,
        QStringLiteral("N/A"),
        isSystemCmd
    );

    bool frameOk = false;
    const QByteArray frame = MilinkProto::Builder::buildMiLinkV4Frame(
        seq,
        MilinkProto::MILINK_APP_ID,
        encodeType,
        miUin,
        headerToken,
        upstream,
        aesKey,
        rsaPubB64,
        0,
        &frameOk
    );

    if (!frameOk || frame.isEmpty()) {
        qWarning() << "[EmbeddedVoipBackend] Failed encrypting MiLink frame for cmd=" << cmd << "enc=" << encodeType;
        return false;
    }

    const qint64 written = m_tcpSocket->write(frame);
    m_tcpSocket->flush();
    qInfo() << "[EmbeddedVoipBackend] Sent" << cmd << "seq=" << seq << "enc=" << encodeType << "bytes=" << written;
    return written == frame.size();
}

void EmbeddedVoipBackend::handleSocketReadyRead()
{
    m_rxBuffer.append(m_tcpSocket->readAll());

    while (true) {
        MilinkProto::ParsedFrame frame;
        QString err;
        if (!MilinkProto::tryExtractFrame(m_rxBuffer, frame, err)) {
            if (!err.isEmpty()) {
                qWarning() << "[EmbeddedVoipBackend] Frame parse error:" << err;
            }
            break;
        }

        const QByteArray key = lookupDecryptKey(frame);
        MilinkProto::DownstreamMessage msg;
        if (!MilinkProto::Builder::decryptDownstream(frame, key, msg)) {
            qWarning() << "[EmbeddedVoipBackend] Failed decrypting downstream seq=" << frame.seq
                       << "enc=" << frame.encodeType << "authState=" << m_authState;
            continue;
        }

        qInfo() << "[EmbeddedVoipBackend] Recv cmd=" << msg.cmd
                << "seq=" << msg.seq
                << "mns=" << msg.mnsCode
                << "busi=" << msg.busiCode
                << "len=" << msg.busiBuff.size()
                << "err=" << msg.mnsError;

        handleDownstreamMessage(msg);
    }
}

QByteArray EmbeddedVoipBackend::lookupDecryptKey(const MilinkProto::ParsedFrame &frame) const
{
    if (frame.encodeType == MilinkProto::EncodeNone) {
        return QByteArray();
    }
    if (frame.seq == m_seqHandshake || frame.encodeType == MilinkProto::EncodeGcmHandshake) {
        return MilinkProto::handshakeKeyBytes();
    }
    if (frame.seq == m_seqChannel || m_authState == AuthChanneling) {
        return m_privacyKey;
    }
    if (frame.seq == m_seqDeviceLogin || m_authState == AuthDeviceLoggingIn) {
        return m_channelGtkeyB2;
    }
    if (frame.seq == m_seqFastLogin || frame.encodeType == MilinkProto::EncodeCbcFastLogin || m_authState == AuthFastLoggingIn) {
        return m_voipSecurityKey;
    }
    if (!m_realNameGtkeyB2.isEmpty()) {
        return m_realNameGtkeyB2;
    }
    if (!m_voipSecurityKey.isEmpty()) {
        return m_voipSecurityKey;
    }
    if (!m_channelGtkeyB2.isEmpty()) {
        return m_channelGtkeyB2;
    }
    return MilinkProto::handshakeKeyBytes();
}

void EmbeddedVoipBackend::handleDownstreamMessage(const MilinkProto::DownstreamMessage &msg)
{
    // Auth pipeline
    if (msg.seq == m_seqHandshake || msg.cmd == QStringLiteral("milink.handshake")) {
        if (msg.mnsCode != 0) {
            const QString err = tr("MiLink 握手失败 (%1)").arg(msg.mnsCode);
            emit errorOccurred(err);
            finishCallWithState(StateError, err);
            return;
        }
        sendChannel();
        return;
    }

    if (msg.seq == m_seqChannel || msg.cmd == QStringLiteral("milink.channel")) {
        if (msg.mnsCode != 0 ||
            !MilinkProto::Builder::parseChannelRsp(msg.busiBuff, m_channelWid, m_channelB2, m_channelGtkeyB2)) {
            const QString err = tr("MiLink 通道协商失败 (%1)").arg(msg.mnsCode);
            emit errorOccurred(err);
            finishCallWithState(StateError, err);
            return;
        }
        qInfo() << "[EmbeddedVoipBackend] Channel established wid=" << m_channelWid;
        sendDeviceLogin();
        return;
    }

    if (msg.seq == m_seqDeviceLogin || msg.cmd == QStringLiteral("voipsdk.account.devicelogin")) {
        qint32 retCode = -1;
        quint64 vuid = 0;
        QString svcToken;
        QByteArray secKey;
        QString devErr;
        if (msg.mnsCode != 0 ||
            !MilinkProto::Builder::parseDeviceLoginRsp(msg.busiBuff, retCode, vuid, svcToken, secKey, devErr) ||
            retCode != 0 || vuid == 0 || svcToken.isEmpty() || secKey.size() != 16) {
            QString err = tr("设备登录失败 (mns=%1, ret=%2 %3)").arg(msg.mnsCode).arg(retCode).arg(devErr);
            if (retCode == 5003 || retCode == 5000) {
                err = tr("VoIP 凭证无效或已过期 (mns=%1, ret=%2 %3)，正在尝试自动更新凭证...").arg(msg.mnsCode).arg(retCode).arg(devErr);
            }
            qWarning() << "[EmbeddedVoipBackend]" << err;
            m_hasPendingInvite = false;
            emit errorOccurred(err);
            finishCallWithState(StateError, err);
            if (retCode == 5003 || retCode == 5000) {
                emit micoapiAuthInvalid();
            }
            return;
        }
        m_callerVuid = vuid;
        m_voipServiceToken = svcToken;
        m_voipSecurityKey = secKey;
        qInfo() << "[EmbeddedVoipBackend] DeviceLogin OK callerVuid=" << m_callerVuid
                << "secKeyLen=" << m_voipSecurityKey.size();
        sendFastLogin();
        return;
    }

    if (msg.seq == m_seqFastLogin || msg.cmd == QStringLiteral("milink.fastlogin")) {
        if (msg.mnsCode != 0 ||
            !MilinkProto::Builder::parseFastLoginRsp(msg.busiBuff, m_realNameGtkeyB2, m_realNameB2, m_suid)) {
            const QString err = tr("MiLink 实名认证失败 (%1)").arg(msg.mnsCode);
            qWarning() << "[EmbeddedVoipBackend]" << err;
            m_hasPendingInvite = false;
            emit errorOccurred(err);
            finishCallWithState(StateError, err);
            return;
        }
        m_authTimeoutTimer.stop();
        m_authState = AuthAuthenticated;
        m_milinkKeepaliveTimer.start();
        qInfo() << "[EmbeddedVoipBackend] FastLogin OK! suid=" << m_suid;

        if (m_hasPendingInvite) {
            sendPendingGroupInvite();
        }
        return;
    }

    // VoIP signaling responses & pushes
    if (msg.seq == m_seqGroupInvite || msg.cmd == QStringLiteral("voipsdk.signal.group_invite")) {
        MilinkProto::SignalEvent ev;
        if (msg.mnsCode != 0 ||
            !MilinkProto::Builder::parseSignalResponse(msg.busiBuff, ev) ||
            ev.errorCode != 0 || ev.roomId == 0) {
            const QString err = tr("发起呼叫失败 (mns=%1, err=%2 %3)")
                .arg(msg.mnsCode).arg(ev.errorCode).arg(ev.errorMsg);
            qWarning() << "[EmbeddedVoipBackend]" << err;
            emit errorOccurred(err);
            finishCallWithState(StateError, err);
            return;
        }

        m_roomId = ev.roomId;
        if (!ev.sessionId.isEmpty()) {
            m_sessionId = ev.sessionId;
        }

        qInfo() << "[EmbeddedVoipBackend] GroupInvite OK! roomId=" << m_roomId
                << "engine=" << ev.engine << "sessionId=" << m_sessionId
                << "inviteType=" << m_pendingInviteType
                << "directIntercom=" << m_directIntercom;

        m_groupHeartbeatTimer.start();
        sendGetAgoraToken(1, 0);
        sendGetAgoraToken(2, m_roomId);

        if (m_directIntercom) {
            setState(StateCalling, tr("正在建立免接听对讲..."));
            const quint64 callGen = m_callGeneration;
            const QString targetDevId = m_targetDeviceId;
            QTimer::singleShot(1500, this, [this, callGen, targetDevId]() {
                if (m_callGeneration == callGen && m_roomId > 0 && !m_remoteAccepted && !m_remoteJoined && !targetDevId.isEmpty()) {
                    qInfo() << "[EmbeddedVoipBackend] Auto-answering DirectIntercom via UBUS '接听电话' for deviceId=" << targetDevId;
                    sendUbusAiService(targetDevId, QStringLiteral("接听电话"));
                }
            });
        } else {
            setState(StateRinging, tr("正在等待对方接听..."));
            if (!m_targetDeviceId.isEmpty() && (!m_minaServiceToken.isEmpty() || !m_serviceToken.isEmpty())) {
                sendTts(m_targetDeviceId, QStringLiteral("来电话了"));
            }
        }
        return;
    }

    if (msg.cmd == QStringLiteral("voipsdk.signal.getagoratoken") ||
        msg.seq == m_seqAgoraAppId || msg.seq == m_seqAgoraToken) {
        qint32 ret = -1;
        QString token;
        QString appId;
        if (msg.mnsCode != 0 || !MilinkProto::Builder::parseGetAgoraTokenRsp(msg.busiBuff, ret, token, appId) || ret != 0) {
            const QString err = tr("获取声网凭证失败 (mns=%1, ret=%2)").arg(msg.mnsCode).arg(ret);
            qWarning() << "[EmbeddedVoipBackend]" << err;
            emit errorOccurred(err);
            hangupCall();
            return;
        }

        if (msg.seq == m_seqAgoraAppId || !appId.isEmpty()) {
            m_agoraAppId = appId;
            qInfo() << "[EmbeddedVoipBackend] Got Agora AppId:" << m_agoraAppId;
        }
        if (msg.seq == m_seqAgoraToken || !token.isEmpty()) {
            m_agoraToken = token;
            qInfo() << "[EmbeddedVoipBackend] Got Agora Token len=" << m_agoraToken.size();
        }

        if (!m_agoraAppId.isEmpty() && !m_agoraToken.isEmpty() && m_roomId > 0 && !m_agoraJoinRequested) {
            joinAgoraMediaChannel();
        }
        return;
    }

    if (msg.cmd == QStringLiteral("voipsdk.signal.group_push")) {
        handleSignalPush(msg.busiBuff);
        return;
    }

    if (msg.cmd == QStringLiteral("voipsdk.signal.gmemberisalive")) {
        MilinkProto::SignalEvent ev;
        if (MilinkProto::Builder::parseSignalResponse(msg.busiBuff, ev) &&
            ev.errorCode != 0 && ev.errorCode != 1152) {
            qWarning() << "[EmbeddedVoipBackend] Heartbeat returned non-zero errorCode=" << ev.errorCode << ev.errorMsg;
        }
        return;
    }
}

void EmbeddedVoipBackend::handleSignalPush(const QByteArray &payload)
{
    MilinkProto::SignalEvent ev;
    if (!MilinkProto::Builder::parseSignalPush(payload, ev)) {
        qWarning() << "[EmbeddedVoipBackend] Failed parsing SignalPush";
        return;
    }

    qInfo() << "[EmbeddedVoipBackend] SignalPush action=" << ev.action
            << "roomId=" << ev.roomId << "fromVuid=" << ev.fromVuid
            << "inviteType=" << ev.groupInviteType << "members=" << ev.memberVuids;

    if (m_roomId == 0 || (ev.roomId != 0 && ev.roomId != m_roomId)) {
        return;
    }

    switch (ev.action) {
    case MilinkProto::ActionRing:
        qInfo() << "[EmbeddedVoipBackend] Remote speaker ringing (ActionRing)";
        if (m_state == StateCalling && !m_directIntercom) {
            setState(StateRinging, tr("对方正在响铃，等待接听..."));
        }
        break;
    case MilinkProto::ActionAccept:
        qInfo() << "[EmbeddedVoipBackend] Remote speaker accepted! (ActionAccept)";
        m_remoteAccepted = true;
        maybeTransitionToConnected();
        break;
    case MilinkProto::ActionBusy:
        qInfo() << "[EmbeddedVoipBackend] Remote speaker busy";
        ++m_callGeneration;
        m_roomId = 0;
        m_groupHeartbeatTimer.stop();
        m_durationTimer.stop();
        leaveAgoraMediaChannel();
        finishCallWithState(StateError, tr("对方正忙，请稍后再试"));
        break;
    case MilinkProto::ActionCancel:
    case MilinkProto::ActionKick:
        qInfo() << "[EmbeddedVoipBackend] Remote ended call/intercom action=" << ev.action;
        ++m_callGeneration;
        m_roomId = 0;
        m_groupHeartbeatTimer.stop();
        m_durationTimer.stop();
        leaveAgoraMediaChannel();
        finishCallWithState(StateHangup, tr("对方已挂断"));
        break;
    default:
        break;
    }
}

void EmbeddedVoipBackend::joinAgoraMediaChannel()
{
    if (m_agoraJoinRequested || m_agoraAppId.isEmpty() || m_agoraToken.isEmpty() || m_roomId == 0) {
        return;
    }
    m_agoraJoinRequested = true;

    const QString channelName = QString::number(m_roomId);
    qInfo() << "[EmbeddedVoipBackend] Joining Agora channel=" << channelName
            << "uid=" << m_callerVuid << "speakerphone=" << m_speakerphone << "muted=" << m_muted;

#if defined(Q_OS_ANDROID)
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    const jint rc = QJniObject::callStaticMethod<jint>(
        "org/qtproject/example/home_gui/AgoraMediaBridge",
        "joinChannel",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JZZ)I",
        activity.object(),
        QJniObject::fromString(m_agoraAppId).object(),
        QJniObject::fromString(m_agoraToken).object(),
        QJniObject::fromString(channelName).object(),
        static_cast<jlong>(m_callerVuid),
        static_cast<jboolean>(m_speakerphone),
        static_cast<jboolean>(m_muted)
    );
    if (rc != 0) {
        const QString err = tr("加入声网音频频道失败 (rc=%1)").arg(rc);
        emit errorOccurred(err);
        hangupCall();
    }
#elif defined(Q_OS_IOS)
    const int rc = AgoraMediaBridgeIos::joinChannel(
        m_agoraAppId,
        m_agoraToken,
        channelName,
        static_cast<quint32>(m_callerVuid),
        m_speakerphone,
        m_muted
    );
    if (rc != 0) {
        const QString err = tr("加入声网音频频道失败 (rc=%1)").arg(rc);
        emit errorOccurred(err);
        hangupCall();
    }
#else
    qInfo() << "[EmbeddedVoipBackend] Non-Android platform: Agora RTC signaling ready (channel="
            << channelName << ", appId=" << m_agoraAppId << ", waiting C++ RTC plugin).";
    m_agoraJoined = true;
    maybeTransitionToConnected();
#endif
}

void EmbeddedVoipBackend::leaveAgoraMediaChannel()
{
#if defined(Q_OS_ANDROID)
    if (m_agoraJoinRequested || m_agoraJoined) {
        QJniObject::callStaticMethod<void>(
            "org/qtproject/example/home_gui/AgoraMediaBridge",
            "leaveChannel",
            "()V"
        );
    }
#elif defined(Q_OS_IOS)
    if (m_agoraJoinRequested || m_agoraJoined) {
        AgoraMediaBridgeIos::leaveChannel();
    }
#endif
    m_agoraJoinRequested = false;
    m_agoraJoined = false;
}

void EmbeddedVoipBackend::notifyAgoraJoined(const QString &channel, quint32 uid)
{
    qInfo() << "[EmbeddedVoipBackend] Agora joined channel=" << channel << "uid=" << uid;
    if (m_roomId == 0 || channel != QString::number(m_roomId)) {
        return;
    }
    m_agoraJoined = true;
    maybeTransitionToConnected();
}

void EmbeddedVoipBackend::notifyAgoraUserJoined(quint32 uid)
{
    qInfo() << "[EmbeddedVoipBackend] Agora remote user joined uid=" << uid;
    if (m_roomId == 0) {
        return;
    }
    m_remoteJoined = true;
    m_remoteAccepted = true;
    maybeTransitionToConnected();
}

void EmbeddedVoipBackend::notifyAgoraUserOffline(quint32 uid, int reason)
{
    qInfo() << "[EmbeddedVoipBackend] Agora remote user offline uid=" << uid << "reason=" << reason;
    if (m_roomId != 0 && (m_state == StateConnected || m_state == StateRinging)) {
        hangupCall();
    }
}

void EmbeddedVoipBackend::notifyAgoraError(int errCode)
{
    qWarning() << "[EmbeddedVoipBackend] Agora SDK error:" << errCode;
    // Note: transient token/network errors (e.g., 101/109/110) can surface here
    if (errCode == 101 || errCode == 109 || errCode == 110 || errCode == 17) {
        emit errorOccurred(tr("声网音频异常 (%1)").arg(errCode));
    }
}

void EmbeddedVoipBackend::notifyAgoraAudioRoute(bool isSpeaker)
{
    if (m_speakerphone != isSpeaker) {
        m_speakerphone = isSpeaker;
        emit audioRouteChanged(isSpeaker);
    }
}

void EmbeddedVoipBackend::maybeTransitionToConnected()
{
    if (m_roomId == 0 || m_state == StateConnected) {
        return;
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // On desktop/embedded builds without local Agora native media, connect once remote accepts or direct intercom room established
    const bool desktopReady = m_directIntercom ? (m_roomId > 0) : m_remoteAccepted;
    if (!desktopReady) {
        return;
    }
#else
    // For mobile builds with Agora native media, wait until local Agora joined and remote accepted or joined Agora channel
    const bool readyToConnect = m_agoraJoined && (m_remoteJoined || m_remoteAccepted);
    if (!readyToConnect) {
        return;
    }
#endif

    m_callDurationSec = 0;
    m_durationTimer.start();
    emit durationChanged(0);
    setState(StateConnected,
             m_directIntercom ? tr("免接听双向对讲中") : tr("通话中"));
}

void EmbeddedVoipBackend::finishCallWithState(int endState, const QString &statusText)
{
    setState(endState, statusText);
    m_resetIdleTimer.start();
}

void EmbeddedVoipBackend::setState(int newState, const QString &statusText)
{
    m_state = newState;
    emit stateChanged(newState, statusText);
}

void EmbeddedVoipBackend::handleDurationTick()
{
    if (m_state == StateConnected) {
        ++m_callDurationSec;
        emit durationChanged(m_callDurationSec);
    }
}

void EmbeddedVoipBackend::handleGroupHeartbeatTick()
{
    sendGroupHeartbeat();
}

void EmbeddedVoipBackend::handleMiLinkKeepaliveTick()
{
    sendMiLinkKeepalive();
}

QString EmbeddedVoipBackend::generateSigSeq(quint64 targetVuid) const
{
    const qint64 ns = QDateTime::currentMSecsSinceEpoch() * 1000000LL +
                      QRandomGenerator::global()->bounded(1000000);
    return QStringLiteral("%1_%2_%3").arg(m_userId).arg(targetVuid).arg(ns);
}

QString EmbeddedVoipBackend::generateSessionId(quint64 callerVuid) const
{
    const qint64 ns = QDateTime::currentMSecsSinceEpoch() * 1000000LL +
                      QRandomGenerator::global()->bounded(1000000);
    return QStringLiteral("%1%2").arg(callerVuid).arg(ns);
}

void EmbeddedVoipBackend::sendWakeup(const QString &targetVuid)
{
    sendMilinkWakeupHttp(targetVuid);
}

void EmbeddedVoipBackend::sendMilinkWakeupHttp(const QString &uid)
{
    if (m_serviceToken.isEmpty() || uid.isEmpty()) {
        qWarning() << "[EmbeddedVoipBackend] Cannot sendMilinkWakeupHttp: missing token or uid";
        return;
    }

    qInfo() << "[EmbeddedVoipBackend] Sending HTTP notifyMiLinkEnable for uid:" << uid;

    QUrl url(QStringLiteral("https://voip.mina.xiaoaisound.com/remote/voip/milink_enable"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    req.setRawHeader("User-Agent", micoUserAgent().toUtf8());
    req.setRawHeader("Cookie", QStringLiteral("userId=%1; serviceToken=%2").arg(m_userId, m_serviceToken).toUtf8());

    QUrlQuery body;
    body.addQueryItem(QStringLiteral("uid"), uid);

    QNetworkReply *reply = m_networkManager.post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[EmbeddedVoipBackend] milink_enable failed:" << reply->errorString() << payload;
        } else {
            qInfo() << "[EmbeddedVoipBackend] milink_enable OK:" << QString::fromUtf8(payload);
        }
        reply->deleteLater();
    });
}

void EmbeddedVoipBackend::sendTts(const QString &targetDeviceId, const QString &text)
{
    QJsonObject msgObj;
    msgObj.insert(QStringLiteral("text"), text);
    msgObj.insert(QStringLiteral("save"), 0);
    sendUbusCommand(targetDeviceId, QStringLiteral("mibrain"), QStringLiteral("text_to_speech"),
                    QString::fromUtf8(QJsonDocument(msgObj).toJson(QJsonDocument::Compact)));
}

void EmbeddedVoipBackend::queryPlayerStatus(const QString &targetDeviceId)
{
    sendUbusCommand(targetDeviceId, QStringLiteral("mediaplayer"), QStringLiteral("player_get_play_status"),
                    QStringLiteral("{}"));
}

void EmbeddedVoipBackend::sendUbusAiService(const QString &targetDeviceId, const QString &nlpText)
{
    QJsonObject msgObj;
    msgObj.insert(QStringLiteral("nlp"), 1);
    msgObj.insert(QStringLiteral("nlp_execute"), 1);
    msgObj.insert(QStringLiteral("nlp_text"), nlpText);
    msgObj.insert(QStringLiteral("caller"), QStringLiteral("app_android"));
    sendUbusCommand(targetDeviceId, QStringLiteral("mibrain"), QStringLiteral("ai_service"),
                    QString::fromUtf8(QJsonDocument(msgObj).toJson(QJsonDocument::Compact)));
}

void EmbeddedVoipBackend::sendUbusHangupFallback(const QString &targetDeviceId)
{
    sendUbusAiService(targetDeviceId, QStringLiteral("挂断电话"));
}

void EmbeddedVoipBackend::sendUbusCommand(const QString &targetDeviceId, const QString &path,
                                          const QString &method, const QString &messageJson)
{
    // api2.mina.xiaoaisound.com authenticates with the xiaoaisound/micoapi serviceToken (m_serviceToken)
    const QString &token = !m_serviceToken.isEmpty() ? m_serviceToken : m_minaServiceToken;
    if (token.isEmpty() || targetDeviceId.isEmpty()) {
        qWarning() << "[EmbeddedVoipBackend] sendUbusCommand missing token or deviceId";
        return;
    }

    QUrl url(QStringLiteral("https://api2.mina.xiaoaisound.com/remote/ubus"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    req.setRawHeader("User-Agent", micoUserAgent().toUtf8());
    req.setRawHeader("Cookie", QStringLiteral("userId=%1; serviceToken=%2").arg(m_userId, token).toUtf8());

    QUrlQuery body;
    body.addQueryItem(QStringLiteral("deviceId"), targetDeviceId);
    body.addQueryItem(QStringLiteral("path"), path);
    body.addQueryItem(QStringLiteral("method"), method);
    body.addQueryItem(QStringLiteral("message"), messageJson);
    body.addQueryItem(QStringLiteral("requestId"),
                      QStringLiteral("app_android_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(12)));

    QNetworkReply *reply = m_networkManager.post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [reply, method]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[EmbeddedVoipBackend] UBUS" << method << "failed:" << reply->errorString() << payload;
        } else {
            qInfo() << "[EmbeddedVoipBackend] UBUS" << method << "response:" << QString::fromUtf8(payload);
        }
        reply->deleteLater();
    });
}
