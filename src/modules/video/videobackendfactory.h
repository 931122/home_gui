#ifndef VIDEOBACKENDFACTORY_H
#define VIDEOBACKENDFACTORY_H

#include <QString>

class QObject;
class AbstractVideoBackend;

class VideoBackendFactory
{
public:
    static QString defaultBackendName();
    static AbstractVideoBackend *create(const QString &backendName, QObject *parent = nullptr);
};

#endif
