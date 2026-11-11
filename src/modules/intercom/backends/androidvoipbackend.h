#ifndef ANDROIDVOIPBACKEND_H
#define ANDROIDVOIPBACKEND_H

#include "ivoipbackend.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QCoreApplication>
#endif

class AndroidVoipBackend : public IVoipBackend
{
    Q_OBJECT

public:
    explicit AndroidVoipBackend(QObject *parent = nullptr);
    ~AndroidVoipBackend() override;

    void init(const QString &userId, const QString &serviceToken, const QString &mimcId) override;
    void startCall(const QString &targetDeviceId, const QString &targetDeviceName,
                   const QString &uid, const QString &deviceModel, int deviceType) override;
    void hangupCall() override;
    void setMute(bool mute) override;
    void setSpeakerphone(bool enable) override;
    bool isReady() const override;
    int state() const override;

    // Called by native JNI callbacks
    static AndroidVoipBackend *instance();
    void notifyStateChanged(int state, const QString &statusText);
    void notifyDuration(int durationSec);
    void notifyAudioRoute(bool isSpeaker);

private:
    int m_state = StateIdle;
    bool m_ready = false;
    static AndroidVoipBackend *s_instance;
};

#endif // ANDROIDVOIPBACKEND_H
