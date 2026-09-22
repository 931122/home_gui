#include "modules/video/videoplayer.h"

#include "modules/video/backends/abstractvideobackend.h"
#include "modules/video/videobackendfactory.h"

VideoPlayer::VideoPlayer(QObject *parent)
    : QObject(parent)
{
    m_restartTimer.setInterval(1200);
    m_restartTimer.setSingleShot(true);
    connect(&m_restartTimer, &QTimer::timeout, this, &VideoPlayer::restartPlayback);
}

VideoPlayer::~VideoPlayer()
{
    stop();
}

QUrl VideoPlayer::source() const
{
    return m_source;
}

void VideoPlayer::setSource(const QUrl &source)
{
    if (m_source == source) {
        return;
    }

    if (m_videoBackend) {
        m_restartTimer.stop();
        m_videoBackend->stop();
        setFramePresented(false);
    }

    m_source = source;
    emit sourceChanged();

    if (m_autoPlay) {
        play(m_targetSize);
    }
}

QString VideoPlayer::backend() const
{
    return m_backend;
}

void VideoPlayer::setBackend(const QString &backend)
{
    const QString normalized = backend.trimmed().toLower();
    const QString nextBackend = normalized.isEmpty() ? QStringLiteral("auto") : normalized;
    if (m_backend == nextBackend) {
        return;
    }

    m_backend = nextBackend;
    emit backendChanged();
    ensureBackend();
    if (m_autoPlay) {
        play(m_targetSize);
    }
}

QString VideoPlayer::decoder() const
{
    return m_decoder;
}

QString VideoPlayer::activeDecoder() const
{
    return m_activeDecoder;
}

void VideoPlayer::setDecoder(const QString &decoder)
{
    const QString normalized = decoder.trimmed().toLower();
    const QString nextDecoder = normalized.isEmpty() ? QStringLiteral("auto") : normalized;
    if (m_decoder == nextDecoder) {
        return;
    }

    m_decoder = nextDecoder;
    emit decoderChanged();
    if (m_autoPlay) {
        play(m_targetSize);
    }
}

bool VideoPlayer::autoPlay() const
{
    return m_autoPlay;
}

void VideoPlayer::setAutoPlay(bool autoPlay)
{
    if (m_autoPlay == autoPlay) {
        return;
    }

    m_autoPlay = autoPlay;
    emit autoPlayChanged();
    if (m_autoPlay) {
        play(m_targetSize);
    } else {
        stop();
    }
}

bool VideoPlayer::networkOnline() const
{
    return m_networkOnline;
}

void VideoPlayer::setNetworkOnline(bool networkOnline)
{
    if (m_networkOnline == networkOnline) {
        return;
    }

    m_networkOnline = networkOnline;
    emit networkOnlineChanged();
    if (m_videoBackend) {
        m_videoBackend->setNetworkOnline(networkOnline);
    }
    if (networkOnline) {
        if (m_autoPlay) {
            play(m_targetSize);
        }
    } else {
        stop();
        resetFrameState();
    }
}

bool VideoPlayer::framePresented() const
{
    return m_framePresented;
}

void VideoPlayer::play(const QSize &targetSize)
{
    m_targetSize = targetSize.isValid() ? targetSize : QSize(640, 360);
    m_stopRequested = false;

    if (!m_networkOnline) {
        resetFrameState();
        return;
    }
    if (!m_source.isValid() || m_source.isEmpty()) {
        resetFrameState();
        return;
    }

    ensureBackend();
    if (!m_videoBackend) {
        return;
    }

    m_restartTimer.stop();
    setFramePresented(false);

    VideoConfig config;
    config.backend = m_backend;
    config.decoder = m_decoder;
    m_videoBackend->start(m_source, config, m_targetSize);
}

void VideoPlayer::stop()
{
    m_stopRequested = true;
    m_restartTimer.stop();
    if (m_videoBackend) {
        m_videoBackend->stop();
    }
    resetFrameState();
}

void VideoPlayer::restartPlayback()
{
    if (m_autoPlay && m_networkOnline && !m_source.isEmpty()) {
        play(m_targetSize);
    }
}

void VideoPlayer::handleBackendFrame(const QImage &image)
{
    if (m_stopRequested) {
        return;
    }
    setFramePresented(true);
    emit frameReady(image);
}

void VideoPlayer::handleBackendFinished(bool retryable)
{
    if (m_stopRequested) {
        return;
    }
    if (!m_networkOnline) {
        resetFrameState();
        return;
    }
    if (!retryable) {
        setFramePresented(false);
        return;
    }
    if (m_autoPlay && !m_source.isEmpty()) {
        setFramePresented(false);
        m_restartTimer.start();
    }
}

void VideoPlayer::ensureBackend()
{
    const QString requestedBackend = m_backend.trimmed().isEmpty()
            ? QStringLiteral("auto")
            : m_backend.trimmed().toLower();
    const QString resolvedBackend = requestedBackend == QStringLiteral("auto")
            ? VideoBackendFactory::defaultBackendName()
            : requestedBackend;
    if (m_videoBackend && m_videoBackend->backendName() == resolvedBackend) {
        return;
    }

    if (m_videoBackend) {
        m_videoBackend->stop();
        delete m_videoBackend;
        m_videoBackend = nullptr;
    }

    m_videoBackend = VideoBackendFactory::create(requestedBackend, this);
    if (!m_videoBackend) {
        return;
    }

    connect(m_videoBackend, &AbstractVideoBackend::frameReady,
            this, &VideoPlayer::handleBackendFrame);
    connect(m_videoBackend, &AbstractVideoBackend::framePresentedChanged,
            this, &VideoPlayer::setFramePresented);
    connect(m_videoBackend, &AbstractVideoBackend::decoderChanged,
            this, &VideoPlayer::handleBackendDecoderChanged);
    connect(m_videoBackend, &AbstractVideoBackend::finished,
            this, &VideoPlayer::handleBackendFinished);
    m_videoBackend->setNetworkOnline(m_networkOnline);
}

void VideoPlayer::handleBackendDecoderChanged(const QString &decoderName)
{
    if (m_activeDecoder == decoderName) {
        return;
    }
    m_activeDecoder = decoderName;
    emit activeDecoderChanged(m_activeDecoder);
}

void VideoPlayer::resetFrameState()
{
    setFramePresented(false);
}

void VideoPlayer::setFramePresented(bool framePresented)
{
    if (m_framePresented == framePresented) {
        return;
    }
    m_framePresented = framePresented;
    emit framePresentedChanged();
}
