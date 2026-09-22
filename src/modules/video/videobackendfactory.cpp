#include "videobackendfactory.h"

#include "modules/video/backends/abstractvideobackend.h"
#include "modules/video/backends/gstreamervideobackend.h"
#include "modules/video/backends/libavvideobackend.h"

QString VideoBackendFactory::defaultBackendName()
{
    return QStringLiteral("libav");
}

AbstractVideoBackend *VideoBackendFactory::create(const QString &backendName, QObject *parent)
{
    const QString normalized = backendName.trimmed().toLower();

    if (normalized.isEmpty() || normalized == QStringLiteral("auto")) {
        return create(defaultBackendName(), parent);
    }
    if (normalized == QStringLiteral("gstreamer")) {
        return new GstreamerVideoBackend(parent);
    }
    if (normalized == QStringLiteral("libav")) {
        return new LibavVideoBackend(parent);
    }

    return create(defaultBackendName(), parent);
}
