#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include <QObject>
#include <QString>
#include <QVariantList>

// 全局共享状态对象。
// 后端模块把运行状态写到这里，QML 直接从这里读，不需要关心模块内部实现。
class GlobalState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString platform READ platform NOTIFY platformChanged)
    Q_PROPERTY(QString videoStreamUrl READ videoStreamUrl NOTIFY videoStreamUrlChanged)
    Q_PROPERTY(bool onvifAvailable READ onvifAvailable NOTIFY onvifAvailableChanged)
    Q_PROPERTY(bool onvifProfileSwitchSupported READ onvifProfileSwitchSupported NOTIFY onvifProfileSwitchSupportedChanged)
    Q_PROPERTY(QString onvifCurrentProfile READ onvifCurrentProfile NOTIFY onvifCurrentProfileChanged)
    Q_PROPERTY(QString ptzStatus READ ptzStatus NOTIFY ptzStatusChanged)
    Q_PROPERTY(QString haStatus READ haStatus NOTIFY haStatusChanged)
    Q_PROPERTY(QVariantList haActionStates READ haActionStates NOTIFY haActionStatesChanged)
    Q_PROPERTY(bool wifiAvailable READ wifiAvailable NOTIFY wifiAvailableChanged)
    Q_PROPERTY(bool wifiScanning READ wifiScanning NOTIFY wifiScanningChanged)
    Q_PROPERTY(QString wifiInterface READ wifiInterface NOTIFY wifiInterfaceChanged)
    Q_PROPERTY(QString wifiStatus READ wifiStatus NOTIFY wifiStatusChanged)
    Q_PROPERTY(QVariantList wifiNetworks READ wifiNetworks NOTIFY wifiNetworksChanged)
    Q_PROPERTY(bool xiaozhiAvailable READ xiaozhiAvailable NOTIFY xiaozhiStateChanged)
    Q_PROPERTY(bool xiaozhiOverlayVisible READ xiaozhiOverlayVisible NOTIFY xiaozhiStateChanged)
    Q_PROPERTY(QString xiaozhiState READ xiaozhiState NOTIFY xiaozhiStateChanged)
    Q_PROPERTY(QString xiaozhiStatus READ xiaozhiStatus NOTIFY xiaozhiStateChanged)
    Q_PROPERTY(QString xiaozhiText READ xiaozhiText NOTIFY xiaozhiTextChanged)
    Q_PROPERTY(bool ptzAvailable READ ptzAvailable NOTIFY ptzAvailableChanged)
    Q_PROPERTY(bool reducedEffects READ reducedEffects NOTIFY reducedEffectsChanged)
    Q_PROPERTY(QString activeVideoDecoder READ activeVideoDecoder WRITE setActiveVideoDecoder NOTIFY activeVideoDecoderChanged)
    Q_PROPERTY(bool intercomAvailable READ intercomAvailable NOTIFY intercomStateChanged)
    Q_PROPERTY(QString intercomStatus READ intercomStatus NOTIFY intercomStateChanged)
    Q_PROPERTY(QString intercomUserId READ intercomUserId NOTIFY intercomConfigChanged)
    Q_PROPERTY(QString intercomDeviceId READ intercomDeviceId NOTIFY intercomConfigChanged)
    Q_PROPERTY(QString intercomServiceToken READ intercomServiceToken NOTIFY intercomConfigChanged)
    Q_PROPERTY(QString intercomHomeId READ intercomHomeId NOTIFY intercomConfigChanged)
    Q_PROPERTY(QVariantList intercomRooms READ intercomRooms NOTIFY intercomRoomsChanged)
    Q_PROPERTY(bool intercomRecording READ intercomRecording NOTIFY intercomRecordingChanged)
    Q_PROPERTY(int intercomRecordDuration READ intercomRecordDuration NOTIFY intercomRecordingChanged)
    Q_PROPERTY(qreal intercomAudioLevel READ intercomAudioLevel NOTIFY intercomAudioLevelChanged)
    Q_PROPERTY(QString intercomQrImageUrl READ intercomQrImageUrl NOTIFY intercomQrLoginChanged)
    Q_PROPERTY(QString intercomQrWebUrl READ intercomQrWebUrl NOTIFY intercomQrLoginChanged)
    Q_PROPERTY(QString intercomQrLoginStatus READ intercomQrLoginStatus NOTIFY intercomQrLoginChanged)
    Q_PROPERTY(bool intercomQrLoggingIn READ intercomQrLoggingIn NOTIFY intercomQrLoginChanged)

public:
    explicit GlobalState(QObject *parent = nullptr);

    QString platform() const;
    QString videoStreamUrl() const;
    bool onvifAvailable() const;
    bool onvifProfileSwitchSupported() const;
    QString onvifCurrentProfile() const;
    QString ptzStatus() const;
    QString haStatus() const;
    QVariantList haActionStates() const;
    bool wifiAvailable() const;
    bool wifiScanning() const;
    QString wifiInterface() const;
    QString wifiStatus() const;
    QVariantList wifiNetworks() const;
    bool xiaozhiAvailable() const;
    bool xiaozhiOverlayVisible() const;
    QString xiaozhiState() const;
    QString xiaozhiStatus() const;
    QString xiaozhiText() const;
    bool ptzAvailable() const;
    bool reducedEffects() const;
    QString activeVideoDecoder() const;
    bool intercomAvailable() const;
    QString intercomStatus() const;
    QString intercomUserId() const;
    QString intercomDeviceId() const;
    QString intercomServiceToken() const;
    QString intercomHomeId() const;
    QVariantList intercomRooms() const;
    bool intercomRecording() const;
    int intercomRecordDuration() const;
    qreal intercomAudioLevel() const;
    QString intercomQrImageUrl() const;
    QString intercomQrWebUrl() const;
    QString intercomQrLoginStatus() const;
    bool intercomQrLoggingIn() const;
    bool voipAvailable() const;
    bool voipActive() const;
    int voipState() const;
    QString voipStatusText() const;
    QString voipTargetDeviceId() const;
    QString voipTargetDeviceName() const;
    QString voipTargetAvatar() const;
    int voipDurationSeconds() const;
    bool voipMuted() const;
    bool voipSpeakerphone() const;
    QVariantList voipSpeakers() const;
    QString voipBackendType() const;
    QString micoapiServiceToken() const;
    QString mimcId() const;

public slots:
    void setPlatform(const QString &platform);
    void setVideoStreamUrl(const QString &videoStreamUrl);
    void setOnvifAvailable(bool onvifAvailable);
    void setOnvifProfileSwitchSupported(bool supported);
    void setOnvifCurrentProfile(const QString &profile);
    void setPtzStatus(const QString &ptzStatus);
    void setHaStatus(const QString &haStatus);
    void setHaActionStates(const QVariantList &haActionStates);
    void setWifiAvailable(bool wifiAvailable);
    void setWifiScanning(bool wifiScanning);
    void setWifiInterface(const QString &wifiInterface);
    void setWifiStatus(const QString &wifiStatus);
    void setWifiNetworks(const QVariantList &wifiNetworks);
    void setXiaozhiAvailable(bool available);
    void setXiaozhiOverlayVisible(bool visible);
    void setXiaozhiState(const QString &state);
    void setXiaozhiStatus(const QString &status);
    void setXiaozhiText(const QString &text);
    void setPtzAvailable(bool ptzAvailable);
    void setReducedEffects(bool reducedEffects);
    void setActiveVideoDecoder(const QString &decoder);
    void setIntercomAvailable(bool available);
    void setIntercomStatus(const QString &status);
    void setIntercomUserId(const QString &userId);
    void setIntercomDeviceId(const QString &deviceId);
    void setIntercomServiceToken(const QString &serviceToken);
    void setIntercomHomeId(const QString &homeId);
    void setIntercomRooms(const QVariantList &rooms);
    void setIntercomRecording(bool recording);
    void setIntercomRecordDuration(int duration);
    void setIntercomAudioLevel(qreal level);
    void setIntercomQrImageUrl(const QString &url);
    void setIntercomQrWebUrl(const QString &url);
    void setIntercomQrLoginStatus(const QString &status);
    void setIntercomQrLoggingIn(bool loggingIn);
    void setVoipAvailable(bool available);
    void setVoipActive(bool active);
    void setVoipState(int state, const QString &statusText = QString());
    void setVoipStatusText(const QString &text);
    void setVoipTarget(const QString &deviceId, const QString &deviceName, const QString &avatar = QString());
    void setVoipDurationSeconds(int seconds);
    void setVoipMuted(bool muted);
    void setVoipSpeakerphone(bool speakerphone);
    void setVoipSpeakers(const QVariantList &speakers);
    void setVoipBackendType(const QString &backendType);
    void setMicoapiServiceToken(const QString &token);
    void setMimcId(const QString &mimcId);

signals:
    void platformChanged();
    void videoStreamUrlChanged();
    void onvifAvailableChanged();
    void onvifProfileSwitchSupportedChanged();
    void onvifCurrentProfileChanged();
    void ptzStatusChanged();
    void haStatusChanged();
    void haActionStatesChanged();
    void wifiAvailableChanged();
    void wifiScanningChanged();
    void wifiInterfaceChanged();
    void wifiStatusChanged();
    void wifiNetworksChanged();
    void xiaozhiStateChanged();
    void xiaozhiTextChanged();
    void ptzAvailableChanged();
    void reducedEffectsChanged();
    void activeVideoDecoderChanged();
    void intercomStateChanged();
    void intercomConfigChanged();
    void intercomRoomsChanged();
    void intercomRecordingChanged();
    void intercomAudioLevelChanged();
    void intercomQrLoginChanged();
    void voipAvailableChanged();
    void voipStateChanged();
    void voipTargetChanged();
    void voipDurationChanged();
    void voipAudioSettingsChanged();
    void voipSpeakersChanged();
    void voipBackendTypeChanged();

private:
    QString m_platform;
    QString m_videoStreamUrl;
    bool m_onvifAvailable = false;
    bool m_onvifProfileSwitchSupported = false;
    QString m_onvifCurrentProfile;
    QString m_ptzStatus;
    QString m_haStatus;
    QVariantList m_haActionStates;
    bool m_wifiAvailable = false;
    bool m_wifiScanning = false;
    QString m_wifiInterface;
    QString m_wifiStatus;
    QVariantList m_wifiNetworks;
    bool m_xiaozhiAvailable = false;
    bool m_xiaozhiOverlayVisible = false;
    QString m_xiaozhiState;
    QString m_xiaozhiStatus;
    QString m_xiaozhiText;
    bool m_ptzAvailable = false;
    bool m_reducedEffects = false;
    QString m_activeVideoDecoder;
    bool m_intercomAvailable = false;
    QString m_intercomStatus;
    QString m_intercomUserId;
    QString m_intercomDeviceId;
    QString m_intercomServiceToken;
    QString m_intercomHomeId;
    QVariantList m_intercomRooms;
    bool m_intercomRecording = false;
    int m_intercomRecordDuration = 0;
    qreal m_intercomAudioLevel = 0.0;
    QString m_intercomQrImageUrl;
    QString m_intercomQrWebUrl;
    QString m_intercomQrLoginStatus;
    bool m_intercomQrLoggingIn = false;
    bool m_voipAvailable = false;
    bool m_voipActive = false;
    int m_voipState = 0;
    QString m_voipStatusText;
    QString m_voipTargetDeviceId;
    QString m_voipTargetDeviceName;
    QString m_voipTargetAvatar;
    int m_voipDurationSeconds = 0;
    bool m_voipMuted = false;
    bool m_voipSpeakerphone = true;
    QVariantList m_voipSpeakers;
    QString m_voipBackendType;
    QString m_micoapiServiceToken;
    QString m_mimcId;
};

#endif
