#ifndef VIDEOITEM_H
#define VIDEOITEM_H

#include <QImage>
#include <QQuickItem>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QTimer>
#include <QUrl>

class VideoItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString backend READ backend WRITE setBackend NOTIFY backendChanged)
    Q_PROPERTY(QString decoder READ decoder WRITE setDecoder NOTIFY decoderChanged)
    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY autoPlayChanged)
    Q_PROPERTY(bool networkOnline READ networkOnline WRITE setNetworkOnline NOTIFY networkOnlineChanged)
    Q_PROPERTY(QString sharedFrameKey READ sharedFrameKey WRITE setSharedFrameKey NOTIFY sharedFrameKeyChanged)
    Q_PROPERTY(bool publishFrames READ publishFrames WRITE setPublishFrames NOTIFY publishFramesChanged)
    Q_PROPERTY(bool useSharedFrame READ useSharedFrame WRITE setUseSharedFrame NOTIFY useSharedFrameChanged)
    Q_PROPERTY(bool framePresented READ framePresented NOTIFY framePresentedChanged)
    Q_PROPERTY(QString activeDecoder READ activeDecoder NOTIFY activeDecoderChanged)

public:
    explicit VideoItem(QQuickItem *parent = nullptr);
    ~VideoItem() override;

    QUrl source() const;
    void setSource(const QUrl &source);

    QString backend() const;
    void setBackend(const QString &backend);

    QString decoder() const;
    void setDecoder(const QString &decoder);

    bool autoPlay() const;
    void setAutoPlay(bool autoPlay);

    bool networkOnline() const;
    void setNetworkOnline(bool networkOnline);

    QString sharedFrameKey() const;
    void setSharedFrameKey(const QString &sharedFrameKey);

    bool publishFrames() const;
    void setPublishFrames(bool publishFrames);

    bool useSharedFrame() const;
    void setUseSharedFrame(bool useSharedFrame);

    bool framePresented() const;
    QString activeDecoder() const;

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

signals:
    void sourceChanged();
    void backendChanged();
    void decoderChanged();
    void activeDecoderChanged();
    void autoPlayChanged();
    void networkOnlineChanged();
    void sharedFrameKeyChanged();
    void publishFramesChanged();
    void useSharedFrameChanged();
    void framePresentedChanged();

public slots:
    void play();
    void stop();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

private slots:
    void handleFrameReady(const QImage &image);
    void refreshSharedFrame();

private:
    QSize targetFrameSize() const;

    QImage m_image;
    bool m_imageDirty = false;
    class VideoPlayer *m_player;
    QTimer m_resizeRestartTimer;
    QTimer m_sharedFrameTimer;
    QSize m_lastTargetFrameSize;
    QString m_sharedFrameKey;
    bool m_publishFrames = false;
    bool m_useSharedFrame = false;
};

#endif
