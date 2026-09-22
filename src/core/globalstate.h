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
};

#endif
