#ifndef INTERCOMMODULE_H
#define INTERCOMMODULE_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>
#include <QVariantMap>

#include "core/imodule.h"
#include "audiorecorder.h"

class ConfigManager;

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
    void saveCredentials(const QString &userId, const QString &serviceToken, const QString &deviceId, const QString &homeId, const QString &refreshToken = QString());
    void fetchRooms();

    // 音箱实时 VoIP 通话控制
    void initVoipRuntime();
    void fetchVoipContacts();
    void notifyMiLinkEnable(const QString &uid);
    void startVoipCall(const QString &targetDeviceId, const QString &targetDeviceName, const QString &avatar = QString(),
                       const QString &uid = QString(), const QString &deviceModel = QString(), int deviceType = 0);
    void hangupVoipCall();
    void setVoipMute(bool mute);
    void setVoipSpeakerphone(bool enable);

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

private:
    QString buildAuthHeader() const;
    void ensureDeviceId();
    void initDefaultRooms();
    void parseRoomsFromJson(const QByteArray &payload);
    QString extractServiceToken(QNetworkReply *reply);

    ConfigManager *m_configManager = nullptr;
    IntercomConfig m_config;
    AudioRecorder m_audioRecorder;
    QNetworkAccessManager m_networkManager;

    QByteArray m_pendingPcmData;
    qint64 m_pendingDurationMs = 0;

    // 登录上下文暂存
    QString m_loginUser;
    QString m_loginPwdHash;
    QString m_loginSign;
    QString m_loginQs;
    QString m_loginCallback;
    QNetworkReply *m_currentReply = nullptr;

    // 扫码与网页登录长轮询上下文
    bool m_qrLoggingIn = false;
    QString m_qrImageUrl;
    QString m_qrWebUrl;
    QString m_qrLpUrl;
    int m_qrTimeoutSec = 300;
    qint64 m_qrStartTimeSec = 0;
    QNetworkReply *m_qrPollReply = nullptr;
    QNetworkReply *m_qrLocationReply = nullptr;
    QString m_qrCandidateUserId;
    static IntercomModule *s_instance;
};

#endif // INTERCOMMODULE_H
