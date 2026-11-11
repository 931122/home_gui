#include "androidvoipbackend.h"

#include <QDebug>

#if defined(Q_OS_ANDROID)
#include <QJniEnvironment>

extern "C" {

JNIEXPORT void JNICALL
Java_org_qtproject_example_home_1gui_XiaoAiVoipManager_nativeOnVoipStateChanged(JNIEnv *env, jclass, jint state, jstring statusText)
{
    const char *chars = statusText ? env->GetStringUTFChars(statusText, nullptr) : "";
    const QString text = QString::fromUtf8(chars);
    if (statusText) env->ReleaseStringUTFChars(statusText, chars);

    QMetaObject::invokeMethod(qApp, [state, text]() {
        AndroidVoipBackend *backend = AndroidVoipBackend::instance();
        if (backend) {
            backend->notifyStateChanged(static_cast<int>(state), text);
        }
    }, Qt::QueuedConnection);
}

JNIEXPORT void JNICALL
Java_org_qtproject_example_home_1gui_XiaoAiVoipManager_nativeOnVoipDuration(JNIEnv *, jclass, jint durationSec)
{
    QMetaObject::invokeMethod(qApp, [durationSec]() {
        AndroidVoipBackend *backend = AndroidVoipBackend::instance();
        if (backend) {
            backend->notifyDuration(static_cast<int>(durationSec));
        }
    }, Qt::QueuedConnection);
}

JNIEXPORT void JNICALL
Java_org_qtproject_example_home_1gui_XiaoAiVoipManager_nativeOnVoipAudioRoute(JNIEnv *, jclass, jboolean isSpeaker)
{
    const bool speaker = (isSpeaker != JNI_FALSE);
    QMetaObject::invokeMethod(qApp, [speaker]() {
        AndroidVoipBackend *backend = AndroidVoipBackend::instance();
        if (backend) {
            backend->notifyAudioRoute(speaker);
        }
    }, Qt::QueuedConnection);
}

} // extern "C"
#endif

AndroidVoipBackend *AndroidVoipBackend::s_instance = nullptr;

AndroidVoipBackend::AndroidVoipBackend(QObject *parent)
    : IVoipBackend(parent)
{
    s_instance = this;
}

AndroidVoipBackend::~AndroidVoipBackend()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

AndroidVoipBackend *AndroidVoipBackend::instance()
{
    return s_instance;
}

void AndroidVoipBackend::init(const QString &userId, const QString &serviceToken, const QString &mimcId)
{
#if defined(Q_OS_ANDROID)
    if (serviceToken.isEmpty()) {
        qWarning() << "[AndroidVoipBackend] init: serviceToken is empty.";
        return;
    }
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        qInfo() << "[AndroidVoipBackend] Initializing XiaoAiVoipManager via JNI for user:" << userId;
        QJniObject::callStaticMethod<void>(
            "org/qtproject/example/home_gui/XiaoAiVoipManager",
            "initVoip",
            "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
            activity.object(),
            QJniObject::fromString(userId).object(),
            QJniObject::fromString(serviceToken).object(),
            QJniObject::fromString(mimcId.isEmpty() ? QStringLiteral("251673189") : mimcId).object()
        );
        m_ready = true;
    }
#else
    Q_UNUSED(userId);
    Q_UNUSED(serviceToken);
    Q_UNUSED(mimcId);
#endif
}

void AndroidVoipBackend::startCall(const QString &targetDeviceId, const QString &targetDeviceName,
                                   const QString &uid, const QString &deviceModel, int deviceType)
{
#if defined(Q_OS_ANDROID)
    qInfo() << "[AndroidVoipBackend] Starting VoIP call to" << targetDeviceName << "uid=" << uid;
    m_state = StateCalling;
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/home_gui/XiaoAiVoipManager",
        "startCall",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V",
        QJniObject::fromString(targetDeviceId).object(),
        QJniObject::fromString(targetDeviceName).object(),
        QJniObject::fromString(uid).object(),
        QJniObject::fromString(deviceModel).object(),
        static_cast<jint>(deviceType)
    );
#else
    Q_UNUSED(targetDeviceId);
    Q_UNUSED(targetDeviceName);
    Q_UNUSED(uid);
    Q_UNUSED(deviceModel);
    Q_UNUSED(deviceType);
#endif
}

void AndroidVoipBackend::hangupCall()
{
#if defined(Q_OS_ANDROID)
    qInfo() << "[AndroidVoipBackend] Hanging up VoIP call";
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/home_gui/XiaoAiVoipManager",
        "hangupCall",
        "()V"
    );
#endif
    m_state = StateHangup;
}

void AndroidVoipBackend::setMute(bool mute)
{
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/home_gui/XiaoAiVoipManager",
        "setMute",
        "(Z)V",
        static_cast<jboolean>(mute)
    );
#else
    Q_UNUSED(mute);
#endif
}

void AndroidVoipBackend::setSpeakerphone(bool enable)
{
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/home_gui/XiaoAiVoipManager",
        "setSpeakerphone",
        "(Z)V",
        static_cast<jboolean>(enable)
    );
#else
    Q_UNUSED(enable);
#endif
}

bool AndroidVoipBackend::isReady() const
{
#if defined(Q_OS_ANDROID)
    jboolean ready = QJniObject::callStaticMethod<jboolean>(
        "org/qtproject/example/home_gui/XiaoAiVoipManager",
        "isVoipReady",
        "()Z"
    );
    return ready != JNI_FALSE;
#else
    return false;
#endif
}

int AndroidVoipBackend::state() const
{
    return m_state;
}

void AndroidVoipBackend::notifyStateChanged(int state, const QString &statusText)
{
    m_state = state;
    emit stateChanged(state, statusText);
}

void AndroidVoipBackend::notifyDuration(int durationSec)
{
    emit durationChanged(durationSec);
}

void AndroidVoipBackend::notifyAudioRoute(bool isSpeaker)
{
    emit audioRouteChanged(isSpeaker);
}
