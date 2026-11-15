#import "audiorecorder_ios.h"
#import "audiorecorder.h"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <QDebug>
#include <cmath>

namespace {

static AVAudioRecorder *sAudioRecorder = nil;
static NSString *sPcmFilePath = nil;
static AudioRecorder *sActiveRecorder = nullptr;

static NSString *getRecordFilePath()
{
    if (!sPcmFilePath) {
        NSString *tempDir = NSTemporaryDirectory();
        sPcmFilePath = [[tempDir stringByAppendingPathComponent:@"home_gui_intercom.wav"] copy];
    }
    return sPcmFilePath;
}

} // namespace

namespace AudioRecorderIos {

void requestPermissionIfNeeded()
{
    @autoreleasepool {
        AVAudioSession *session = [AVAudioSession sharedInstance];
        if ([session respondsToSelector:@selector(recordPermission)]) {
            if ([session recordPermission] == AVAudioSessionRecordPermissionUndetermined) {
                [session requestRecordPermission:^(BOOL granted) {
                    qInfo() << "[AudioRecorderIos] Pre-request mic permission result:" << granted;
                }];
            }
        }
    }
}

bool startRecording(AudioRecorder *recorder)
{
    if (!recorder) {
        return false;
    }

    @autoreleasepool {
        AVAudioSession *session = [AVAudioSession sharedInstance];
        NSError *err = nil;

        // Check permission if explicitly denied
        if ([session respondsToSelector:@selector(recordPermission)]) {
            AVAudioSessionRecordPermission perm = [session recordPermission];
            if (perm == AVAudioSessionRecordPermissionDenied) {
                qWarning() << "[AudioRecorderIos] Microphone permission denied";
                emit recorder->errorOccurred(QObject::tr("麦克风权限未授予，请在系统设置中允许使用麦克风"));
                return false;
            } else if (perm == AVAudioSessionRecordPermissionUndetermined) {
                [session requestRecordPermission:^(BOOL granted) {
                    qInfo() << "[AudioRecorderIos] Mic permission request result:" << granted;
                }];
                emit recorder->errorOccurred(QObject::tr("请授予麦克风权限后重试"));
                return false;
            }
        }

        // Configure audio session
        AVAudioSessionCategoryOptions opts = AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionAllowBluetooth;
        [session setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:opts error:&err];
        if (err) {
            qWarning() << "[AudioRecorderIos] setCategory failed:" << QString::fromNSString(err.localizedDescription);
            err = nil;
        }

        [session setActive:YES error:&err];
        if (err) {
            qWarning() << "[AudioRecorderIos] setActive failed:" << QString::fromNSString(err.localizedDescription);
            emit recorder->errorOccurred(QObject::tr("初始化音频会话失败: %1").arg(QString::fromNSString(err.localizedDescription)));
            return false;
        }

        // Stop any previous recording
        if (sAudioRecorder) {
            if ([sAudioRecorder isRecording]) {
                [sAudioRecorder stop];
            }
            sAudioRecorder = nil;
        }

        NSString *filePath = getRecordFilePath();
        [[NSFileManager defaultManager] removeItemAtPath:filePath error:nil];
        NSURL *url = [NSURL fileURLWithPath:filePath];

        NSDictionary *settings = @{
            AVFormatIDKey: @(kAudioFormatLinearPCM),
            AVSampleRateKey: @(16000.0),
            AVNumberOfChannelsKey: @(1),
            AVLinearPCMBitDepthKey: @(16),
            AVLinearPCMIsBigEndianKey: @NO,
            AVLinearPCMIsFloatKey: @NO,
            AVLinearPCMIsNonInterleaved: @NO
        };

        sAudioRecorder = [[AVAudioRecorder alloc] initWithURL:url settings:settings error:&err];
        if (err || !sAudioRecorder) {
            qWarning() << "[AudioRecorderIos] AVAudioRecorder init failed:" << (err ? QString::fromNSString(err.localizedDescription) : "unknown");
            emit recorder->errorOccurred(QObject::tr("创建录音器失败: %1").arg(err ? QString::fromNSString(err.localizedDescription) : QString()));
            return false;
        }

        [sAudioRecorder setMeteringEnabled:YES];
        if (![sAudioRecorder prepareToRecord]) {
            qWarning() << "[AudioRecorderIos] prepareToRecord failed";
        }

        if (![sAudioRecorder record]) {
            qWarning() << "[AudioRecorderIos] record failed";
            emit recorder->errorOccurred(QObject::tr("启动录音失败"));
            return false;
        }

        sActiveRecorder = recorder;
        qInfo() << "[AudioRecorderIos] AVAudioRecorder started recording to" << QString::fromNSString(filePath);
        return true;
    }
}

QByteArray stopRecording()
{
    @autoreleasepool {
        if (!sAudioRecorder) {
            return QByteArray();
        }

        if ([sAudioRecorder isRecording]) {
            [sAudioRecorder stop];
        }
        sAudioRecorder = nil;
        sActiveRecorder = nullptr;

        NSString *filePath = getRecordFilePath();
        NSData *fileData = [NSData dataWithContentsOfFile:filePath];

        AVAudioSession *session = [AVAudioSession sharedInstance];
        NSError *err = nil;
        [session setActive:NO withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:&err];

        if (!fileData || [fileData length] == 0) {
            qWarning() << "[AudioRecorderIos] Recorded file is empty or not found at" << QString::fromNSString(filePath);
            return QByteArray();
        }

        QByteArray wavBytes(reinterpret_cast<const char *>([fileData bytes]), static_cast<int>([fileData length]));
        qInfo() << "[AudioRecorderIos] Total recorded WAV bytes:" << wavBytes.size();

        // Extract raw PCM: search for the 'data' chunk in WAV header
        const int dataIdx = wavBytes.indexOf("data");
        QByteArray pcmData;
        if (dataIdx >= 0 && dataIdx + 8 < wavBytes.size()) {
            pcmData = wavBytes.mid(dataIdx + 8);
        } else if (wavBytes.size() > 44) {
            pcmData = wavBytes.mid(44);
        } else {
            pcmData = wavBytes;
        }

        qInfo() << "[AudioRecorderIos] Extracted raw 16kHz mono PCM bytes:" << pcmData.size();
        return pcmData;
    }
}

void cancelRecording()
{
    @autoreleasepool {
        if (sAudioRecorder) {
            if ([sAudioRecorder isRecording]) {
                [sAudioRecorder stop];
            }
            sAudioRecorder = nil;
        }
        sActiveRecorder = nullptr;

        NSString *filePath = getRecordFilePath();
        [[NSFileManager defaultManager] removeItemAtPath:filePath error:nil];

        AVAudioSession *session = [AVAudioSession sharedInstance];
        NSError *err = nil;
        [session setActive:NO withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:&err];
    }
}

qreal currentAudioLevel()
{
    @autoreleasepool {
        if (!sAudioRecorder || ![sAudioRecorder isRecording]) {
            return 0.0;
        }
        [sAudioRecorder updateMeters];
        float power = [sAudioRecorder averagePowerForChannel:0];
        // Convert dB (-60 dB to 0 dB) to 0.0 - 1.0 linear level
        if (power < -60.0f) {
            return 0.0;
        }
        if (power >= 0.0f) {
            return 1.0;
        }
        return static_cast<qreal>((power + 60.0f) / 60.0f);
    }
}

} // namespace AudioRecorderIos
