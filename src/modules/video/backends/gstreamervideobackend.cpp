#include "modules/video/backends/gstreamervideobackend.h"

#include <initializer_list>

#include <QImage>
#include <QMetaObject>

#ifdef HOME_GUI_HAS_GSTREAMER
extern "C" {
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/video/video.h>
}
#endif

namespace {

#ifdef HOME_GUI_HAS_GSTREAMER
QString normalizedDecoder(const QString &decoder)
{
    const QString value = decoder.trimmed().toLower();
    return value.isEmpty() ? QStringLiteral("auto") : value;
}

bool elementExists(const char *factoryName)
{
    GstElementFactory *factory = gst_element_factory_find(factoryName);
    if (!factory) {
        return false;
    }
    gst_object_unref(factory);
    return true;
}

QString rtspLocationWithoutCredentials(const QUrl &source)
{
    QUrl sanitized = source;
    sanitized.setUserName(QString());
    sanitized.setPassword(QString());
    return sanitized.toString();
}

GstElement *makeElement(const char *factoryName, const char *name)
{
    return gst_element_factory_make(factoryName, name);
}

void unrefElement(GstElement *element)
{
    if (element) {
        gst_object_unref(element);
    }
}

bool linkElements(std::initializer_list<GstElement *> elements)
{
    GstElement *previous = nullptr;
    for (GstElement *element : elements) {
        if (!element) {
            return false;
        }
        if (previous && !gst_element_link(previous, element)) {
            return false;
        }
        previous = element;
    }
    return true;
}

void configureAppSink(GstElement *appSink)
{
    g_object_set(appSink,
                 "emit-signals", TRUE,
                 "drop", TRUE,
                 "max-buffers", 1,
                 "sync", FALSE,
                 nullptr);
}

void configureOutputCaps(GstElement *capsFilter, const QSize &targetSize)
{
    Q_UNUSED(targetSize);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "BGRA",
                                        nullptr);
    g_object_set(capsFilter, "caps", caps, nullptr);
    gst_caps_unref(caps);
}

void linkDynamicPadToSink(GstPad *pad, GstElement *sinkElement)
{
    GstPad *sinkPad = gst_element_get_static_pad(sinkElement, "sink");
    if (!sinkPad) {
        return;
    }
    if (!gst_pad_is_linked(sinkPad)) {
        gst_pad_link(pad, sinkPad);
    }
    gst_object_unref(sinkPad);
}

void onDecodebinPadAdded(GstElement *, GstPad *pad, gpointer userData)
{
    GstElement *queue = static_cast<GstElement *>(userData);
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }

    bool isVideo = false;
    if (caps && gst_caps_get_size(caps) > 0) {
        const GstStructure *structure = gst_caps_get_structure(caps, 0);
        const gchar *name = gst_structure_get_name(structure);
        isVideo = name && g_str_has_prefix(name, "video/");
    }
    if (caps) {
        gst_caps_unref(caps);
    }

    if (isVideo) {
        linkDynamicPadToSink(pad, queue);
    }
}

void onRtspPadAdded(GstElement *, GstPad *pad, gpointer userData)
{
    GstElement *depay = static_cast<GstElement *>(userData);
    linkDynamicPadToSink(pad, depay);
}

QList<GstreamerPipelineCandidate> buildHardwareRtspCandidates(const QUrl &source,
                                                              const QSize &targetSize)
{
    Q_UNUSED(source)
    Q_UNUSED(targetSize)
    struct CodecPlan {
        const char *depay;
        const char *parser;
        const char *hardwareDecoders[4];
    };

    static const CodecPlan codecPlans[] = {
        { "rtph264depay", "h264parse",
          { "mppvideodec", "v4l2slh264dec", "v4l2h264dec", nullptr } },
        { "rtph265depay", "h265parse",
          { "mppvideodec", "v4l2slh265dec", "v4l2h265dec", nullptr } }
    };

    QList<GstreamerPipelineCandidate> candidates;
    for (const CodecPlan &plan : codecPlans) {
        if (!elementExists(plan.depay) || !elementExists(plan.parser)) {
            continue;
        }

        for (const char *const *hardwareDecoder = plan.hardwareDecoders; *hardwareDecoder; ++hardwareDecoder) {
            if (!elementExists(*hardwareDecoder)) {
                continue;
            }
            candidates.append({
                QStringLiteral("hardware"),
                QString::fromLatin1(*hardwareDecoder),
                QString::fromLatin1(plan.depay),
                QString::fromLatin1(plan.parser),
                true
            });
        }
    }

    return candidates;
}

QList<GstreamerPipelineCandidate> buildSoftwareCandidates(const QUrl &source,
                                                          const QSize &targetSize)
{
    Q_UNUSED(source)
    Q_UNUSED(targetSize)
    return QList<GstreamerPipelineCandidate>{
        {
            QStringLiteral("software"),
            QStringLiteral("uridecodebin"),
            QString(),
            QString(),
            false
        }
    };
}
#endif

}

GstreamerVideoBackend::GstreamerVideoBackend(QObject *parent)
    : AbstractVideoBackend(parent)
{
    m_busPollTimer.setInterval(100);
    connect(&m_busPollTimer, &QTimer::timeout, this, &GstreamerVideoBackend::pollBus);
    m_startupTimer.setInterval(2500);
    m_startupTimer.setSingleShot(true);
    connect(&m_startupTimer, &QTimer::timeout, this, &GstreamerVideoBackend::handleStartupTimeout);
}

GstreamerVideoBackend::~GstreamerVideoBackend()
{
    stop();
}

QString GstreamerVideoBackend::backendName() const
{
    return QStringLiteral("gstreamer");
}

void GstreamerVideoBackend::start(const QUrl &source, const VideoConfig &config, const QSize &targetSize)
{
    stop();

    if (!m_networkOnline) {
        emit statusTextChanged(tr("network offline"));
        emit framePresentedChanged(false);
        emit finished(false);
        return;
    }

#ifndef HOME_GUI_HAS_GSTREAMER
    Q_UNUSED(source)
    Q_UNUSED(config)
    Q_UNUSED(targetSize)
    emit errorTextChanged(tr("GStreamer backend is not built in this image"));
    emit statusTextChanged(tr("backend unavailable"));
    emit framePresentedChanged(false);
    emit finished(false);
    return;
#else
    if (!initializeGstreamer()) {
        emit errorTextChanged(tr("GStreamer init failed"));
        emit statusTextChanged(tr("backend unavailable"));
        emit framePresentedChanged(false);
        emit finished(false);
        return;
    }

    if (!startPipeline(source, config, targetSize)) {
        emit framePresentedChanged(false);
        emit finished(true);
    }
#endif
}

void GstreamerVideoBackend::stop()
{
#ifdef HOME_GUI_HAS_GSTREAMER
    m_startupTimer.stop();
    m_busPollTimer.stop();
    clearPipeline();
    m_candidates.clear();
    m_candidateIndex = -1;
    m_frameReceived = false;
#endif
}

void GstreamerVideoBackend::setNetworkOnline(bool online)
{
    m_networkOnline = online;
    if (!online) {
        stop();
    }
}

void GstreamerVideoBackend::pollBus()
{
#ifndef HOME_GUI_HAS_GSTREAMER
    return;
#else
    if (!m_bus) {
        return;
    }

    bool keepPolling = true;
    while (GstMessage *message = gst_bus_pop(m_bus)) {
        switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError *error = nullptr;
            gchar *debugInfo = nullptr;
            gst_message_parse_error(message, &error, &debugInfo);
            const QString errorText = error ? QString::fromLocal8Bit(error->message) : tr("Unknown gstreamer error");
            g_clear_error(&error);
            g_free(debugInfo);
            if (!m_frameReceived && tryNextCandidate()) {
                gst_message_unref(message);
                continue;
            }
            emit errorTextChanged(errorText);
            setStoppedState(true, tr("stream error"));
            keepPolling = false;
            break;
        }
        case GST_MESSAGE_EOS:
            if (!m_frameReceived && tryNextCandidate()) {
                gst_message_unref(message);
                continue;
            }
            setStoppedState(true, tr("stream ended"));
            keepPolling = false;
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if (GST_MESSAGE_SRC(message) == GST_OBJECT(m_pipeline)) {
                GstState oldState = GST_STATE_NULL;
                GstState newState = GST_STATE_NULL;
                GstState pendingState = GST_STATE_NULL;
                gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
                if (newState == GST_STATE_PLAYING) {
                    emit statusTextChanged(tr("playing"));
                } else if (newState == GST_STATE_PAUSED) {
                    emit statusTextChanged(tr("buffering"));
                }
            }
            break;
        default:
            break;
        }
        gst_message_unref(message);
        if (!keepPolling || !m_bus) {
            break;
        }
    }
#endif
}

void GstreamerVideoBackend::handleStartupTimeout()
{
#ifdef HOME_GUI_HAS_GSTREAMER
    if (m_frameReceived) {
        return;
    }
    if (tryNextCandidate()) {
        return;
    }
    emit errorTextChanged(tr("No video frame received from any GStreamer pipeline"));
    setStoppedState(true, tr("stream timeout"));
#endif
}

void GstreamerVideoBackend::setStoppedState(bool retryable, const QString &statusText)
{
    stop();
    emit framePresentedChanged(false);
    emit statusTextChanged(statusText);
    emit finished(retryable);
}

#ifdef HOME_GUI_HAS_GSTREAMER
bool GstreamerVideoBackend::initializeGstreamer()
{
    static bool initialized = false;
    static bool ok = false;
    if (initialized) {
        return ok;
    }

    initialized = true;
    int argc = 0;
    char **argv = nullptr;
    ok = gst_init_check(&argc, &argv, nullptr);
    return ok;
}

bool GstreamerVideoBackend::startPipeline(const QUrl &source, const VideoConfig &config, const QSize &targetSize)
{
    const QString decoderPreference = normalizedDecoder(config.decoder);
    QList<GstreamerPipelineCandidate> candidates;
    const bool isRtspSource = source.scheme().compare(QStringLiteral("rtsp"), Qt::CaseInsensitive) == 0
            || source.scheme().compare(QStringLiteral("rtsps"), Qt::CaseInsensitive) == 0;

    if (decoderPreference == QStringLiteral("hardware")) {
        if (isRtspSource) {
            candidates = buildHardwareRtspCandidates(source, targetSize);
        }
    } else if (decoderPreference == QStringLiteral("software")) {
        candidates = buildSoftwareCandidates(source, targetSize);
    } else {
        if (isRtspSource) {
            candidates = buildHardwareRtspCandidates(source, targetSize);
        }
        candidates.append(buildSoftwareCandidates(source, targetSize));
    }

    if (candidates.isEmpty()) {
        emit errorTextChanged(tr("No matching GStreamer decoder pipeline for %1").arg(decoderPreference));
        emit statusTextChanged(tr("decoder unavailable"));
        return false;
    }

    emit statusTextChanged(tr("connecting"));
    emit errorTextChanged(QString());

    m_source = source;
    m_targetSize = targetSize;
    m_candidates = candidates;
    m_candidateIndex = -1;
    m_frameReceived = false;
    return tryNextCandidate();
}

bool GstreamerVideoBackend::tryNextCandidate()
{
    clearPipeline();
    m_startupTimer.stop();

    while (++m_candidateIndex < m_candidates.size()) {
        const GstreamerPipelineCandidate &candidate = m_candidates.at(m_candidateIndex);
        if (!createCandidatePipeline(candidate, m_source, m_targetSize)) {
            continue;
        }

        const GstStateChangeReturn stateResult = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        if (stateResult == GST_STATE_CHANGE_FAILURE) {
            emit errorTextChanged(tr("Start pipeline failed with %1").arg(candidate.decoderElement));
            clearPipeline();
            continue;
        }

        m_frameReceived = false;
        m_busPollTimer.start();
        m_startupTimer.start();
        emit decoderChanged(candidate.decoderMode);
        emit statusTextChanged(tr("connecting via %1").arg(candidate.decoderElement));
        return true;
    }

    emit statusTextChanged(tr("decoder unavailable"));
    return false;
}

bool GstreamerVideoBackend::createCandidatePipeline(const GstreamerPipelineCandidate &candidate,
                                                    const QUrl &source,
                                                    const QSize &targetSize)
{
    GstElement *pipeline = gst_pipeline_new(nullptr);
    GstElement *sourceElement = nullptr;
    GstElement *depay = nullptr;
    GstElement *parser = nullptr;
    GstElement *decoder = nullptr;
    GstElement *queue = makeElement("queue", nullptr);
    GstElement *convert = makeElement("videoconvert", nullptr);
    GstElement *scale = makeElement("videoscale", nullptr);
    GstElement *capsFilter = makeElement("capsfilter", nullptr);
    GstElement *appSink = makeElement("appsink", "video_sink");

    if (!pipeline || !queue || !convert || !scale || !capsFilter || !appSink) {
        emit errorTextChanged(tr("Create GStreamer elements failed"));
        unrefElement(appSink);
        unrefElement(capsFilter);
        unrefElement(scale);
        unrefElement(convert);
        unrefElement(queue);
        unrefElement(pipeline);
        return false;
    }

    g_object_set(queue, "max-size-buffers", 2, "leaky", 2, nullptr);
    configureOutputCaps(capsFilter, targetSize);
    configureAppSink(appSink);

    if (candidate.rtspHardware) {
        sourceElement = makeElement("rtspsrc", nullptr);
        depay = makeElement(candidate.depayElement.toLatin1().constData(), nullptr);
        parser = makeElement(candidate.parserElement.toLatin1().constData(), nullptr);
        decoder = makeElement(candidate.decoderElement.toLatin1().constData(), nullptr);
        if (!sourceElement || !depay || !parser || !decoder) {
            emit errorTextChanged(tr("Create GStreamer hardware pipeline failed"));
            unrefElement(decoder);
            unrefElement(parser);
            unrefElement(depay);
            unrefElement(sourceElement);
            unrefElement(appSink);
            unrefElement(capsFilter);
            unrefElement(scale);
            unrefElement(convert);
            unrefElement(queue);
            unrefElement(pipeline);
            return false;
        }

        g_object_set(sourceElement,
                     "location", rtspLocationWithoutCredentials(source).toUtf8().constData(),
                     "latency", 150,
                     "timeout", static_cast<guint64>(5000000),
                     nullptr);
        if (!source.userName().isEmpty()) {
            g_object_set(sourceElement,
                         "user-id", source.userName().toUtf8().constData(),
                         "user-pw", source.password().toUtf8().constData(),
                         nullptr);
        }

        gst_bin_add_many(GST_BIN(pipeline), sourceElement, depay, parser, decoder,
                         queue, convert, scale, capsFilter, appSink, nullptr);
        if (!linkElements({depay, parser, decoder, queue, convert, scale, capsFilter, appSink})) {
            emit errorTextChanged(tr("Link GStreamer hardware pipeline failed"));
            gst_object_unref(pipeline);
            return false;
        }
        g_signal_connect(sourceElement, "pad-added", G_CALLBACK(onRtspPadAdded), depay);
    } else {
        sourceElement = makeElement("uridecodebin", nullptr);
        if (!sourceElement) {
            emit errorTextChanged(tr("Create GStreamer software pipeline failed"));
            unrefElement(appSink);
            unrefElement(capsFilter);
            unrefElement(scale);
            unrefElement(convert);
            unrefElement(queue);
            unrefElement(pipeline);
            return false;
        }

        g_object_set(sourceElement, "uri", source.toString().toUtf8().constData(), nullptr);
        gst_bin_add_many(GST_BIN(pipeline), sourceElement, queue, convert, scale, capsFilter, appSink, nullptr);
        if (!linkElements({queue, convert, scale, capsFilter, appSink})) {
            emit errorTextChanged(tr("Link GStreamer software pipeline failed"));
            gst_object_unref(pipeline);
            return false;
        }
        g_signal_connect(sourceElement, "pad-added", G_CALLBACK(onDecodebinPadAdded), queue);
    }

    m_pipeline = pipeline;
    m_appSink = GST_ELEMENT(gst_object_ref(appSink));
    m_bus = gst_element_get_bus(pipeline);
    g_signal_connect_swapped(m_appSink, "new-sample", G_CALLBACK(&GstreamerVideoBackend::onNewSample), this);
    return true;
}

void GstreamerVideoBackend::clearPipeline()
{
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_element_get_state(m_pipeline, nullptr, nullptr, GST_SECOND / 2);
    }
    if (m_bus) {
        gst_object_unref(m_bus);
        m_bus = nullptr;
    }
    if (m_appSink) {
        gst_object_unref(m_appSink);
        m_appSink = nullptr;
    }
    if (m_pipeline) {
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }
}

int GstreamerVideoBackend::onNewSample(void *userData)
{
    GstreamerVideoBackend *self = static_cast<GstreamerVideoBackend *>(userData);
    if (!self || !self->m_appSink) {
        return 0;
    }

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(self->m_appSink));
    if (!sample) {
        return 0;
    }

    GstCaps *caps = gst_sample_get_caps(sample);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo mapInfo = {};
    if (!caps || !buffer || !gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return 0;
    }

    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, caps)) {
        gst_buffer_unmap(buffer, &mapInfo);
        gst_sample_unref(sample);
        return 0;
    }

    QImage image(mapInfo.data,
                 static_cast<int>(GST_VIDEO_INFO_WIDTH(&info)),
                 static_cast<int>(GST_VIDEO_INFO_HEIGHT(&info)),
                 static_cast<int>(GST_VIDEO_INFO_PLANE_STRIDE(&info, 0)),
                 QImage::Format_ARGB32);
    const QImage copy = image.copy();
    gst_buffer_unmap(buffer, &mapInfo);
    gst_sample_unref(sample);

    QMetaObject::invokeMethod(self, [self, copy]() {
        self->m_frameReceived = true;
        self->m_startupTimer.stop();
        emit self->frameReady(copy);
        emit self->framePresentedChanged(true);
        emit self->statusTextChanged(self->tr("playing"));
        emit self->errorTextChanged(QString());
    }, Qt::QueuedConnection);
    return 0;
}
#endif
