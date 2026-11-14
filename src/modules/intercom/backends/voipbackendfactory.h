#ifndef VOIPBACKENDFACTORY_H
#define VOIPBACKENDFACTORY_H

#include "ivoipbackend.h"

class VoipBackendFactory
{
public:
    static QString resolveBackendName(const QString &preferredBackend = QStringLiteral("auto"));
    static IVoipBackend *createBackend(const QString &preferredBackend = QStringLiteral("auto"),
                                     QObject *parent = nullptr);
    static IVoipBackend *createBackend(QObject *parent);
};

#endif // VOIPBACKENDFACTORY_H
