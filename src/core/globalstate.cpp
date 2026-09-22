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
