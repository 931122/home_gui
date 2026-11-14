#ifndef IVOIPBACKEND_H
#define IVOIPBACKEND_H

#include <QObject>
#include <QString>

class IVoipBackend : public QObject
{
    Q_OBJECT

public:
    enum VoipState {
        StateIdle = 0,
        StateCalling = 1,
        StateRinging = 2,
        StateConnected = 3,
        StateHangup = 4,
        StateError = 5
    };
    Q_ENUM(VoipState)

    explicit IVoipBackend(QObject *parent = nullptr);
    ~IVoipBackend() override;

    virtual void init(const QString &userId, const QString &serviceToken, const QString &mimcId) = 0;
    virtual void setCallerVuid(quint64 callerVuid) { Q_UNUSED(callerVuid); }
    virtual void startCall(const QString &targetDeviceId, const QString &targetDeviceName,
                           const QString &uid, const QString &deviceModel, int deviceType,
                           const QString &did = QString()) = 0;
    virtual void startDirectIntercom(const QString &targetDeviceId, const QString &targetDeviceName,
                                     const QString &uid, const QString &deviceModel, int deviceType,
                                     const QString &did = QString()) {
        startCall(targetDeviceId, targetDeviceName, uid, deviceModel, deviceType, did);
    }
    virtual void hangupCall() = 0;
    virtual void sendWakeup(const QString &uid) { Q_UNUSED(uid); }
    virtual void sendTts(const QString &deviceId, const QString &text) { Q_UNUSED(deviceId); Q_UNUSED(text); }
    virtual void sendIntercom(const QString &deviceId, const QString &text) { Q_UNUSED(deviceId); Q_UNUSED(text); }
    virtual void setMute(bool mute) = 0;
    virtual void setSpeakerphone(bool enable) = 0;
    virtual bool isReady() const = 0;
    virtual int state() const = 0;

signals:
    void stateChanged(int state, const QString &statusText);
    void durationChanged(int durationSec);
    void audioRouteChanged(bool isSpeaker);
    void errorOccurred(const QString &errorMsg);
};

#endif // IVOIPBACKEND_H
