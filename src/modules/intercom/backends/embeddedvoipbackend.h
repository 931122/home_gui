#ifndef EMBEDDEDVOIPBACKEND_H
#define EMBEDDEDVOIPBACKEND_H

#include "ivoipbackend.h"
#include "milinkprotoutils.h"

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QTcpSocket>
#include <QTimer>

class EmbeddedVoipBackend : public IVoipBackend
{
    Q_OBJECT

public:
    enum AuthState {
        AuthDisconnected = 0,
        AuthTcpConnecting,
        AuthHandshaking,
        AuthChanneling,
        AuthDeviceLoggingIn,
        AuthFastLoggingIn,
        AuthAuthenticated
    };

    explicit EmbeddedVoipBackend(QObject *parent = nullptr);
    ~EmbeddedVoipBackend() override;

    static EmbeddedVoipBackend *instance();

    void init(const QString &userId, const QString &serviceToken, const QString &mimcId) override;
    void startCall(const QString &targetDeviceId, const QString &targetDeviceName,
                   const QString &uid, const QString &deviceModel, int deviceType,
                   const QString &did = QString()) override;
    void startDirectIntercom(const QString &targetDeviceId, const QString &targetDeviceName,
                             const QString &uid, const QString &deviceModel, int deviceType,
                             const QString &did = QString()) override;
    void hangupCall() override;
    void setMute(bool mute) override;
    void setSpeakerphone(bool enable) override;
    bool isReady() const override;
    int state() const override;

    // CLI & extended integration helpers
    void setCallerVuid(quint64 vuid) override;
    void setMinaServiceToken(const QString &minaServiceToken);
    void setTarget(const QString &targetDeviceId, const QString &targetVuid,
                   const QString &targetDid, const QString &targetModel);
    void sendWakeup(const QString &targetVuid) override;
    void sendTts(const QString &targetDeviceId, const QString &text) override;
    void queryPlayerStatus(const QString &targetDeviceId);

    // Agora media callbacks (marshaled onto Qt thread from JNI or future C++ RTC engine)
    void notifyAgoraJoined(const QString &channel, quint32 uid);
    void notifyAgoraUserJoined(quint32 uid);
    void notifyAgoraUserOffline(quint32 uid, int reason);
    void notifyAgoraError(int errCode);
    void notifyAgoraAudioRoute(bool isSpeaker);

signals:
    void errorOccurred(const QString &errorMsg);
    void micoapiAuthInvalid();

private slots:
    void handleSocketConnected();
    void handleSocketDisconnected();
    void handleSocketReadyRead();
    void handleSocketError(QAbstractSocket::SocketError socketError);
    void handleDurationTick();
    void handleGroupHeartbeatTick();
    void handleMiLinkKeepaliveTick();
    void handleAuthTimeout();

private:
    void beginCallFlow(int groupInviteType,
                       const QString &targetDeviceId,
                       const QString &targetDeviceName,
                       const QString &uid,
                       const QString &deviceModel,
                       int deviceType,
                       const QString &did);
    void connectToMiLinkServer(bool advanceHost = false);
    void resetAuthSession(bool closeSocket);
    void sendHandshake();
    void sendChannel();
    void sendDeviceLogin();
    void sendFastLogin();
    void sendPendingGroupInvite();
    void sendGetAgoraToken(qint32 type, quint64 roomId = 0);
    void sendGroupHeartbeat();
    void sendGroupCancel(quint64 roomId, quint64 targetVuid, qint32 groupInviteType, const QString &sessionId);
    void sendMiLinkKeepalive();

    bool sendMiLinkPacket(const QString &cmd,
                          quint8 encodeType,
                          const QByteArray &busiBuff,
                          quint64 miUin,
                          const QByteArray &headerToken,
                          const QByteArray &aesKey,
                          const QByteArray &rsaPubB64,
                          const QString &tokenForTokenInfo,
                          bool isSystemCmd,
                          quint32 *outSeq = nullptr);

    QByteArray lookupDecryptKey(const MilinkProto::ParsedFrame &frame) const;
    void handleDownstreamMessage(const MilinkProto::DownstreamMessage &msg);
    void handleSignalPush(const QByteArray &payload);
    void maybeTransitionToConnected();
    void joinAgoraMediaChannel();
    void leaveAgoraMediaChannel();
    void finishCallWithState(int endState, const QString &statusText);
    void setState(int newState, const QString &statusText = QString());

    void sendMilinkWakeupHttp(const QString &uid);
    void sendUbusAiService(const QString &targetDeviceId, const QString &nlpText);
    void sendUbusHangupFallback(const QString &targetDeviceId);
    void sendUbusCommand(const QString &targetDeviceId, const QString &path, const QString &method, const QString &messageJson);
    QString generateSigSeq(quint64 targetVuid) const;
    QString generateSessionId(quint64 callerVuid) const;

    static EmbeddedVoipBackend *s_instance;

    QString m_userId;
    QString m_serviceToken;      // micoapi serviceToken (used for milink_enable + devicelogin)
    QString m_minaServiceToken;  // optional mina serviceToken (used for UBUS TTS/status)
    QString m_mimcId;

    quint64 m_callerVuid = 0;
    QString m_targetDeviceId;
    QString m_targetDeviceName;
    QString m_targetVuidStr;
    quint64 m_targetVuid = 0;
    QString m_targetDidStr;
    quint64 m_targetDid = 0;
    QString m_targetModel;
    int m_targetDeviceType = 0;

    int m_state = StateIdle;
    AuthState m_authState = AuthDisconnected;
    bool m_muted = false;
    bool m_speakerphone = true;

    QTcpSocket *m_tcpSocket = nullptr;
    QByteArray m_rxBuffer;
    int m_hostIndex = 0;
    int m_reconnectAttempts = 0;
    quint32 m_seqId = 1;

    // MiLink v4 session crypto keys
    int m_rsaKeyIndex = 0;
    QByteArray m_privacyKey;
    quint64 m_channelWid = 0;
    QByteArray m_channelB2;
    QByteArray m_channelGtkeyB2;
    QString m_voipServiceToken;
    QByteArray m_voipSecurityKey;
    QByteArray m_realNameB2;
    QByteArray m_realNameGtkeyB2;
    QString m_suid;

    // Pending/active sequence tracking
    quint32 m_seqHandshake = 0;
    quint32 m_seqChannel = 0;
    quint32 m_seqDeviceLogin = 0;
    quint32 m_seqFastLogin = 0;
    quint32 m_seqGroupInvite = 0;
    quint32 m_seqAgoraAppId = 0;
    quint32 m_seqAgoraToken = 0;
    quint32 m_seqGroupCancel = 0;

    // Call & Agora session state
    bool m_hasPendingInvite = false;
    bool m_directIntercom = false;
    quint64 m_callGeneration = 0;
    qint32 m_pendingInviteType = 1; // MiLink wire groupInviteType (always 1 for client-originated calls)
    QString m_sessionId;
    quint64 m_roomId = 0;
    QString m_agoraAppId;
    QString m_agoraToken;
    bool m_agoraJoinRequested = false;
    bool m_agoraJoined = false;
    bool m_remoteAccepted = false;
    bool m_remoteJoined = false;
    int m_callDurationSec = 0;

    QTimer m_durationTimer;
    QTimer m_groupHeartbeatTimer;
    QTimer m_milinkKeepaliveTimer;
    QTimer m_authTimeoutTimer;
    QTimer m_resetIdleTimer;

    QNetworkAccessManager m_networkManager;
};

#endif // EMBEDDEDVOIPBACKEND_H
