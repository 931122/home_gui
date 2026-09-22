#ifndef GSTREAMERVIDEOBACKEND_H
#define GSTREAMERVIDEOBACKEND_H

#include <QList>
#include <QSize>
#include <QString>
#include <QTimer>

#include "modules/video/backends/abstractvideobackend.h"

#ifdef HOME_GUI_HAS_GSTREAMER
typedef struct _GstElement GstElement;
typedef struct _GstBus GstBus;
#endif

struct GstreamerPipelineCandidate
{
    QString decoderMode;
    QString decoderElement;
    QString depayElement;
    QString parserElement;
    bool rtspHardware = false;
};

class GstreamerVideoBackend : public AbstractVideoBackend
{
    Q_OBJECT

public:
    explicit GstreamerVideoBackend(QObject *parent = nullptr);
    ~GstreamerVideoBackend() override;

    QString backendName() const override;

public slots:
    void start(const QUrl &source, const VideoConfig &config, const QSize &targetSize) override;
    void stop() override;
    void setNetworkOnline(bool online) override;

private slots:
    void pollBus();
    void handleStartupTimeout();

private:
    void setStoppedState(bool retryable, const QString &statusText);

#ifdef HOME_GUI_HAS_GSTREAMER
    bool initializeGstreamer();
    bool startPipeline(const QUrl &source, const VideoConfig &config, const QSize &targetSize);
    bool tryNextCandidate();
    bool createCandidatePipeline(const GstreamerPipelineCandidate &candidate,
                                 const QUrl &source,
                                 const QSize &targetSize);
    void clearPipeline();
    static int onNewSample(void *userData);

    GstElement *m_pipeline = nullptr;
    GstElement *m_appSink = nullptr;
    GstBus *m_bus = nullptr;
    QList<GstreamerPipelineCandidate> m_candidates;
    int m_candidateIndex = -1;
    bool m_frameReceived = false;
    QUrl m_source;
    QSize m_targetSize;
#endif

    QTimer m_busPollTimer;
    QTimer m_startupTimer;
    bool m_networkOnline = true;
};

#endif
