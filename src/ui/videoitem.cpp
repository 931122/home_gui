#include "videoitem.h"

#include <QColor>
#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QQuickWindow>

#include "modules/video/videoplayer.h"

namespace {

QHash<QString, QImage> &sharedFrames()
{
    static QHash<QString, QImage> frames;
    return frames;
}

QMutex &sharedFramesMutex()
{
    static QMutex mutex;
    return mutex;
}

void removeSharedFrame(const QString &key)
{
    if (key.isEmpty()) {
        return;
    }

    QMutexLocker locker(&sharedFramesMutex());
    sharedFrames().remove(key);
}

}

VideoItem::VideoItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_player(new VideoPlayer(this))
{
    setFlag(ItemHasContents, true);
    m_resizeRestartTimer.setSingleShot(true);
    m_resizeRestartTimer.setInterval(250);
    m_sharedFrameTimer.setInterval(100);
    connect(m_player, &VideoPlayer::frameReady,
            this, &VideoItem::handleFrameReady);
    connect(m_player, &VideoPlayer::framePresentedChanged,
            this, &VideoItem::framePresentedChanged);
    connect(m_player, &VideoPlayer::activeDecoderChanged,
            this, &VideoItem::activeDecoderChanged);
    connect(&m_resizeRestartTimer, &QTimer::timeout, this, [this]() {
        if (m_player->autoPlay() && !m_player->source().isEmpty()) {
            play();
        }
    });
    connect(&m_sharedFrameTimer, &QTimer::timeout,
            this, &VideoItem::refreshSharedFrame);
}

VideoItem::~VideoItem()
{
    stop();
}

QUrl VideoItem::source() const
{
    return m_player->source();
}

void VideoItem::setSource(const QUrl &source)
{
    if (m_player->source() == source) {
        return;
    }

    m_player->setSource(source);
    if (!m_useSharedFrame) {
        // 切流时先丢掉本地缓存帧，避免快速连切时短暂显示错路画面。
        m_image = QImage();
        m_imageDirty = true;
        update();
    }
    emit sourceChanged();
}

QString VideoItem::backend() const
{
    return m_player->backend();
}

void VideoItem::setBackend(const QString &backend)
{
    if (m_player->backend() == backend.trimmed().toLower()) {
        return;
    }
    m_player->setBackend(backend);
    emit backendChanged();
}

QString VideoItem::decoder() const
{
    return m_player->decoder();
}

QString VideoItem::activeDecoder() const
{
    return m_player->activeDecoder();
}

void VideoItem::setDecoder(const QString &decoder)
{
    if (m_player->decoder() == decoder.trimmed().toLower()) {
        return;
    }
    m_player->setDecoder(decoder);
    emit decoderChanged();
}

bool VideoItem::autoPlay() const
{
    return m_player->autoPlay();
}

void VideoItem::setAutoPlay(bool autoPlay)
{
    if (m_player->autoPlay() == autoPlay) {
        return;
    }
    m_player->setAutoPlay(autoPlay);
    emit autoPlayChanged();
}

bool VideoItem::networkOnline() const
{
    return m_player->networkOnline();
}

void VideoItem::setNetworkOnline(bool networkOnline)
{
    if (m_player->networkOnline() == networkOnline) {
        return;
    }
    m_player->setNetworkOnline(networkOnline);
    emit networkOnlineChanged();
}

QString VideoItem::sharedFrameKey() const
{
    return m_sharedFrameKey;
}

void VideoItem::setSharedFrameKey(const QString &sharedFrameKey)
{
    const QString nextKey = sharedFrameKey.trimmed();
    if (m_sharedFrameKey == nextKey) {
        return;
    }

    if (m_publishFrames && !m_sharedFrameKey.isEmpty()) {
        removeSharedFrame(m_sharedFrameKey);
    }
    m_sharedFrameKey = nextKey;
    emit sharedFrameKeyChanged();
    if (m_useSharedFrame) {
        m_image = QImage();
        refreshSharedFrame();
    }
}

bool VideoItem::publishFrames() const
{
    return m_publishFrames;
}

void VideoItem::setPublishFrames(bool publishFrames)
{
    if (m_publishFrames == publishFrames) {
        return;
    }
    m_publishFrames = publishFrames;
    emit publishFramesChanged();
}

bool VideoItem::useSharedFrame() const
{
    return m_useSharedFrame;
}

void VideoItem::setUseSharedFrame(bool useSharedFrame)
{
    if (m_useSharedFrame == useSharedFrame) {
        return;
    }
    m_useSharedFrame = useSharedFrame;
    if (m_useSharedFrame) {
        m_sharedFrameTimer.start();
        refreshSharedFrame();
    } else {
        m_sharedFrameTimer.stop();
    }
    emit useSharedFrameChanged();
}

bool VideoItem::framePresented() const
{
    return m_player->framePresented();
}

QSGNode *VideoItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_image.isNull()) {
        delete oldNode;
        return nullptr;
    }

    QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);
    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setFiltering(QSGTexture::Linear);
    }

    if (m_imageDirty) {
        QSGTexture *texture = window()->createTextureFromImage(m_image, QQuickWindow::TextureIsOpaque);
        node->setTexture(texture);
        node->setOwnsTexture(true);
        node->markDirty(QSGNode::DirtyMaterial);
        m_imageDirty = false;
    }

    if (node->texture()) {
        const QRectF target = boundingRect();
        const qreal imageRatio = qreal(m_image.width()) / qMax(1, m_image.height());
        const qreal targetRatio = target.width() / qMax<qreal>(1.0, target.height());
        QRectF drawRect = target;
        if (imageRatio > targetRatio) {
            const qreal height = target.width() / imageRatio;
            drawRect.setY(target.y() + (target.height() - height) / 2.0);
            drawRect.setHeight(height);
        } else {
            const qreal width = target.height() * imageRatio;
            drawRect.setX(target.x() + (target.width() - width) / 2.0);
            drawRect.setWidth(width);
        }
        node->setRect(drawRect);
    }

    return node;
}

void VideoItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

void VideoItem::play()
{
    m_resizeRestartTimer.stop();
    if (m_useSharedFrame) {
        refreshSharedFrame();
        return;
    }
    m_lastTargetFrameSize = targetFrameSize();
    m_player->play(m_lastTargetFrameSize);
}

void VideoItem::stop()
{
    m_resizeRestartTimer.stop();
    if (m_useSharedFrame) {
        m_sharedFrameTimer.stop();
        m_image = QImage();
        m_imageDirty = true;
        update();
        return;
    }
    m_player->stop();
    m_image = QImage();
    m_imageDirty = true;
    update();
    if (m_publishFrames && !m_sharedFrameKey.isEmpty()) {
        removeSharedFrame(m_sharedFrameKey);
    }
}

void VideoItem::handleFrameReady(const QImage &image)
{
    m_image = image;
    m_imageDirty = true;
    if (m_publishFrames && !m_sharedFrameKey.isEmpty()) {
        static qint64 lastSharedInsertMs = 0;
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - lastSharedInsertMs >= 500) { // 缩略图共享帧降频至 2fps，彻底消除每帧高频全局锁竞争
            lastSharedInsertMs = nowMs;
            QMutexLocker locker(&sharedFramesMutex());
            sharedFrames().insert(m_sharedFrameKey, m_image);
        }
    }
    update();
}

void VideoItem::refreshSharedFrame()
{
    if (!m_useSharedFrame || m_sharedFrameKey.isEmpty()) {
        return;
    }

    QImage sharedImage;
    {
        QMutexLocker locker(&sharedFramesMutex());
        sharedImage = sharedFrames().value(m_sharedFrameKey);
    }

    if (!sharedImage.isNull() && sharedImage.cacheKey() != m_image.cacheKey()) {
        m_image = sharedImage;
        m_imageDirty = true;
        update();
    }
}

QSize VideoItem::targetFrameSize() const
{
    const qreal devicePixelRatio = window() ? qMax<qreal>(1.0, window()->effectiveDevicePixelRatio()) : 1.0;
    const int width = qMax(1, qRound(this->width() * devicePixelRatio));
    const int height = qMax(1, qRound(this->height() * devicePixelRatio));
    if (width > 1 && height > 1) {
        return QSize(width, height);
    }
    return QSize(640, 360);
}
