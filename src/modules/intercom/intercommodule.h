#ifndef INTERCOMMODULE_H
#define INTERCOMMODULE_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>
#include <QSettings>

#include "core/imodule.h"
#include "audiorecorder.h"

class ConfigManager;
class IVoipBackend;

class AccessibleCookieJar : public QNetworkCookieJar
{
public:
    using QNetworkCookieJar::QNetworkCookieJar;
    QList<QNetworkCookie> allCookiesList() const { return allCookies(); }
    void setAllCookiesList(const QList<QNetworkCookie> &cookies) { setAllCookies(cookies); }
};

class IntercomModule : public IModule
{
    Q_OBJECT

public:
    explicit IntercomModule(ConfigManager *configManager, GlobalState *globalState, QObject *parent = nullptr);
    ~IntercomModule() override;

    QString name() const override;
    void applyConfig(const AppConfig &config) override;
    void start() override;
    void stop() override;

    // 录音控制
    bool startRecording();
    void stopRecording();
    void cancelRecording();

    // 语音发送
    void sendPendingAudio(const QString &roomId, const QString &roomName);

    // 小米通行证登录与房间拉取
    void login(const QString &username, const QString &password);
    void startQrLogin();
    void cancelQrLogin();
    bool importLocalXiaoAiToken();
    void refreshOAuthToken();
    void saveCredentials(const QString &userId, const QString &serviceToken, const QString &deviceId, const QString &homeId, const QString &refreshToken = QString(), const QString &micoapiToken = QString());
    void fetchRooms();

    // 音箱实时 VoIP 通话控制
    void initVoipRuntime();
    void fetchVoipContacts();
    void fetchXiaoaiDialogue(const QString &deviceId, const QString &hardware = QString(), qint64 timestamp = 0, bool append = false);
    void notifyMiLinkEnable(const QString &uid);
    void startVoipCall(const QString &targetDeviceId, const QString &targetDeviceName, const QString &avatar = QString(),
                       const QString &uid = QString(), const QString &deviceModel = QString(), int deviceType = 0,
                       const QString &did = QString());
    void startDirectIntercom(const QString &targetDeviceId, const QString &targetDeviceName, const QString &avatar = QString(),
                            const QString &uid = QString(), const QString &deviceModel = QString(), int deviceType = 0,
                            const QString &did = QString());
    void hangupVoipCall();
    void setVoipMute(bool mute);
    void setVoipSpeakerphone(bool enable);
    QString voipBackendType() const;
    void setVoipBackendType(const QString &backendType);

    static IntercomModule *instance();
    void onVoipStateChanged(int state, const QString &statusText);
    void onVoipDuration(int durationSec);
    void onVoipAudioRoute(bool isSpeaker);

signals:
    void loginResult(bool success, const QString &message);
    void sendAudioResult(bool success, const QString &message);
    void recordingReadyForRoomSelect(qint64 durationMs);

private slots:
    void handleLoginStep1Reply();
    void handleLoginStep2Reply();
    void handleLoginLocationReply();
    void handleQrLoginUrlReply();
    void handleQrPollReply();
    void handleQrLocationReply();
    void pollQrLogin();
    void handleRefreshTokenReply();
    void handleFetchRoomsReply();
    void handleFetchHomeMembersReply();
    void handleUploadAudioReply();
    void handleVoipContactsReply();
    void handleXiaoaiDialogueReply();

private:
    QString buildAuthHeader() const;
    void ensureDeviceId();
    void initDefaultRooms();
    void parseRoomsFromJson(const QByteArray &payload);
    void fetchMinaDeviceRooms(const QString &token);
    void parseMinaDeviceRooms(const QByteArray &payload);
    void clearAuthCookiesForSid(const QString &sid, bool includePassToken = false);
    QString extractCookieOrHeaderValue(QNetworkReply *reply, const QString &cookieName, const QList<QUrl> &preferredUrls) const;
    QString extractServiceToken(QNetworkReply *reply, const QString &sid = QString());
    QString extractPassToken(QNetworkReply *reply, const QJsonObject &jsonObj = QJsonObject()) const;
    void applyLoginTokens(const QString &userId, const QString &sid, const QString &token, const QString &passToken, const QString &statusMsg = QString());
    void exchangeTokenWithPassToken(const QString &sid);
    void exchangeOAuthTokenWithPassToken();
    void handleOAuthServiceLoginReply(QNetworkReply *reply, const QString &userId, const QString &passToken);
    void requestOAuthAuthorizeCode(const QString &userId, const QString &passToken);
    void handleOAuthAuthorizeRedirect(QNetworkReply *reply, const QString &userId, const QString &passToken, int hopCount = 0);
    void requestOAuthTokenByCode(const QString &authCode);
    static QString buildStsUrl(const QString &locationUrl, const QString &nonce, const QString &ssecurity);
    void startVoipSession(bool directIntercom, const QString &targetDeviceId, const QString &targetDeviceName,
                          const QString &avatar, const QString &uid, const QString &deviceModel,
                          int deviceType, const QString &did);

    ConfigManager *m_configManager = nullptr;
    IntercomConfig m_config;
    AudioRecorder m_audioRecorder;
    AccessibleCookieJar *m_cookieJar = nullptr;
    QNetworkAccessManager m_networkManager;

    QByteArray m_pendingPcmData;
    qint64 m_pendingDurationMs = 0;

    // 登录上下文暂存
    QString m_loginSid = QStringLiteral("micoapi");
    QString m_loginUser;
    QString m_loginPwdHash;
    QString m_loginSign;
    QString m_loginQs;
    QString m_loginCallback;
    QString m_loginCandidateUserId;
    QString m_loginCandidatePassToken;
    QString m_loginCandidateSsecurity;
    QString m_loginCandidateNonce;
    QNetworkReply *m_currentReply = nullptr;

    // 扫码与网页登录长轮询上下文
    bool m_qrLoggingIn = false;
    QString m_qrLoginSid = QStringLiteral("micoapi");
    QString m_qrImageUrl;
    QString m_qrWebUrl;
    QString m_qrLpUrl;
    int m_qrTimeoutSec = 300;
    qint64 m_qrStartTimeSec = 0;
    QNetworkReply *m_qrPollReply = nullptr;
    QNetworkReply *m_qrLocationReply = nullptr;
    bool ensureVoipBackend(const QString &preferredBackend);
    void syncBackendExtraTokens();

    QString m_qrCandidateUserId;
    QString m_qrCandidatePassToken;
    QString m_qrCandidateSsecurity;
    QString m_qrCandidateNonce;
    QSet<QString> m_pendingTokenExchanges;
    bool m_oauthExchanging = false;
    QString m_pendingTargetRoomId;
    QString m_pendingTargetRoomName;
    bool m_retriedVoip401 = false;
    bool m_retriedMicoapi5003 = false;
    bool m_dialogueAppend = false;
    QMap<QString, QString> m_roomNameToId;
    QMap<QString, QString> m_deviceToRoomMap;
    IVoipBackend *m_voipBackend = nullptr;
    QString m_activeVoipBackendName;
    static IntercomModule *s_instance;
};

#endif // INTERCOMMODULE_H
