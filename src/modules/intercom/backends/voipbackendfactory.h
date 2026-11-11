#ifndef VOIPBACKENDFACTORY_H
#define VOIPBACKENDFACTORY_H

#include "ivoipbackend.h"

class VoipBackendFactory
{
public:
    static IVoipBackend *createBackend(QObject *parent = nullptr);
};

#endif // VOIPBACKENDFACTORY_H
