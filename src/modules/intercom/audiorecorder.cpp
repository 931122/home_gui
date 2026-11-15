#include "audiorecorder.h"

#include <QtEndian>
#include <cmath>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>

#if QT_CONFIG(permissions)
#include <QPermissions>
#endif

#if defined(Q_OS_IOS)
#include "audiorecorder_ios.h"
#endif

#if defined(Q_OS_ANDROID)
#include <aaudio/AAudio.h>

static aaudio_data_callback_result_t aaudioDataCallback(
    AAudioStream *stream,
    void *userData,
    void *audioData,
    int32_t numFrames)
{
    Q_UNUSED(stream);
    AudioRecorder *self = static_cast<AudioRecorder *>(userData);
    if (!self || numFrames <= 0) {
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }

    const int byteCount = numFrames * static_cast<int>(sizeof(qint16));
    self->handleAndroidAudioFrame(reinterpret_cast<const char *>(audioData), byteCount);

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AudioRecorder::handleAndroidAudioFrame(const char *data, int len)
{
    if (len <= 0) return;
    QMutexLocker locker(&m_androidPcmMutex);
    m_androidPcmBuffer.append(data, len);
}
#endif

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent)
{
    m_tickTimer.setInterval(100);
    connect(&m_tickTimer, &QTimer::timeout, this, &AudioRecorder::handleTimerTick);
#if defined(Q_OS_IOS)
    AudioRecorderIos::requestPermissionIfNeeded();
#endif
}

AudioRecorder::~AudioRecorder()
{
    cancelRecording();
}

bool AudioRecorder::isRecording() const
{
    return m_isRecording;
}

qint64 AudioRecorder::durationMs() const
{
    return m_isRecording ? m_elapsedTimer.elapsed() : m_durationMs;
}

qreal AudioRecorder::audioLevel() const
{
    return m_audioLevel;
}

bool AudioRecorder::startRecording()
{
    if (m_isRecording) {
        return true;
    }

#if QT_CONFIG(permissions) && !defined(Q_OS_IOS)
    QMicrophonePermission micPermission;
    switch (qApp->checkPermission(micPermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(micPermission, this, [this]() {
            startRecording();
        });
        return false;
    case Qt::PermissionStatus::Denied:
        emit errorOccurred(tr("麦克风权限未授予，无法录音"));
        return false;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif

    m_buffer.close();
    m_buffer.setData(QByteArray());
    m_buffer.open(QIODevice::ReadWrite);

#ifdef HOME_GUI_HAS_MULTIMEDIA
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    if (defaultDevice.isNull()) {
        qWarning() << "[AudioRecorder] No audio input device found";
        emit errorOccurred(tr("未检测到麦克风音频输入设备"));
        return false;
    }

    if (!defaultDevice.isFormatSupported(format)) {
        qWarning() << "[AudioRecorder] 16kHz Mono Int16 not natively supported, attempting default config";
    }

    delete m_audioSource;
    m_audioSource = new QAudioSource(defaultDevice, format, this);
    m_audioIo = m_audioSource->start();
    if (!m_audioIo) {
        qWarning() << "[AudioRecorder] Failed to start QAudioSource";
        emit errorOccurred(tr("麦克风启动失败，请检查麦克风权限"));
        delete m_audioSource;
        m_audioSource = nullptr;
        return false;
    }

    connect(m_audioIo, &QIODevice::readyRead, this, &AudioRecorder::handleAudioDataReady);
#elif defined(Q_OS_ANDROID)
    {
        QMutexLocker locker(&m_androidPcmMutex);
        m_androidPcmBuffer.clear();
    }
    if (m_aaudioStream) {
        AAudioStream_close(m_aaudioStream);
        m_aaudioStream = nullptr;
    }

    AAudioStreamBuilder *builder = nullptr;
    aaudio_result_t res = AAudio_createStreamBuilder(&builder);
    if (res == AAUDIO_OK && builder) {
        AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
        AAudioStreamBuilder_setSampleRate(builder, 16000);
        AAudioStreamBuilder_setChannelCount(builder, 1);
        AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
        AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        AAudioStreamBuilder_setInputPreset(builder, AAUDIO_INPUT_PRESET_VOICE_COMMUNICATION);
        AAudioStreamBuilder_setDataCallback(builder, aaudioDataCallback, this);

        res = AAudioStreamBuilder_openStream(builder, &m_aaudioStream);
        AAudioStreamBuilder_delete(builder);

        if (res == AAUDIO_OK && m_aaudioStream) {
            res = AAudioStream_requestStart(m_aaudioStream);
            if (res != AAUDIO_OK) {
                qWarning() << "[AudioRecorder] AAudioStream_requestStart failed:" << AAudio_convertResultToText(res);
                AAudioStream_close(m_aaudioStream);
                m_aaudioStream = nullptr;
                emit errorOccurred(tr("启动录音失败: %1").arg(QString::fromUtf8(AAudio_convertResultToText(res))));
                return false;
            }
        } else {
            qWarning() << "[AudioRecorder] AAudioStreamBuilder_openStream failed:" << AAudio_convertResultToText(res);
            m_aaudioStream = nullptr;
            emit errorOccurred(tr("初始化麦克风音频流失败"));
            return false;
        }
    } else {
        emit errorOccurred(tr("创建音频流构建器失败"));
        return false;
    }
#elif defined(Q_OS_IOS)
    if (!AudioRecorderIos::startRecording(this)) {
        m_buffer.close();
        return false;
    }
#endif

    m_isRecording = true;
    m_audioLevel = 0.0;
    m_elapsedTimer.start();
    m_tickTimer.start();

    emit recordingStarted();
    emit durationUpdated(0);
    return true;
}

void AudioRecorder::handleAudioChunk(const QByteArray &chunk)
{
    if (!m_isRecording || chunk.isEmpty()) {
        return;
    }

    m_buffer.write(chunk);
    calculateAudioLevel(chunk);
}

void AudioRecorder::handleAudioDataReady()
{
#ifdef HOME_GUI_HAS_MULTIMEDIA
    if (!m_audioIo || !m_isRecording) {
        return;
    }

    const QByteArray chunk = m_audioIo->readAll();
    handleAudioChunk(chunk);
#endif
}

void AudioRecorder::calculateAudioLevel(const QByteArray &chunk)
{
    if (chunk.size() < 2) {
        return;
    }

    const int numSamples = chunk.size() / 2;
    const qint16 *samples = reinterpret_cast<const qint16 *>(chunk.constData());
    double sumSquares = 0.0;

    for (int i = 0; i < numSamples; ++i) {
        const double val = qFromLittleEndian<qint16>(samples[i]);
        sumSquares += val * val;
    }

    const double rms = std::sqrt(sumSquares / numSamples);
    // 归一化到 0.0 ~ 1.0 (常用语音范围 500 ~ 16000)
    const qreal level = qBound(0.0, rms / 12000.0, 1.0);
    m_audioLevel = level;
    emit audioLevelChanged(level);
}

void AudioRecorder::handleTimerTick()
{
    if (!m_isRecording) {
        return;
    }

    const int seconds = static_cast<int>(m_elapsedTimer.elapsed() / 1000);
    emit durationUpdated(seconds);

#if defined(Q_OS_IOS)
    const qreal iosLvl = AudioRecorderIos::currentAudioLevel();
    m_audioLevel = iosLvl;
    emit audioLevelChanged(iosLvl);
#elif defined(Q_OS_ANDROID)
    QByteArray recentChunk;
    {
        QMutexLocker locker(&m_androidPcmMutex);
        const int sampleBytes = 640;
        if (m_androidPcmBuffer.size() >= sampleBytes) {
            recentChunk = m_androidPcmBuffer.right(sampleBytes);
        }
    }
    if (!recentChunk.isEmpty()) {
        calculateAudioLevel(recentChunk);
    }
#elif !defined(HOME_GUI_HAS_MULTIMEDIA)
    // 模拟录音电平波动（针对无多媒体库平台保活）
    static int phase = 0;
    phase = (phase + 1) % 10;
    const qreal mockLevel = 0.2 + 0.3 * std::sin(phase * 0.6);
    m_audioLevel = mockLevel;
    emit audioLevelChanged(mockLevel);
#endif
}

QByteArray AudioRecorder::stopRecording(qint64 *outDurationMs)
{
    if (!m_isRecording) {
        if (outDurationMs) *outDurationMs = m_durationMs;
        return m_buffer.data();
    }

    m_durationMs = m_elapsedTimer.elapsed();
    m_isRecording = false;
    m_tickTimer.stop();

#ifdef HOME_GUI_HAS_MULTIMEDIA
    if (m_audioSource) {
        if (m_audioIo) {
            const QByteArray lastChunk = m_audioIo->readAll();
            if (!lastChunk.isEmpty()) {
                m_buffer.write(lastChunk);
            }
        }
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_audioIo = nullptr;
    }
#elif defined(Q_OS_ANDROID)
    if (m_aaudioStream) {
        AAudioStream_requestStop(m_aaudioStream);
        AAudioStream_close(m_aaudioStream);
        m_aaudioStream = nullptr;
    }
    QByteArray capturedPcm;
    {
        QMutexLocker locker(&m_androidPcmMutex);
        capturedPcm = m_androidPcmBuffer;
        m_androidPcmBuffer.clear();
    }
    m_buffer.close();
    m_buffer.setData(capturedPcm);

    // 计算采样峰值与 RMS 并输出日志
    qint16 maxAmp = 0;
    double sumSq = 0.0;
    const int numSamples = capturedPcm.size() / 2;
    const qint16 *samples = reinterpret_cast<const qint16 *>(capturedPcm.constData());
    for (int i = 0; i < numSamples; ++i) {
        const qint16 s = std::abs(samples[i]);
        if (s > maxAmp) maxAmp = s;
        sumSq += static_cast<double>(s) * s;
    }
    const double rms = numSamples > 0 ? std::sqrt(sumSq / numSamples) : 0.0;
    qInfo() << "[AudioRecorder::stopRecording] Android PCM captured: bytes=" << capturedPcm.size()
            << "durationMs=" << m_durationMs
            << "peakAmp=" << maxAmp
            << "rms=" << rms;

    QFile debugFile(QStringLiteral("/sdcard/home_gui_last_speech.pcm"));
    if (debugFile.open(QIODevice::WriteOnly)) {
        debugFile.write(capturedPcm);
        debugFile.close();
    }
#elif defined(Q_OS_IOS)
    m_buffer.close();
    const QByteArray iosPcm = AudioRecorderIos::stopRecording();
    m_buffer.setData(iosPcm);
    m_audioLevel = 0.0;
    emit audioLevelChanged(0.0);
    emit recordingStopped(m_durationMs);
    if (outDurationMs) {
        *outDurationMs = m_durationMs;
    }
    return iosPcm;
#else
    // 降级模式：若无多媒体模块且非 Android，生成静音/测试 16kHz PCM
    if (m_buffer.size() == 0) {
        const int samples = static_cast<int>(m_durationMs * 16);
        QByteArray mockPcm(samples * 2, 0);
        m_buffer.write(mockPcm);
    }
#endif

    m_audioLevel = 0.0;
    emit audioLevelChanged(0.0);
    emit recordingStopped(m_durationMs);

    if (outDurationMs) {
        *outDurationMs = m_durationMs;
    }

    return m_buffer.data();
}

void AudioRecorder::cancelRecording()
{
    if (!m_isRecording) {
        return;
    }

    m_isRecording = false;
    m_tickTimer.stop();

#ifdef HOME_GUI_HAS_MULTIMEDIA
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_audioIo = nullptr;
    }
#elif defined(Q_OS_ANDROID)
    if (m_aaudioStream) {
        AAudioStream_requestStop(m_aaudioStream);
        AAudioStream_close(m_aaudioStream);
        m_aaudioStream = nullptr;
    }
    {
        QMutexLocker locker(&m_androidPcmMutex);
        m_androidPcmBuffer.clear();
    }
#elif defined(Q_OS_IOS)
    AudioRecorderIos::cancelRecording();
#endif

    m_buffer.close();
    m_buffer.setData(QByteArray());
    m_audioLevel = 0.0;
    emit audioLevelChanged(0.0);
    emit recordingCanceled();
}

