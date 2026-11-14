#include "voipbackendfactory.h"
#include "embeddedvoipbackend.h"

QString VoipBackendFactory::resolveBackendName(const QString &preferredBackend)
{
    Q_UNUSED(preferredBackend);
    return QStringLiteral("embedded");
}

IVoipBackend *VoipBackendFactory::createBackend(const QString &preferredBackend, QObject *parent)
{
    Q_UNUSED(preferredBackend);
    return new EmbeddedVoipBackend(parent);
}

IVoipBackend *VoipBackendFactory::createBackend(QObject *parent)
{
    return new EmbeddedVoipBackend(parent);
}
