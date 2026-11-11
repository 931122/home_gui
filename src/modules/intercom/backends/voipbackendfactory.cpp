#include "voipbackendfactory.h"
#include "embeddedvoipbackend.h"

#if defined(Q_OS_ANDROID)
#include "androidvoipbackend.h"
#endif

IVoipBackend *VoipBackendFactory::createBackend(QObject *parent)
{
    // Use the pure C++ EmbeddedVoipBackend on all platforms (Android, Linux, macOS)
    return new EmbeddedVoipBackend(parent);
}
