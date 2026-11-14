package org.qtproject.example.home_gui;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import io.agora.rtc.Constants;
import io.agora.rtc.IRtcEngineEventHandler;
import io.agora.rtc.RtcEngine;

/**
 * Pure Agora RTC audio bridge used ONLY by EmbeddedVoipBackend (C++ MiLink v4 signaling).
 * Does not touch Xiaomi VoIP SDK (u6.v / MiLink Java).
 */
public final class AgoraMediaBridge {
    private static final String TAG = "AgoraMediaBridge";
    private static final int REQ_RECORD_AUDIO = 31042;

    private static Context sAppContext;
    private static RtcEngine sRtcEngine;
    private static String sCurrentAppId;
    private static boolean sSpeakerphone = true;
    private static boolean sMuted = false;
    private static boolean sInChannel = false;
    private static final Handler sMainHandler = new Handler(Looper.getMainLooper());

    public static native void nativeOnAgoraJoined(String channel, int uid);
    public static native void nativeOnAgoraUserJoined(int uid);
    public static native void nativeOnAgoraUserOffline(int uid, int reason);
    public static native void nativeOnAgoraError(int errCode);
    public static native void nativeOnAgoraAudioRoute(boolean isSpeaker);

    private static final IRtcEngineEventHandler sEventHandler = new IRtcEngineEventHandler() {
        @Override
        public void onJoinChannelSuccess(String channel, int uid, int elapsed) {
            Log.i(TAG, "onJoinChannelSuccess: channel=" + channel + " uid=" + (uid & 0xFFFFFFFFL));
            try {
                if (sRtcEngine != null) {
                    sRtcEngine.setEnableSpeakerphone(sSpeakerphone);
                    sRtcEngine.muteLocalAudioStream(sMuted);
                }
            } catch (Throwable t) {
                Log.w(TAG, "Post-join audio config note: " + t.getMessage());
            }
            nativeOnAgoraJoined(channel != null ? channel : "", uid);
        }

        @Override
        public void onUserJoined(int uid, int elapsed) {
            Log.i(TAG, "onUserJoined: uid=" + (uid & 0xFFFFFFFFL));
            nativeOnAgoraUserJoined(uid);
        }

        @Override
        public void onUserOffline(int uid, int reason) {
            Log.i(TAG, "onUserOffline: uid=" + (uid & 0xFFFFFFFFL) + " reason=" + reason);
            nativeOnAgoraUserOffline(uid, reason);
        }

        @Override
        public void onLeaveChannel(RtcStats stats) {
            Log.i(TAG, "onLeaveChannel");
            sInChannel = false;
        }

        @Override
        public void onError(int err) {
            Log.e(TAG, "onError: err=" + err);
            nativeOnAgoraError(err);
        }

        @Override
        public void onConnectionStateChanged(int state, int reason) {
            Log.i(TAG, "onConnectionStateChanged: state=" + state + " reason=" + reason);
        }

        @Override
        public void onAudioRouteChanged(int routing) {
            boolean isSpeaker = (routing == Constants.AUDIO_ROUTE_SPEAKERPHONE);
            Log.i(TAG, "onAudioRouteChanged: routing=" + routing + " isSpeaker=" + isSpeaker);
            nativeOnAgoraAudioRoute(isSpeaker);
        }
    };

    private AgoraMediaBridge() {}

    public static synchronized void init(Context context) {
        if (context == null) {
            return;
        }
        sAppContext = context.getApplicationContext();
        ensureAudioPermission(context);
    }

    public static void ensureAudioPermission(Context context) {
        if (context == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return;
        }
        if (context.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) {
            return;
        }
        if (context instanceof Activity) {
            final Activity activity = (Activity) context;
            sMainHandler.post(() -> {
                try {
                    activity.requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, REQ_RECORD_AUDIO);
                } catch (Throwable t) {
                    Log.w(TAG, "requestPermissions failed: " + t.getMessage());
                }
            });
        } else {
            Log.w(TAG, "RECORD_AUDIO not granted and context is not Activity.");
        }
    }

    public static synchronized int joinChannel(Context context,
                                               String appId,
                                               String token,
                                               String channelId,
                                               long uid,
                                               boolean speakerphone,
                                               boolean muted) {
        if (context != null && sAppContext == null) {
            sAppContext = context.getApplicationContext();
        }
        if (context != null) {
            ensureAudioPermission(context);
        }
        if (sAppContext == null || appId == null || appId.isEmpty() || channelId == null || channelId.isEmpty()) {
            Log.e(TAG, "Invalid joinChannel args");
            return -1;
        }

        sSpeakerphone = speakerphone;
        sMuted = muted;

        try {
            if (sRtcEngine != null && !appId.equals(sCurrentAppId)) {
                try {
                    sRtcEngine.leaveChannel();
                } catch (Throwable ignored) {}
                RtcEngine.destroy();
                sRtcEngine = null;
                sCurrentAppId = null;
            }

            if (sRtcEngine == null) {
                Log.i(TAG, "Creating pure Agora RtcEngine sdk=" + RtcEngine.getSdkVersion());
                sRtcEngine = RtcEngine.create(sAppContext, appId, sEventHandler);
                sCurrentAppId = appId;
            }

            if (sRtcEngine != null && sInChannel) {
                Log.w(TAG, "Previous Agora channel still active, leaving before rejoin");
                try {
                    sRtcEngine.leaveChannel();
                } catch (Throwable ignored) {}
                sInChannel = false;
            }

            configureAudioMode(true, speakerphone);

            sRtcEngine.setChannelProfile(Constants.CHANNEL_PROFILE_LIVE_BROADCASTING);
            sRtcEngine.setClientRole(Constants.CLIENT_ROLE_BROADCASTER);
            sRtcEngine.setAudioProfile(Constants.AUDIO_PROFILE_DEFAULT, Constants.AUDIO_SCENARIO_DEFAULT);
            sRtcEngine.enableAudio();
            sRtcEngine.setDefaultAudioRoutetoSpeakerphone(speakerphone);
            sRtcEngine.setEnableSpeakerphone(speakerphone);
            sRtcEngine.muteLocalAudioStream(muted);

            String effectiveToken = token != null ? token : "";
            int rc = sRtcEngine.joinChannel(effectiveToken, channelId, "", (int) uid);
            if (rc == -17) {
                Log.w(TAG, "joinChannel rc=-17 (already in channel), forcing leaveChannel and retrying");
                try {
                    sRtcEngine.leaveChannel();
                    Thread.sleep(80);
                } catch (Throwable ignored) {}
                rc = sRtcEngine.joinChannel(effectiveToken, channelId, "", (int) uid);
            }
            if (rc == 0) {
                sInChannel = true;
            }
            Log.i(TAG, "joinChannel rc=" + rc + " channel=" + channelId + " uid=" + (uid & 0xFFFFFFFFL));
            return rc;
        } catch (Throwable t) {
            Log.e(TAG, "joinChannel exception: " + t.getMessage(), t);
            return -2;
        }
    }

    public static synchronized void leaveChannel() {
        sInChannel = false;
        try {
            if (sRtcEngine != null) {
                int rc = sRtcEngine.leaveChannel();
                Log.i(TAG, "Leaving Agora channel rc=" + rc);
            }
        } catch (Throwable t) {
            Log.w(TAG, "leaveChannel error: " + t.getMessage());
        } finally {
            configureAudioMode(false, false);
        }
    }

    public static synchronized void setMute(boolean muted) {
        sMuted = muted;
        try {
            if (sRtcEngine != null) {
                sRtcEngine.muteLocalAudioStream(muted);
            }
        } catch (Throwable t) {
            Log.w(TAG, "setMute error: " + t.getMessage());
        }
    }

    public static synchronized void setSpeakerphone(boolean enable) {
        sSpeakerphone = enable;
        try {
            if (sRtcEngine != null) {
                sRtcEngine.setEnableSpeakerphone(enable);
            }
            configureAudioMode(true, enable);
        } catch (Throwable t) {
            Log.w(TAG, "setSpeakerphone error: " + t.getMessage());
        }
    }

    private static void configureAudioMode(boolean inCall, boolean speakerphone) {
        if (sAppContext == null) {
            return;
        }
        try {
            AudioManager am = (AudioManager) sAppContext.getSystemService(Context.AUDIO_SERVICE);
            if (am != null) {
                if (inCall) {
                    am.setMode(AudioManager.MODE_IN_COMMUNICATION);
                    am.setSpeakerphoneOn(speakerphone);
                } else {
                    am.setSpeakerphoneOn(false);
                    am.setMode(AudioManager.MODE_NORMAL);
                }
            }
        } catch (Throwable t) {
            Log.w(TAG, "configureAudioMode note: " + t.getMessage());
        }
    }
}
