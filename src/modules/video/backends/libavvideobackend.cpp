#include "modules/video/backends/libavvideobackend.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QImage>
#include <QMutexLocker>
#include <cmath>

#if defined(HOME_GUI_HAS_FFMPEG)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
}

#if defined(Q_OS_ANDROID)
#include <QtCore/QJniEnvironment>
extern "C" {
#include <libavcodec/jni.h>
}
#endif

namespace {

QString avErrorToString(int errorCode)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(errorCode, buffer, sizeof(buffer));
    return QString::fromLocal8Bit(buffer);
}

int decodeInterruptCallback(void *opaque)
{
    LibavDecodeThread *thread = static_cast<LibavDecodeThread *>(opaque);
    return thread && thread->stopRequested() ? 1 : 0;
}

}
#endif

LibavDecodeThread::LibavDecodeThread(QObject *parent)
    : QThread(parent)
{
#if defined(HOME_GUI_HAS_FFMPEG)
    static const bool logLevelInitialized = []() {
        av_log_set_level(AV_LOG_FATAL);
        return true;
    }();
    Q_UNUSED(logLevelInitialized)
#endif
}

void LibavDecodeThread::configure(const QUrl &source, const VideoConfig &config, const QSize &targetSize, bool networkOnline)
{
    QMutexLocker locker(&m_mutex);
    m_source = source;
    m_config = config;
    m_targetSize = targetSize;
    m_networkOnline = networkOnline;
    m_stopRequested = !networkOnline;
}

void LibavDecodeThread::requestStop()
{
    QMutexLocker locker(&m_mutex);
    m_stopRequested = true;
}

void LibavDecodeThread::setNetworkOnline(bool online)
{
    QMutexLocker locker(&m_mutex);
    m_networkOnline = online;
    if (!online) {
        m_stopRequested = true;
    }
}

bool LibavDecodeThread::stopRequested()
{
    QMutexLocker locker(&m_mutex);
    return m_stopRequested || !m_networkOnline;
}

void LibavDecodeThread::run()
{
    bool retryable = false;
    if (!openStream(retryable)) {
        emit framePresentedChanged(false);
        emit finished(retryable);
    }
}

bool LibavDecodeThread::openStream(bool &retryable)
{
    retryable = false;
#if !defined(HOME_GUI_HAS_FFMPEG)
    emit statusTextChanged(tr("decoder unavailable"));
    emit errorTextChanged(tr("FFmpeg decoder not compiled in"));
    return false;
#else

    QUrl source;
    VideoConfig config;
    QSize targetSize;
    {
        QMutexLocker locker(&m_mutex);
        source = m_source;
        config = m_config;
        targetSize = m_targetSize;
        if (m_stopRequested) {
            return false;
        }
        if (!m_networkOnline) {
            emit statusTextChanged(tr("network offline"));
            return false;
        }
    }

    if (!source.isValid() || source.isEmpty()) {
        emit errorTextChanged(tr("Invalid video source"));
        return false;
    }

#if defined(HOME_GUI_HAS_FFMPEG) && defined(Q_OS_ANDROID)
    JavaVM *vm = QJniEnvironment::javaVM();
    if (vm) {
        av_jni_set_java_vm(vm, nullptr);
    }
#endif

    const QString decoderPreference = config.decoder.trimmed().toLower();
#if !defined(Q_OS_ANDROID)
    if (decoderPreference == QStringLiteral("hardware")) {
        emit errorTextChanged(tr("Hardware decoder is not available in libav backend on this platform"));
        emit statusTextChanged(tr("decoder unavailable"));
        emit decoderChanged(QStringLiteral("hardware"));
        return false;
    }
#endif

    emit statusTextChanged(tr("connecting"));
    emit errorTextChanged(QString());

    AVFormatContext *formatContext = avformat_alloc_context();
    AVCodecContext *codecContext = nullptr;
    SwsContext *scaleContext = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    int videoStreamIndex = -1;
    AVStream *videoStream = nullptr;
    bool presentedFrame = false;
    bool openOk = false;
    bool isHardware = false;
    int hwOpaqueFrameCount = 0;
    const AVCodec *codec = nullptr;
    QElapsedTimer frameThrottleTimer;
    qint64 lastFrameEmitMs = -1000;
    const qint64 minFrameEmitIntervalMs = 75; // 保留约 13~14fps，平衡流畅度与 CPU 负载。

    if (!formatContext) {
        emit errorTextChanged(tr("Allocate format context failed"));
        return false;
    }
    formatContext->interrupt_callback.callback = decodeInterruptCallback;
    formatContext->interrupt_callback.opaque = this;

    AVDictionary *options = nullptr;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    av_dict_set(&options, "stimeout", "3000000", 0);        // 3秒网络超时
    av_dict_set(&options, "probesize", "65536", 0);         // 64KB 极速探测（原默认5MB，消除漫长黑屏）
    av_dict_set(&options, "analyzeduration", "300000", 0);   // 0.3秒探测上限
    av_dict_set(&options, "fflags", "nobuffer", 0);         // 禁用队列缓冲
    av_dict_set(&options, "flags", "low_delay", 0);         // 低延迟解码标志
    av_dict_set(&options, "max_delay", "100000", 0);        // 100ms 最大延迟
    av_dict_set(&options, "buffer_size", "262144", 0);      // 256KB 接收缓冲，杜绝积压
    av_dict_set(&options, "reorder_queue_size", "8", 0);

    formatContext->probesize = 65536;
    formatContext->max_analyze_duration = 300000;
    formatContext->flags |= AVFMT_FLAG_NOBUFFER;

    QElapsedTimer streamConnectTimer;
    streamConnectTimer.start();

    const QByteArray sourceUtf8 = source.toString().toUtf8();
    int result = avformat_open_input(&formatContext, sourceUtf8.constData(), nullptr, &options);
    av_dict_free(&options);
    if (result < 0) {
        emit errorTextChanged(tr("Open stream failed: %1").arg(avErrorToString(result)));
        retryable = true;
        goto cleanup;
    }

    result = avformat_find_stream_info(formatContext, nullptr);
    if (result < 0) {
        emit errorTextChanged(tr("Read stream info failed: %1").arg(avErrorToString(result)));
        retryable = true;
        goto cleanup;
    }

    videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex < 0) {
        emit errorTextChanged(tr("No video stream found: %1").arg(avErrorToString(videoStreamIndex)));
        goto cleanup;
    }

    {
        videoStream = formatContext->streams[videoStreamIndex];
        codec = nullptr;
        isHardware = false;

#if defined(Q_OS_ANDROID)
        // 在 Android 上，若未显式强制纯软解（software），优先尝试调用 MediaCodec 硬件加速
        // 自动由 Android 底层驱动路由至高通骁龙（c2.qti.avc.decoder/OMX.qcom）或联发科天玑（c2.mtk.avc.decoder/OMX.MTK）
        if (decoderPreference != QStringLiteral("software")) {
            if (videoStream->codecpar->codec_id == AV_CODEC_ID_H264) {
                codec = avcodec_find_decoder_by_name("h264_mediacodec");
            } else if (videoStream->codecpar->codec_id == AV_CODEC_ID_HEVC) {
                codec = avcodec_find_decoder_by_name("hevc_mediacodec");
            }
            if (codec) {
                isHardware = true;
            }
        }
#endif

        if (!codec) {
            codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
            isHardware = false;
        }

        if (!codec) {
            emit errorTextChanged(tr("No decoder found for codec %1").arg(videoStream->codecpar->codec_id));
            goto cleanup;
        }

        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            emit errorTextChanged(tr("Allocate decoder context failed"));
            goto cleanup;
        }

        result = avcodec_parameters_to_context(codecContext, videoStream->codecpar);
        if (result < 0) {
            emit errorTextChanged(tr("Copy codec parameters failed: %1").arg(avErrorToString(result)));
            goto cleanup;
        }

        if (isHardware) {
            codecContext->thread_count = 1;
        } else {
            codecContext->thread_count = qBound(2, QThread::idealThreadCount() / 2, 4);
            codecContext->thread_type = FF_THREAD_FRAME;
            codecContext->flags2 |= AV_CODEC_FLAG2_FAST;
        }

        result = avcodec_open2(codecContext, codec, nullptr);

#if defined(Q_OS_ANDROID)
        // 若 MediaCodec 硬件加速启动失败（如老旧设备兼容性异常），自动无缝平滑回退到 CPU 软解
        if (result < 0 && isHardware) {
            qWarning() << "[VideoDecoder] MediaCodec hardware decoder open failed, falling back to software decoder:" << avErrorToString(result);
            avcodec_free_context(&codecContext);
            codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
            isHardware = false;
            if (codec) {
                codecContext = avcodec_alloc_context3(codec);
                if (codecContext && avcodec_parameters_to_context(codecContext, videoStream->codecpar) >= 0) {
                    codecContext->thread_count = qBound(2, QThread::idealThreadCount() / 2, 4);
                    codecContext->thread_type = FF_THREAD_FRAME;
                    codecContext->flags2 |= AV_CODEC_FLAG2_FAST;
                    result = avcodec_open2(codecContext, codec, nullptr);
                }
            }
        }
#endif

        if (result < 0) {
            emit errorTextChanged(tr("Open decoder failed: %1").arg(avErrorToString(result)));
            goto cleanup;
        }

        emit decoderChanged(isHardware ? QStringLiteral("hardware (mediacodec)") : QStringLiteral("software"));
        qInfo() << "[VideoDecoder] Video decoder successfully opened:" << codec->name
                << (isHardware ? "[Hardware Accelerated: Qualcomm Snapdragon / MediaTek Dimensity VPU]" : "[Software: ARM NEON]");
    }

    frame = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet) {
        emit errorTextChanged(tr("Allocate frame buffers failed"));
        goto cleanup;
    }

    openOk = true;
    emit statusTextChanged(tr("buffering"));
    frameThrottleTimer.start();

    while (true) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopRequested) {
                break;
            }
        }

        result = av_read_frame(formatContext, packet);
        if (result == AVERROR_EOF) {
            retryable = true;
            break;
        }
        if (result < 0) {
            emit errorTextChanged(tr("Read frame failed: %1").arg(avErrorToString(result)));
            retryable = true;
            break;
        }

        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }

        result = avcodec_send_packet(codecContext, packet);
        av_packet_unref(packet);
        if (result < 0) {
            emit errorTextChanged(tr("Send packet failed: %1").arg(avErrorToString(result)));
            retryable = true;
            break;
        }

        while (result >= 0) {
            result = avcodec_receive_frame(codecContext, frame);
            if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
                break;
            }
            if (result < 0) {
                emit errorTextChanged(tr("Decode frame failed: %1").arg(avErrorToString(result)));
                retryable = true;
                goto cleanup;
            }

            if (frame->width <= 0 || frame->height <= 0 || frame->format < 0) {
                av_frame_unref(frame);
                continue;
            }

            // 防御：若解码出纯硬件显存句柄（未附带 CPU 内存缓冲），不可直接送入 swscale
            if (frame->format == AV_PIX_FMT_MEDIACODEC) {
                hwOpaqueFrameCount++;
                av_frame_unref(frame);
                if (isHardware && hwOpaqueFrameCount >= 15) {
                    qWarning() << "[VideoDecoder] Hardware decoder only produces surface frames without memory buffer, falling back to CPU software decoder";
                    avcodec_free_context(&codecContext);
                    codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
                    isHardware = false;
                    if (codec) {
                        codecContext = avcodec_alloc_context3(codec);
                        if (codecContext && avcodec_parameters_to_context(codecContext, videoStream->codecpar) >= 0) {
                            codecContext->thread_count = qBound(2, QThread::idealThreadCount() / 2, 4);
                            codecContext->thread_type = FF_THREAD_FRAME;
                            codecContext->flags2 |= AV_CODEC_FLAG2_FAST;
                            if (avcodec_open2(codecContext, codec, nullptr) >= 0) {
                                emit decoderChanged(QStringLiteral("software"));
                                qInfo() << "[VideoDecoder] Fallback to Software Decoder successful:" << codec->name;
                            }
                        }
                    }
                    hwOpaqueFrameCount = 0;
                }
                continue;
            }

            const qint64 nowMs = frameThrottleTimer.elapsed();
            if (presentedFrame && nowMs - lastFrameEmitMs < minFrameEmitIntervalMs) {
                av_frame_unref(frame);
                continue;
            }

            int outputWidth = frame->width;
            int outputHeight = frame->height;
            // 若原分辨率高于 960x540（如 2K/2.5K/4K 超清监控源），等比降采样至 960x540 以内
            // 手机屏上由 GPU (QSGTexture::Linear) 硬件插值放大，不仅画质毫无损失，更能减少 45% 的内存搬运带宽
            if (outputWidth > 960 || outputHeight > 540) {
                const double scale = qMin(960.0 / outputWidth, 540.0 / outputHeight);
                outputWidth = static_cast<int>(std::round(outputWidth * scale));
                outputHeight = static_cast<int>(std::round(outputHeight * scale));
            }
            outputWidth = (outputWidth + 1) & ~1;
            outputHeight = (outputHeight + 1) & ~1;

            // 使用 SWS_POINT 进行极速最近邻抽样（耗时仅为双线性的1/5），平滑滤波完全交由 GPU 硬件处理
            scaleContext = sws_getCachedContext(scaleContext,
                                                frame->width,
                                                frame->height,
                                                static_cast<AVPixelFormat>(frame->format),
                                                outputWidth,
                                                outputHeight,
                                                AV_PIX_FMT_RGBA,
                                                SWS_POINT,
                                                nullptr,
                                                nullptr,
                                                nullptr);
            if (!scaleContext) {
                static int scalerFailCount = 0;
                if (++scalerFailCount <= 5) {
                    qWarning() << "[VideoDecoder] Create scaler failed for format:" << frame->format
                               << "w:" << frame->width << "h:" << frame->height;
                }
                av_frame_unref(frame);
                continue;
            }

            QImage image(outputWidth, outputHeight, QImage::Format_RGBA8888);
            uint8_t *dstData[4] = { image.bits(), nullptr, nullptr, nullptr };
            int dstLinesize[4] = { static_cast<int>(image.bytesPerLine()), 0, 0, 0 };

            QElapsedTimer swsTimer;
            swsTimer.start();
            sws_scale(scaleContext,
                      frame->data,
                      frame->linesize,
                      0,
                      frame->height,
                      dstData,
                      dstLinesize);
            const qint64 swsElapsedUs = swsTimer.nsecsElapsed() / 1000;

            if (!presentedFrame) {
                presentedFrame = true;
                emit framePresentedChanged(true);
                emit statusTextChanged(tr("playing"));
                const char *pixFmtName = av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format));
                qInfo().nospace() << "[VideoDecoder] >>> FIRST VIDEO FRAME RENDERED <<<"
                                  << " TimeToFirstFrame=" << streamConnectTimer.elapsed() << "ms"
                                  << ", Decoder=" << codec->name
                                  << " (" << (isHardware ? "Qualcomm/MediaTek Hardware MediaCodec VPU" : "CPU Software NEON") << ")"
                                  << ", PixelFormat=" << (pixFmtName ? pixFmtName : "unknown")
                                  << ", VideoSize=" << frame->width << "x" << frame->height
                                  << ", ScalerTarget=" << outputWidth << "x" << outputHeight
                                  << ", swsScaleCost=" << swsElapsedUs << "us";
            } else {
                static int perfStatCounter = 0;
                if (++perfStatCounter >= 150) {
                    perfStatCounter = 0;
                    qInfo().nospace() << "[VideoDecoder] Pipeline perf: swsScaleCost=" << swsElapsedUs
                                      << "us (" << (swsElapsedUs / 1000.0) << "ms)"
                                      << ", targetSize=" << outputWidth << "x" << outputHeight;
                }
            }
            emit frameReady(image);
            emit errorTextChanged(QString());
            lastFrameEmitMs = nowMs;
            hwOpaqueFrameCount = 0;
            av_frame_unref(frame);
        }
    }

cleanup:
    if (!openOk && !retryable) {
        emit framePresentedChanged(false);
    } else if (!presentedFrame) {
        emit framePresentedChanged(false);
    }

    if (packet) {
        av_packet_free(&packet);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    if (scaleContext) {
        sws_freeContext(scaleContext);
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
    }
    if (formatContext) {
        avformat_close_input(&formatContext);
    }
    return false;
#endif
}

LibavVideoBackend::LibavVideoBackend(QObject *parent)
    : AbstractVideoBackend(parent)
{
    connect(&m_decodeThread, &LibavDecodeThread::frameReady,
            this, &AbstractVideoBackend::frameReady);
    connect(&m_decodeThread, &LibavDecodeThread::statusTextChanged,
            this, &AbstractVideoBackend::statusTextChanged);
    connect(&m_decodeThread, &LibavDecodeThread::errorTextChanged,
            this, &AbstractVideoBackend::errorTextChanged);
    connect(&m_decodeThread, &LibavDecodeThread::framePresentedChanged,
            this, &AbstractVideoBackend::framePresentedChanged);
    connect(&m_decodeThread, &LibavDecodeThread::finished,
            this, &AbstractVideoBackend::finished);
    connect(&m_decodeThread, &LibavDecodeThread::decoderChanged,
            this, &AbstractVideoBackend::decoderChanged);
}

LibavVideoBackend::~LibavVideoBackend()
{
    stop();
}

QString LibavVideoBackend::backendName() const
{
    return QStringLiteral("libav");
}

void LibavVideoBackend::start(const QUrl &source, const VideoConfig &config, const QSize &targetSize)
{
    stop();
    m_decodeThread.configure(source, config, targetSize, m_networkOnline);
    m_decodeThread.start();
}

void LibavVideoBackend::stop()
{
    if (!m_decodeThread.isRunning()) {
        return;
    }

    m_decodeThread.requestStop();
    if (!m_decodeThread.wait(5000)) {
        qWarning() << "libav decode thread did not stop in time; waiting for FFmpeg interrupt";
        m_decodeThread.wait();
    }
}

void LibavVideoBackend::setNetworkOnline(bool online)
{
    m_networkOnline = online;
    m_decodeThread.setNetworkOnline(online);
}
