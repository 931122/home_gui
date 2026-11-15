#include "globalstate.h"

GlobalState::GlobalState(QObject *parent)
    : QObject(parent)
{
}

QString GlobalState::platform() const
{
    return m_platform;
}

QString GlobalState::videoStreamUrl() const
{
    return m_videoStreamUrl;
}

bool GlobalState::onvifAvailable() const
{
    return m_onvifAvailable;
}

bool GlobalState::onvifProfileSwitchSupported() const
{
    return m_onvifProfileSwitchSupported;
}

QString GlobalState::onvifCurrentProfile() const
{
    return m_onvifCurrentProfile;
}

QString GlobalState::ptzStatus() const
{
    return m_ptzStatus;
}

QString GlobalState::haStatus() const
{
    return m_haStatus;
}

QVariantList GlobalState::haActionStates() const
{
    return m_haActionStates;
}

bool GlobalState::wifiAvailable() const
{
    return m_wifiAvailable;
}

bool GlobalState::wifiScanning() const
{
    return m_wifiScanning;
}

QString GlobalState::wifiInterface() const
{
    return m_wifiInterface;
}

QString GlobalState::wifiStatus() const
{
    return m_wifiStatus;
}

QVariantList GlobalState::wifiNetworks() const
{
    return m_wifiNetworks;
}

bool GlobalState::xiaozhiAvailable() const
{
    return m_xiaozhiAvailable;
}

bool GlobalState::xiaozhiOverlayVisible() const
{
    return m_xiaozhiOverlayVisible;
}

QString GlobalState::xiaozhiState() const
{
    return m_xiaozhiState;
}

QString GlobalState::xiaozhiStatus() const
{
    return m_xiaozhiStatus;
}

QString GlobalState::xiaozhiText() const
{
    return m_xiaozhiText;
}

bool GlobalState::ptzAvailable() const
{
    return m_ptzAvailable;
}

bool GlobalState::reducedEffects() const
{
    return m_reducedEffects;
}

QString GlobalState::activeVideoDecoder() const
{
    return m_activeVideoDecoder;
}

void GlobalState::setPlatform(const QString &platform)
{
    // 所有 setter 都只有在值真正变化时才发信号，避免界面无效刷新。
    if (m_platform == platform) {
        return;
    }
    m_platform = platform;
    emit platformChanged();
}

void GlobalState::setVideoStreamUrl(const QString &videoStreamUrl)
{
    // ONVIF 解析成功后会把最终 RTSP 地址回写到这里，供前端播放器使用。
    if (m_videoStreamUrl == videoStreamUrl) {
        return;
    }
    m_videoStreamUrl = videoStreamUrl;
    emit videoStreamUrlChanged();
}

void GlobalState::setOnvifAvailable(bool onvifAvailable)
{
    if (m_onvifAvailable == onvifAvailable) {
        return;
    }
    m_onvifAvailable = onvifAvailable;
    emit onvifAvailableChanged();
}

void GlobalState::setOnvifProfileSwitchSupported(bool supported)
{
    if (m_onvifProfileSwitchSupported == supported) {
        return;
    }
    m_onvifProfileSwitchSupported = supported;
    emit onvifProfileSwitchSupportedChanged();
}

void GlobalState::setOnvifCurrentProfile(const QString &profile)
{
    if (m_onvifCurrentProfile == profile) {
        return;
    }
    m_onvifCurrentProfile = profile;
    emit onvifCurrentProfileChanged();
}

void GlobalState::setPtzStatus(const QString &ptzStatus)
{
    if (m_ptzStatus == ptzStatus) {
        return;
    }
    m_ptzStatus = ptzStatus;
    emit ptzStatusChanged();
}

void GlobalState::setHaStatus(const QString &haStatus)
{
    if (m_haStatus == haStatus) {
        return;
    }
    m_haStatus = haStatus;
    emit haStatusChanged();
}

void GlobalState::setHaActionStates(const QVariantList &haActionStates)
{
    if (m_haActionStates == haActionStates) {
        return;
    }
    m_haActionStates = haActionStates;
    emit haActionStatesChanged();
}

void GlobalState::setWifiAvailable(bool wifiAvailable)
{
    if (m_wifiAvailable == wifiAvailable) {
        return;
    }
    m_wifiAvailable = wifiAvailable;
    emit wifiAvailableChanged();
}

void GlobalState::setWifiScanning(bool wifiScanning)
{
    if (m_wifiScanning == wifiScanning) {
        return;
    }
    m_wifiScanning = wifiScanning;
    emit wifiScanningChanged();
}

void GlobalState::setWifiInterface(const QString &wifiInterface)
{
    if (m_wifiInterface == wifiInterface) {
        return;
    }
    m_wifiInterface = wifiInterface;
    emit wifiInterfaceChanged();
}

void GlobalState::setWifiStatus(const QString &wifiStatus)
{
    if (m_wifiStatus == wifiStatus) {
        return;
    }
    m_wifiStatus = wifiStatus;
    emit wifiStatusChanged();
}

void GlobalState::setWifiNetworks(const QVariantList &wifiNetworks)
{
    if (m_wifiNetworks == wifiNetworks) {
        return;
    }
    m_wifiNetworks = wifiNetworks;
    emit wifiNetworksChanged();
}

void GlobalState::setXiaozhiAvailable(bool available)
{
    if (m_xiaozhiAvailable == available) {
        return;
    }
    m_xiaozhiAvailable = available;
    emit xiaozhiStateChanged();
}

void GlobalState::setXiaozhiOverlayVisible(bool visible)
{
    if (m_xiaozhiOverlayVisible == visible) {
        return;
    }
    m_xiaozhiOverlayVisible = visible;
    emit xiaozhiStateChanged();
}

void GlobalState::setXiaozhiState(const QString &state)
{
    if (m_xiaozhiState == state) {
        return;
    }
    m_xiaozhiState = state;
    emit xiaozhiStateChanged();
}

void GlobalState::setXiaozhiStatus(const QString &status)
{
    if (m_xiaozhiStatus == status) {
        return;
    }
    m_xiaozhiStatus = status;
    emit xiaozhiStateChanged();
}

void GlobalState::setXiaozhiText(const QString &text)
{
    if (m_xiaozhiText == text) {
        return;
    }
    m_xiaozhiText = text;
    emit xiaozhiTextChanged();
}

void GlobalState::setPtzAvailable(bool ptzAvailable)
{
    if (m_ptzAvailable == ptzAvailable) {
        return;
    }
    m_ptzAvailable = ptzAvailable;
    emit ptzAvailableChanged();
}

void GlobalState::setReducedEffects(bool reducedEffects)
{
    if (m_reducedEffects == reducedEffects) {
        return;
    }
    m_reducedEffects = reducedEffects;
    emit reducedEffectsChanged();
}

void GlobalState::setActiveVideoDecoder(const QString &decoder)
{
    if (m_activeVideoDecoder == decoder) {
        return;
    }
    m_activeVideoDecoder = decoder;
    emit activeVideoDecoderChanged();
}

bool GlobalState::intercomAvailable() const
{
    return m_intercomAvailable;
}

QString GlobalState::intercomStatus() const
{
    return m_intercomStatus;
}

QString GlobalState::intercomUserId() const
{
    return m_intercomUserId;
}

QString GlobalState::intercomDeviceId() const
{
    return m_intercomDeviceId;
}

QString GlobalState::intercomServiceToken() const
{
    return m_intercomServiceToken;
}

QString GlobalState::intercomHomeId() const
{
    return m_intercomHomeId;
}

QVariantList GlobalState::intercomRooms() const
{
    return m_intercomRooms;
}

bool GlobalState::intercomRecording() const
{
    return m_intercomRecording;
}

int GlobalState::intercomRecordDuration() const
{
    return m_intercomRecordDuration;
}

qreal GlobalState::intercomAudioLevel() const
{
    return m_intercomAudioLevel;
}

QString GlobalState::intercomQrImageUrl() const
{
    return m_intercomQrImageUrl;
}

QString GlobalState::intercomQrWebUrl() const
{
    return m_intercomQrWebUrl;
}

QString GlobalState::intercomQrLoginStatus() const
{
    return m_intercomQrLoginStatus;
}

bool GlobalState::intercomQrLoggingIn() const
{
    return m_intercomQrLoggingIn;
}

bool GlobalState::voipAvailable() const
{
    return m_voipAvailable;
}

bool GlobalState::voipActive() const
{
    return m_voipActive;
}

int GlobalState::voipState() const
{
    return m_voipState;
}

QString GlobalState::voipStatusText() const
{
    return m_voipStatusText;
}

QString GlobalState::voipTargetDeviceId() const
{
    return m_voipTargetDeviceId;
}

QString GlobalState::voipTargetDeviceName() const
{
    return m_voipTargetDeviceName;
}

QString GlobalState::voipTargetAvatar() const
{
    return m_voipTargetAvatar;
}

int GlobalState::voipDurationSeconds() const
{
    return m_voipDurationSeconds;
}

bool GlobalState::voipMuted() const
{
    return m_voipMuted;
}

bool GlobalState::voipSpeakerphone() const
{
    return m_voipSpeakerphone;
}

QVariantList GlobalState::voipSpeakers() const
{
    return m_voipSpeakers;
}

QString GlobalState::voipBackendType() const
{
    return m_voipBackendType;
}

QString GlobalState::micoapiServiceToken() const
{
    return m_micoapiServiceToken;
}

QString GlobalState::mimcId() const
{
    return m_mimcId;
}

QVariantList GlobalState::xiaoaiDialogueRecords() const
{
    return m_xiaoaiDialogueRecords;
}

bool GlobalState::xiaoaiDialogueLoading() const
{
    return m_xiaoaiDialogueLoading;
}

QString GlobalState::xiaoaiDialogueError() const
{
    return m_xiaoaiDialogueError;
}

qint64 GlobalState::xiaoaiDialogueNextTimestamp() const
{
    return m_xiaoaiDialogueNextTimestamp;
}

void GlobalState::setIntercomAvailable(bool available)
{
    if (m_intercomAvailable == available) {
        return;
    }
    m_intercomAvailable = available;
    emit intercomStateChanged();
}

void GlobalState::setIntercomStatus(const QString &status)
{
    if (m_intercomStatus == status) {
        return;
    }
    m_intercomStatus = status;
    emit intercomStateChanged();
}

void GlobalState::setIntercomUserId(const QString &userId)
{
    if (m_intercomUserId == userId) {
        return;
    }
    m_intercomUserId = userId;
    emit intercomConfigChanged();
}

void GlobalState::setIntercomDeviceId(const QString &deviceId)
{
    if (m_intercomDeviceId == deviceId) {
        return;
    }
    m_intercomDeviceId = deviceId;
    emit intercomConfigChanged();
}

void GlobalState::setIntercomServiceToken(const QString &serviceToken)
{
    if (m_intercomServiceToken == serviceToken) {
        return;
    }
    m_intercomServiceToken = serviceToken;
    emit intercomConfigChanged();
}

void GlobalState::setIntercomHomeId(const QString &homeId)
{
    if (m_intercomHomeId == homeId) {
        return;
    }
    m_intercomHomeId = homeId;
    emit intercomConfigChanged();
}

void GlobalState::setIntercomRooms(const QVariantList &rooms)
{
    m_intercomRooms = rooms;
    emit intercomRoomsChanged();
}

void GlobalState::setIntercomRecording(bool recording)
{
    if (m_intercomRecording == recording) {
        return;
    }
    m_intercomRecording = recording;
    emit intercomRecordingChanged();
}

void GlobalState::setIntercomRecordDuration(int duration)
{
    if (m_intercomRecordDuration == duration) {
        return;
    }
    m_intercomRecordDuration = duration;
    emit intercomRecordingChanged();
}

void GlobalState::setIntercomAudioLevel(qreal level)
{
    m_intercomAudioLevel = level;
    emit intercomAudioLevelChanged();
}

void GlobalState::setIntercomQrImageUrl(const QString &url)
{
    if (m_intercomQrImageUrl == url) {
        return;
    }
    m_intercomQrImageUrl = url;
    emit intercomQrLoginChanged();
}

void GlobalState::setIntercomQrWebUrl(const QString &url)
{
    if (m_intercomQrWebUrl == url) {
        return;
    }
    m_intercomQrWebUrl = url;
    emit intercomQrLoginChanged();
}

void GlobalState::setIntercomQrLoginStatus(const QString &status)
{
    if (m_intercomQrLoginStatus == status) {
        return;
    }
    m_intercomQrLoginStatus = status;
    emit intercomQrLoginChanged();
}

void GlobalState::setIntercomQrLoggingIn(bool loggingIn)
{
    if (m_intercomQrLoggingIn == loggingIn) {
        return;
    }
    m_intercomQrLoggingIn = loggingIn;
    emit intercomQrLoginChanged();
}

void GlobalState::setVoipAvailable(bool available)
{
    if (m_voipAvailable == available) return;
    m_voipAvailable = available;
    emit voipAvailableChanged();
}

void GlobalState::setVoipActive(bool active)
{
    if (m_voipActive == active) return;
    m_voipActive = active;
    emit voipStateChanged();
}

void GlobalState::setVoipState(int state, const QString &statusText)
{
    m_voipState = state;
    if (!statusText.isEmpty()) {
        m_voipStatusText = statusText;
    }
    if (state == 0) {
        m_voipActive = false;
    } else {
        m_voipActive = true;
    }
    emit voipStateChanged();
}

void GlobalState::setVoipStatusText(const QString &text)
{
    if (m_voipStatusText == text) return;
    m_voipStatusText = text;
    emit voipStateChanged();
}

void GlobalState::setVoipTarget(const QString &deviceId, const QString &deviceName, const QString &avatar)
{
    m_voipTargetDeviceId = deviceId;
    m_voipTargetDeviceName = deviceName;
    m_voipTargetAvatar = avatar;
    emit voipTargetChanged();
}

void GlobalState::setVoipDurationSeconds(int seconds)
{
    if (m_voipDurationSeconds == seconds) return;
    m_voipDurationSeconds = seconds;
    emit voipDurationChanged();
}

void GlobalState::setVoipMuted(bool muted)
{
    if (m_voipMuted == muted) return;
    m_voipMuted = muted;
    emit voipAudioSettingsChanged();
}

void GlobalState::setVoipSpeakerphone(bool speakerphone)
{
    if (m_voipSpeakerphone == speakerphone) return;
    m_voipSpeakerphone = speakerphone;
    emit voipAudioSettingsChanged();
}

void GlobalState::setVoipSpeakers(const QVariantList &speakers)
{
    m_voipSpeakers = speakers;
    emit voipSpeakersChanged();
}

void GlobalState::setVoipBackendType(const QString &backendType)
{
    if (m_voipBackendType == backendType) return;
    m_voipBackendType = backendType;
    emit voipBackendTypeChanged();
}

void GlobalState::setMicoapiServiceToken(const QString &token)
{
    m_micoapiServiceToken = token;
}

void GlobalState::setMimcId(const QString &mimcId)
{
    m_mimcId = mimcId;
}

void GlobalState::setXiaoaiDialogueRecords(const QVariantList &records, bool append)
{
    if (append) {
        m_xiaoaiDialogueRecords.append(records);
    } else {
        m_xiaoaiDialogueRecords = records;
    }
    emit xiaoaiDialogueRecordsChanged();
}

void GlobalState::setXiaoaiDialogueLoading(bool loading)
{
    if (m_xiaoaiDialogueLoading == loading) return;
    m_xiaoaiDialogueLoading = loading;
    emit xiaoaiDialogueLoadingChanged();
}

void GlobalState::setXiaoaiDialogueError(const QString &error)
{
    if (m_xiaoaiDialogueError == error) return;
    m_xiaoaiDialogueError = error;
    emit xiaoaiDialogueErrorChanged();
}

void GlobalState::setXiaoaiDialogueNextTimestamp(qint64 ts)
{
    if (m_xiaoaiDialogueNextTimestamp == ts) return;
    m_xiaoaiDialogueNextTimestamp = ts;
    emit xiaoaiDialogueNextTimestampChanged();
}
