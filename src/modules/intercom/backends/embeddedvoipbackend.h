#ifndef EMBEDDEDVOIPBACKEND_H
#define EMBEDDEDVOIPBACKEND_H

#include "ivoipbackend.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpSocket>
#include <QTimer>
#include <QBuffer>
#include <QByteArray>

#ifdef HOME_GUI_HAS_MULTIMEDIA
#include <QAudioFormat>
#include <QAudioSink>
#include <QMediaDevices>
#endif

class EmbeddedVoipBackend : public IVoipBackend
{
    Q_OBJECT

public:
    explicit EmbeddedVoipBackend(QObject *parent = nullptr);
    ~EmbeddedVoipBackend() override;

    void init(const QString &userId, const QString &serviceToken, const QString &mimcId) override;
    void startCall(const QString &targetDeviceId, const QString &targetDeviceName,
                   const QString &uid, const QString &deviceModel, int deviceType) override;
    void hangupCall() override;
    void setMute(bool mute) override;
    void setSpeakerphone(bool enable) override;
    bool isReady() const override;
    int state() const override;

private slots:
    void handleSocketConnected();
    void handleSocketDisconnected();
    void handleSocketReadyRead();
    void handleSocketError(QAbstractSocket::SocketError socketError);
    void handleDurationTick();
    void handleRingbackTick();

private:
    void sendMilinkWakeup(const QString &uid);
    void startRingbackTone();
    void stopRingbackTone();
    void connectToMiLinkServer();
    void sendMiLinkPacket(const QString &command, const QByteArray &data, int seqId = 0);
    void setState(int newState, const QString &statusText = QString());

    // Account & Credentials
    QString m_userId;
    QString m_serviceToken;
    QString m_mimcId;

    // Call Context
    int m_state = StateIdle;
    QString m_targetDeviceId;
    QString m_targetDeviceName;
    QString m_targetUid;
    QString m_targetModel;
    int m_targetType = 0;
    bool m_muted = false;
    bool m_speakerphone = true;
    int m_callDurationSec = 0;

    // Timers & Network
    QTimer m_durationTimer;
    QTimer m_ringbackCadenceTimer;
    QNetworkAccessManager m_networkManager;
    QTcpSocket *m_tcpSocket = nullptr;

    // Audio Tone Generation (Pure C++ / Qt Multimedia / ALSA fallback)
#ifdef HOME_GUI_HAS_MULTIMEDIA
    QAudioSink *m_audioSink = nullptr;
    QBuffer m_toneBuffer;
    QByteArray m_toneData;
#endif
    bool m_toneCadenceHigh = false;
    int m_toneCycleCount = 0;
};

#endif // EMBEDDEDVOIPBACKEND_H
