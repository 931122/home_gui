#ifndef AUDIORECORDER_IOS_H
#define AUDIORECORDER_IOS_H

#include <QByteArray>
#include <QtGlobal>

class AudioRecorder;

namespace AudioRecorderIos {
    void requestPermissionIfNeeded();
    bool startRecording(AudioRecorder *recorder);
    QByteArray stopRecording();
    void cancelRecording();
    qreal currentAudioLevel();
}

#endif // AUDIORECORDER_IOS_H
