#include "agoramediabridge_ios.h"
#include "embeddedvoipbackend.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>

#if defined(HOME_GUI_HAS_AGORA_IOS)

#import <AVFoundation/AVFoundation.h>
#import <AgoraRtcKit/AgoraRtcEngineKit.h>
#import <Foundation/Foundation.h>

@interface HomeGuiAgoraDelegate : NSObject <AgoraRtcEngineDelegate>
@end

static NSString *const sAgoraLock = @"HomeGuiAgoraBridgeLock";
static AgoraRtcEngineKit *sRtcEngine = nil;
static HomeGuiAgoraDelegate *sDelegate = nil;
static NSString *sCurrentAppId = nil;
static BOOL sSpeakerphone = YES;
static BOOL sMuted = NO;
static BOOL sInChannel = NO;

static void configureIosAudioSession(BOOL inCall, BOOL speakerphone)
{
    AVAudioSession *session = [AVAudioSession sharedInstance];
    NSError *err = nil;
    if (inCall) {
        AVAudioSessionCategoryOptions opts = AVAudioSessionCategoryOptionAllowBluetooth;
        if (speakerphone) {
            opts |= AVAudioSessionCategoryOptionDefaultToSpeaker;
        }
        [session setCategory:AVAudioSessionCategoryPlayAndRecord
                        mode:AVAudioSessionModeVoiceChat
                     options:opts
                       error:&err];
        if (err) {
            qWarning() << "[AgoraMediaBridgeIos] setCategory error:"
                       << QString::fromNSString(err.localizedDescription);
            err = nil;
        }
        [session setActive:YES error:&err];
        if (err) {
            qWarning() << "[AgoraMediaBridgeIos] setActive(YES) error:"
                       << QString::fromNSString(err.localizedDescription);
            err = nil;
        }
        [session overrideOutputAudioPort:(speakerphone ? AVAudioSessionPortOverrideSpeaker
                                                       : AVAudioSessionPortOverrideNone)
                                   error:&err];
    } else {
        [session overrideOutputAudioPort:AVAudioSessionPortOverrideNone error:&err];
        err = nil;
        [session setActive:NO
               withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                     error:&err];
    }
}

static void requestRecordPermissionIfNeeded()
{
    AVAudioSession *session = [AVAudioSession sharedInstance];
    if ([session recordPermission] == AVAudioSessionRecordPermissionUndetermined) {
        [session requestRecordPermission:^(BOOL granted) {
            qInfo() << "[AgoraMediaBridgeIos] Microphone permission granted:" << granted;
        }];
    }
}

@implementation HomeGuiAgoraDelegate

- (void)rtcEngine:(AgoraRtcEngineKit *)engine didJoinChannel:(NSString *)channel withUid:(NSUInteger)uid elapsed:(NSInteger)elapsed
{
    Q_UNUSED(elapsed);
    const quint32 localUid = static_cast<quint32>(uid);
    const QString ch = channel ? QString::fromNSString(channel) : QString();
    qInfo() << "[AgoraMediaBridgeIos] didJoinChannel:" << ch << "uid=" << localUid;

    BOOL spk = YES;
    BOOL mute = NO;
    @synchronized(sAgoraLock) {
        sInChannel = YES;
        spk = sSpeakerphone;
        mute = sMuted;
    }
    if (engine) {
        [engine setEnableSpeakerphone:spk];
        [engine muteLocalAudioStream:mute];
    }

    if (!qApp) return;
    QMetaObject::invokeMethod(qApp, [ch, localUid]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraJoined(ch, localUid);
        }
    }, Qt::QueuedConnection);
}

- (void)rtcEngine:(AgoraRtcEngineKit *)engine didJoinedOfUid:(NSUInteger)uid elapsed:(NSInteger)elapsed
{
    Q_UNUSED(engine);
    Q_UNUSED(elapsed);
    const quint32 remoteUid = static_cast<quint32>(uid);
    qInfo() << "[AgoraMediaBridgeIos] didJoinedOfUid:" << remoteUid;
    if (!qApp) return;
    QMetaObject::invokeMethod(qApp, [remoteUid]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraUserJoined(remoteUid);
        }
    }, Qt::QueuedConnection);
}

- (void)rtcEngine:(AgoraRtcEngineKit *)engine didOfflineOfUid:(NSUInteger)uid reason:(AgoraUserOfflineReason)reason
{
    Q_UNUSED(engine);
    const quint32 remoteUid = static_cast<quint32>(uid);
    const int reasonCode = static_cast<int>(reason);
    qInfo() << "[AgoraMediaBridgeIos] didOfflineOfUid:" << remoteUid << "reason=" << reasonCode;
    if (!qApp) return;
    QMetaObject::invokeMethod(qApp, [remoteUid, reasonCode]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraUserOffline(remoteUid, reasonCode);
        }
    }, Qt::QueuedConnection);
}

- (void)rtcEngine:(AgoraRtcEngineKit *)engine didLeaveChannelWithStats:(AgoraChannelStats *)stats
{
    Q_UNUSED(engine);
    Q_UNUSED(stats);
    qInfo() << "[AgoraMediaBridgeIos] didLeaveChannel";
    @synchronized(sAgoraLock) {
        sInChannel = NO;
    }
}

- (void)rtcEngine:(AgoraRtcEngineKit *)engine didOccurError:(AgoraErrorCode)errorCode
{
    Q_UNUSED(engine);
    const int code = static_cast<int>(errorCode);
    qWarning() << "[AgoraMediaBridgeIos] didOccurError:" << code;
    if (!qApp) return;
    QMetaObject::invokeMethod(qApp, [code]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraError(code);
        }
    }, Qt::QueuedConnection);
}

- (void)rtcEngine:(AgoraRtcEngineKit *)engine connectionChangedToState:(AgoraConnectionStateType)state reason:(AgoraConnectionChangedReason)reason
{
    Q_UNUSED(engine);
    qInfo() << "[AgoraMediaBridgeIos] connectionChanged state=" << static_cast<int>(state)
            << "reason=" << static_cast<int>(reason);
}

- (void)rtcEngine:(AgoraRtcEngineKit *)engine didAudioRouteChanged:(AgoraAudioOutputRouting)routing
{
    Q_UNUSED(engine);
    const bool isSpeaker = (routing == AgoraAudioOutputRoutingSpeakerphone ||
                            routing == AgoraAudioOutputRoutingLoudspeaker);
    qInfo() << "[AgoraMediaBridgeIos] didAudioRouteChanged routing=" << static_cast<int>(routing)
            << "isSpeaker=" << isSpeaker;
    if (!qApp) return;
    QMetaObject::invokeMethod(qApp, [isSpeaker]() {
        if (EmbeddedVoipBackend *b = EmbeddedVoipBackend::instance()) {
            b->notifyAgoraAudioRoute(isSpeaker);
        }
    }, Qt::QueuedConnection);
}

@end

namespace AgoraMediaBridgeIos {

void init()
{
    @autoreleasepool {
        requestRecordPermissionIfNeeded();
    }
}

int joinChannel(const QString &appId,
                const QString &token,
                const QString &channelId,
                quint32 uid,
                bool speakerphone,
                bool muted)
{
    if (appId.isEmpty() || channelId.isEmpty()) {
        qWarning() << "[AgoraMediaBridgeIos] Invalid joinChannel arguments";
        return -1;
    }

    @autoreleasepool {
        @synchronized(sAgoraLock) {
            requestRecordPermissionIfNeeded();
            sSpeakerphone = speakerphone ? YES : NO;
            sMuted = muted ? YES : NO;

            NSString *nsAppId = appId.toNSString();
            NSString *nsToken = token.isEmpty() ? @"" : token.toNSString();
            NSString *nsChannel = channelId.toNSString();

            if (sRtcEngine != nil && sCurrentAppId != nil && ![sCurrentAppId isEqualToString:nsAppId]) {
                if (sInChannel) {
                    [sRtcEngine leaveChannel:nil];
                    sInChannel = NO;
                }
                [AgoraRtcEngineKit destroy];
                sRtcEngine = nil;
#if !__has_feature(objc_arc)
                [sCurrentAppId release];
#endif
                sCurrentAppId = nil;
            }

            if (sDelegate == nil) {
                sDelegate = [[HomeGuiAgoraDelegate alloc] init];
            }

            if (sRtcEngine == nil) {
                qInfo() << "[AgoraMediaBridgeIos] Creating AgoraRtcEngineKit sdk="
                        << QString::fromNSString([AgoraRtcEngineKit getSdkVersion]);
                sRtcEngine = [AgoraRtcEngineKit sharedEngineWithAppId:nsAppId delegate:sDelegate];
#if !__has_feature(objc_arc)
                [sRtcEngine retain];
                sCurrentAppId = [nsAppId copy];
#else
                sCurrentAppId = [nsAppId copy];
#endif
            }

            if (sInChannel) {
                qWarning() << "[AgoraMediaBridgeIos] Previous Agora channel still active, leaving before rejoin";
                [sRtcEngine leaveChannel:nil];
                sInChannel = NO;
            }

            configureIosAudioSession(YES, sSpeakerphone);

            [sRtcEngine setChannelProfile:AgoraChannelProfileLiveBroadcasting];
            [sRtcEngine setClientRole:AgoraClientRoleBroadcaster];
            [sRtcEngine setAudioProfile:AgoraAudioProfileDefault scenario:AgoraAudioScenarioDefault];
            [sRtcEngine enableAudio];
            [sRtcEngine setDefaultAudioRouteToSpeakerphone:sSpeakerphone];
            [sRtcEngine setEnableSpeakerphone:sSpeakerphone];
            [sRtcEngine muteLocalAudioStream:sMuted];

            int rc = [sRtcEngine joinChannelByToken:nsToken
                                          channelId:nsChannel
                                               info:nil
                                                uid:static_cast<NSUInteger>(uid)
                                        joinSuccess:nil];
            if (rc == -17) {
                qWarning() << "[AgoraMediaBridgeIos] joinChannel rc=-17, forcing leaveChannel and retrying";
                [sRtcEngine leaveChannel:nil];
                [NSThread sleepForTimeInterval:0.08];
                rc = [sRtcEngine joinChannelByToken:nsToken
                                          channelId:nsChannel
                                               info:nil
                                                uid:static_cast<NSUInteger>(uid)
                                        joinSuccess:nil];
            }
            if (rc == 0) {
                sInChannel = YES;
            }
            qInfo() << "[AgoraMediaBridgeIos] joinChannel rc=" << rc << "channel=" << channelId << "uid=" << uid;
            return rc;
        }
    }
}

void leaveChannel()
{
    @autoreleasepool {
        @synchronized(sAgoraLock) {
            sInChannel = NO;
            if (sRtcEngine != nil) {
                const int rc = [sRtcEngine leaveChannel:nil];
                qInfo() << "[AgoraMediaBridgeIos] Leaving Agora channel rc=" << rc;
            }
            configureIosAudioSession(NO, NO);
        }
    }
}

void setMute(bool muted)
{
    @autoreleasepool {
        @synchronized(sAgoraLock) {
            sMuted = muted ? YES : NO;
            if (sRtcEngine != nil) {
                [sRtcEngine muteLocalAudioStream:sMuted];
            }
        }
    }
}

void setSpeakerphone(bool enable)
{
    @autoreleasepool {
        @synchronized(sAgoraLock) {
            sSpeakerphone = enable ? YES : NO;
            if (sRtcEngine != nil) {
                [sRtcEngine setEnableSpeakerphone:sSpeakerphone];
            }
            configureIosAudioSession(YES, sSpeakerphone);
        }
    }
}

} // namespace AgoraMediaBridgeIos

#else // !HOME_GUI_HAS_AGORA_IOS

namespace AgoraMediaBridgeIos {

void init() {}

int joinChannel(const QString &, const QString &, const QString &channelId, quint32, bool, bool)
{
    qWarning() << "[AgoraMediaBridgeIos] Agora iOS SDK not linked; cannot join audio channel" << channelId;
    return -1;
}

void leaveChannel() {}
void setMute(bool) {}
void setSpeakerphone(bool) {}

} // namespace AgoraMediaBridgeIos

#endif // HOME_GUI_HAS_AGORA_IOS
