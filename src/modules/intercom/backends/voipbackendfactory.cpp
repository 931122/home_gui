#include "voipbackendfactory.h"
#include "embeddedvoipbackend.h"

#if defined(Q_OS_ANDROID)
#include "androidvoipbackend.h"
#endif

IVoipBackend *VoipBackendFactory::createBackend(QObject *parent)
{
#if defined(Q_OS_ANDROID)
    return new AndroidVoipBackend(parent);
#else
    return new EmbeddedVoipBackend(parent);
#endif
}
