#include "videomodule.h"

#include <QMetaObject>
#include <QUrl>

#include "core/globalstate.h"
#include "modules/video/onvifclient.h"

namespace {

QUrl cameraUrlFromConfig(const VideoConfig &config)
{
    return QUrl::fromUserInput(config.url.trimmed());
}

bool isRtspCameraUrl(const QUrl &url)
{
    const QString scheme = url.scheme().trimmed().toLower();
    return scheme == QStringLiteral("rtsp") || scheme == QStringLiteral("rtsps");
}

} // namespace

void VideoWorker::initialize(const VideoConfig &config)
{
    m_config = config;
    m_profileToken.clear();
    m_ptzServiceUrl.clear();

    if (!m_onvifClient) {
        m_onvifClient = new OnvifClient(this);
    }

    if (!config.enabled) {
        resetUnavailable(tr("PTZ disabled"));
        return;
    }

    const QUrl cameraUrl = cameraUrlFromConfig(config);
    if (!cameraUrl.isValid() || cameraUrl.isEmpty()) {
        resetUnavailable(tr("PTZ unavailable"));
        return;
    }

    if (isRtspCameraUrl(cameraUrl)) {
        emit streamUriChanged(cameraUrl.toString());
        emit onvifAvailabilityChanged(false);
        emit onvifProfileSwitchSupportedChanged(false);
        emit onvifCurrentProfileChanged(QString());
        emit ptzAvailabilityChanged(false);
        emit ptzStatusChanged(tr("PTZ unavailable"));
        return;
    }

    if (!OnvifClient::isOnvifUrl(cameraUrl)) {
        resetUnavailable(tr("PTZ unavailable"));
        return;
    }

    OnvifStreamInfo streamInfo;
    if (!m_onvifClient->resolveStream(config, streamInfo)) {
        resetUnavailable(tr("PTZ unavailable"));
        return;
    }

    m_profileToken = streamInfo.profileToken;
    m_ptzServiceUrl = streamInfo.ptzServiceUrl;
    const bool ptzAvailable = !m_ptzServiceUrl.isEmpty();

    emit streamUriChanged(streamInfo.streamUri);
    emit onvifAvailabilityChanged(true);
    emit onvifProfileSwitchSupportedChanged(streamInfo.profileSwitchSupported);
    emit onvifCurrentProfileChanged(streamInfo.currentProfile);
    emit ptzAvailabilityChanged(ptzAvailable);
    emit ptzStatusChanged(ptzAvailable ? tr("PTZ idle") : tr("PTZ unavailable"));
}

void VideoWorker::abort()
{
    if (m_onvifClient) {
        m_onvifClient->abortCurrentReply();
    }
}

void VideoWorker::shutdown()
{
    abort();
    m_profileToken.clear();
    m_ptzServiceUrl.clear();
    emit streamUriChanged(QString());
    emit onvifAvailabilityChanged(false);
    emit onvifProfileSwitchSupportedChanged(false);
    emit onvifCurrentProfileChanged(QString());
    emit ptzAvailabilityChanged(false);
    emit ptzStatusChanged(tr("PTZ stopped"));
}

void VideoWorker::movePtz(const QString &direction)
{
    if (!m_config.enabled || m_ptzServiceUrl.isEmpty()) {
        emit ptzStatusChanged(tr("PTZ unavailable"));
        return;
    }

    QString command = direction.trimmed().toLower();
    if (command.isEmpty()) {
        command = QStringLiteral("stop");
    }
    if (m_profileToken.isEmpty()) {
        emit ptzStatusChanged(tr("PTZ profile missing"));
        return;
    }

    QString errorMessage;
    if (!m_onvifClient || !m_onvifClient->sendPtzCommand(m_config, m_profileToken, m_ptzServiceUrl, command, errorMessage)) {
        if (command == QStringLiteral("stop")) {
            emit ptzStatusChanged(tr("PTZ stop failed: %1").arg(errorMessage));
        } else if (command == QStringLiteral("home")) {
            emit ptzStatusChanged(tr("PTZ home failed: %1").arg(errorMessage));
        } else if (errorMessage == QStringLiteral("unknown PTZ command")) {
            emit ptzStatusChanged(tr("PTZ unknown command"));
        } else {
            emit ptzStatusChanged(tr("PTZ %1 failed: %2").arg(command, errorMessage));
        }
        return;
    }

    if (command == QStringLiteral("stop")) {
        emit ptzStatusChanged(tr("PTZ stopped"));
    } else if (command == QStringLiteral("home")) {
        emit ptzStatusChanged(tr("PTZ moving home"));
    } else {
        emit ptzStatusChanged(tr("PTZ moving %1").arg(command));
    }
}

void VideoWorker::resetUnavailable(const QString &ptzStatus)
{
    emit streamUriChanged(QString());
    emit onvifAvailabilityChanged(false);
    emit onvifProfileSwitchSupportedChanged(false);
    emit onvifCurrentProfileChanged(QString());
    emit ptzAvailabilityChanged(false);
    emit ptzStatusChanged(ptzStatus);
}

VideoModule::VideoModule(GlobalState *globalState, QObject *parent)
    : IModule(globalState, parent)
    , m_worker(new VideoWorker)
{
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &VideoWorker::streamUriChanged, globalState, &GlobalState::setVideoStreamUrl);
    connect(m_worker, &VideoWorker::onvifAvailabilityChanged, globalState, &GlobalState::setOnvifAvailable);
    connect(m_worker, &VideoWorker::onvifProfileSwitchSupportedChanged, globalState, &GlobalState::setOnvifProfileSwitchSupported);
    connect(m_worker, &VideoWorker::onvifCurrentProfileChanged, globalState, &GlobalState::setOnvifCurrentProfile);
    connect(m_worker, &VideoWorker::ptzStatusChanged, globalState, &GlobalState::setPtzStatus);
    connect(m_worker, &VideoWorker::ptzAvailabilityChanged, globalState, &GlobalState::setPtzAvailable);
    m_workerThread.start();
}

VideoModule::~VideoModule()
{
    stop();
    m_workerThread.quit();
    m_workerThread.wait();
}

QString VideoModule::name() const
{
    return QStringLiteral("video");
}

void VideoModule::applyConfig(const AppConfig &config)
{
    m_config = config.video;
}

void VideoModule::start()
{
    QMetaObject::invokeMethod(m_worker, "initialize", Qt::QueuedConnection,
                              Q_ARG(VideoConfig, m_config));
}

void VideoModule::stop()
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "abort", Qt::QueuedConnection);
        QMetaObject::invokeMethod(m_worker, "shutdown", Qt::QueuedConnection);
    }
}

void VideoModule::movePtz(const QString &direction)
{
    QMetaObject::invokeMethod(m_worker, "movePtz", Qt::QueuedConnection,
                              Q_ARG(QString, direction));
}
