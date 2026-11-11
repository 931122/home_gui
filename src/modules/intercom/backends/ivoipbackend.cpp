#include "ivoipbackend.h"

// Anchors the vtable and ensures moc runs properly
IVoipBackend::IVoipBackend(QObject *parent)
    : QObject(parent)
{
}

IVoipBackend::~IVoipBackend() = default;
