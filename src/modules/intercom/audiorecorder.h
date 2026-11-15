#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QByteArray>
#include <QElapsedTimer>
#include <QTimer>
#include <QBuffer>
#include <QMutex>

#ifdef HOME_GUI_HAS_MULTIMEDIA
#include <QAudioFormat>
#include <QAudioSource>
#include <QMediaDevices>
#endif

#if defined(Q_OS_ANDROID)
struct AAudioStreamStruct;
typedef struct AAudioStreamStruct AAudioStream;
#endif

class AudioRecorder : public QObject
{
    Q_OBJECT

public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder() override;

    bool isRecording() const;
    qint64 durationMs() const;
    qreal audioLevel() const;

public slots:
    bool startRecording();
    QByteArray stopRecording(qint64 *outDurationMs = nullptr);
    void cancelRecording();
    void handleAudioChunk(const QByteArray &chunk);

#if defined(Q_OS_ANDROID)
    void handleAndroidAudioFrame(const char *data, int len);
#endif

signals:
    void recordingStarted();
    void recordingStopped(qint64 durationMs);
    void recordingCanceled();
    void durationUpdated(int durationSeconds);
    void audioLevelChanged(qreal level);
    void errorOccurred(const QString &errorMsg);

private slots:
    void handleTimerTick();
    void handleAudioDataReady();

private:
    void calculateAudioLevel(const QByteArray &chunk);

    bool m_isRecording = false;
    QElapsedTimer m_elapsedTimer;
    QTimer m_tickTimer;
    qreal m_audioLevel = 0.0;
    qint64 m_durationMs = 0;

    QBuffer m_buffer;
#ifdef HOME_GUI_HAS_MULTIMEDIA
    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_audioIo = nullptr;
#endif

#if defined(Q_OS_ANDROID)
    AAudioStream *m_aaudioStream = nullptr;
    QMutex m_androidPcmMutex;
    QByteArray m_androidPcmBuffer;
#endif
};

#endif // AUDIORECORDER_H
