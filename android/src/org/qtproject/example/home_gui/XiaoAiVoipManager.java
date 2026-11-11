package org.qtproject.example.home_gui;

import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.FrameLayout;

import dalvik.system.PathClassLoader;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

public class XiaoAiVoipManager {
    private static final String TAG = "XiaoAiVoipManager";

    public static final int STATE_IDLE = 0;
    public static final int STATE_CALLING = 1;
    public static final int STATE_RINGING = 2;
    public static final int STATE_CONNECTED = 3;
    public static final int STATE_HANGUP = 4;
    public static final int STATE_ERROR = 5;

    public static class MicoAppWrapper extends Application {
        public MicoAppWrapper(Context base) {
            attachBaseContext(base);
        }

        @Override
        public String getPackageName() {
            return "com.xiaomi.mico";
        }

        @Override
        public Context getApplicationContext() {
            return this;
        }
    }

    private static XiaoAiVoipManager sInstance;

    private Context mContext;
    private PathClassLoader mLoader;
    private String mUserId;
    private String mServiceToken;
    private String mMimcId;

    // Xiaomi VoIP SDK core reflection objects
    private Class<?> mVoipClass;
    private Object mVoipInstance;
    private Class<?> mJ6cClass;
    private Class<?> mJ6iClass;
    private Class<?> mJ6hClass;
    private Object mAccountManager;
    private Object mOperator; // K6.B SimpleOperator
    private Class<?> mOperatorClass;
    private Object mCallListenerProxy;
    private Object mVideoLayoutProxy;
    private FrameLayout mDummyFrameLayout;

    private volatile boolean mIsInitialized = false;
    private volatile boolean mIsLoggedIn = false;
    private volatile int mState = STATE_IDLE;

    private String mTargetDeviceId;
    private String mTargetDeviceName;
    private long mTargetUid = 0;
    private boolean mMuted = false;
    private boolean mSpeakerphone = true;
    private int mCallDurationSec = 0;

    private final Handler mMainHandler = new Handler(Looper.getMainLooper());
    private MediaPlayer mMediaPlayer;
    private Runnable mDurationRunnable;

    // JNI Native Callbacks to Qt / C++
    public static native void nativeOnVoipStateChanged(int state, String statusText);
    public static native void nativeOnVoipDuration(int durationSec);
    public static native void nativeOnVoipAudioRoute(boolean isSpeaker);

    public static synchronized XiaoAiVoipManager getInstance() {
        if (sInstance == null) {
            sInstance = new XiaoAiVoipManager();
        }
        return sInstance;
    }

    public static void initVoip(Context context, String userId, String serviceToken, String mimcId) {
        getInstance().init(context, userId, serviceToken, mimcId);
    }

    public static void startCall(String deviceId, String deviceName, String uid, String deviceModel, int deviceType) {
        getInstance().call(deviceId, deviceName, uid, deviceModel, deviceType);
    }

    public static void startCall(String deviceId, String deviceName) {
        getInstance().call(deviceId, deviceName, null, null, 0);
    }

    public static void hangupCall() {
        getInstance().hangup();
    }

    public static void setMute(boolean mute) {
        getInstance().mute(mute);
    }

    public static void setSpeakerphone(boolean enable) {
        getInstance().speakerphone(enable);
    }

    public static boolean isVoipReady() {
        return getInstance().isReady();
    }

    public static int getVoipState() {
        return getInstance().mState;
    }

    public boolean isReady() {
        return mIsInitialized && mOperator != null;
    }

    public synchronized void init(Context context, String userId, String serviceToken, String mimcId) {
        mContext = context.getApplicationContext();
        mUserId = userId;
        mServiceToken = serviceToken;
        mMimcId = mimcId;

        try {
            IntentFilter filter = new IntentFilter();
            filter.addAction("org.qtproject.example.home_gui.ACTION_VOIP_CALL");
            filter.addAction("org.qtproject.example.home_gui.ACTION_VOIP_HANGUP");
            mContext.registerReceiver(new BroadcastReceiver() {
                @Override
                public void onReceive(Context ctx, Intent intent) {
                    String action = intent.getAction();
                    if ("org.qtproject.example.home_gui.ACTION_VOIP_CALL".equals(action)) {
                        String uid = intent.getStringExtra("uid");
                        String name = intent.getStringExtra("name");
                        String did = intent.getStringExtra("deviceId");
                        String model = intent.getStringExtra("model");
                        Log.i(TAG, "Received ACTION_VOIP_CALL broadcast: uid=" + uid + " name=" + name);
                        startCall(did, name, uid, model, 0);
                    } else if ("org.qtproject.example.home_gui.ACTION_VOIP_HANGUP".equals(action)) {
                        Log.i(TAG, "Received ACTION_VOIP_HANGUP broadcast");
                        hangupCall();
                    }
                }
            }, filter);
        } catch (Throwable t) {
            Log.w(TAG, "Failed registering debug broadcast receiver: " + t.getMessage());
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Log.i(TAG, "Initializing Xiaomi VoIP runtime for userId=" + mUserId);
                    ApplicationInfo micoInfo = mContext.getPackageManager().getApplicationInfo("com.xiaomi.mico", 0);
                    String apkPath = micoInfo.sourceDir;
                    String libPath = micoInfo.nativeLibraryDir;
                    Log.i(TAG, "Mico APK: " + apkPath + ", libs: " + libPath);

                    mLoader = new PathClassLoader(apkPath, libPath, mContext.getClassLoader());

                    // 1. Initialize XLog if available
                    try {
                        Class<?> xlogClass = mLoader.loadClass("y2.e");
                        Method initXLog = xlogClass.getMethod("m");
                        initXLog.invoke(null);
                        Log.i(TAG, "Initialized XLog via y2.e.m()");
                    } catch (Throwable t) {
                        Log.w(TAG, "XLog init note: " + t.getMessage());
                    }

                    // 2. Load u6.v singleton
                    mVoipClass = mLoader.loadClass("u6.v");
                    mVoipInstance = mVoipClass.getMethod("i").invoke(null);
                    Log.i(TAG, "Loaded u6.v singleton: " + mVoipInstance);

                    // 3. Set p = false (in-process mode, bypass milink_ipc service)
                    try {
                        Field pField = mVoipClass.getDeclaredField("p");
                        pField.setAccessible(true);
                        pField.setBoolean(mVoipInstance, false);
                        Log.i(TAG, "Set u6.v.p = false (in-process mode)");
                    } catch (Throwable t) {
                        Log.w(TAG, "Failed setting p field directly: " + t.getMessage());
                    }

                    // 4. Load interface classes
                    mJ6cClass = mLoader.loadClass("J6.c");
                    mJ6iClass = mLoader.loadClass("J6.i");
                    try {
                        mJ6hClass = mLoader.loadClass("J6.h");
                        Log.i(TAG, "Loaded J6.h interface class");
                    } catch (Throwable t) {
                        Log.w(TAG, "Failed loading J6.h: " + t.getMessage());
                    }

                    // 5. Register Call Listener (J6.i)
                    mCallListenerProxy = Proxy.newProxyInstance(
                        mLoader,
                        new Class<?>[]{ mJ6iClass },
                        new InvocationHandler() {
                            @Override
                            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                                handleVoipEvent(method.getName(), args);
                                return null;
                            }
                        }
                    );
                    Method setListenerMethod = mVoipClass.getMethod("w", mJ6iClass);
                    setListenerMethod.invoke(mVoipInstance, mCallListenerProxy);
                    Log.i(TAG, "Registered J6.i listener proxy via u6.v.w()");

                    // 6. Initialize VoIP SDK via v.q(...)
                    File logDir = new File(mContext.getExternalFilesDir(null), "voip_sdk");
                    logDir.mkdirs();
                    Method qMethod = mVoipClass.getMethod("q", Application.class, int.class, String.class, String.class);
                    Application baseApp = (Application) (mContext instanceof Application ? mContext : mContext.getApplicationContext());
                    MicoAppWrapper micoApp = new MicoAppWrapper(baseApp);
                    qMethod.invoke(mVoipInstance, micoApp, 10018, "XiaoMiAiSpeaker", logDir.getAbsolutePath());
                    Log.i(TAG, "u6.v.q() called successfully with MicoAppWrapper (pkg=com.xiaomi.mico)");

                    // 7. Register Init Callback (J6.c)
                    Object initCallbackProxy = Proxy.newProxyInstance(
                        mLoader,
                        new Class<?>[]{ mJ6cClass },
                        new InvocationHandler() {
                            @Override
                            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                                Log.i(TAG, "VoIP SDK init callback completed");
                                return null;
                            }
                        }
                    );
                    mVoipClass.getMethod("p", mJ6cClass).invoke(mVoipInstance, initCallbackProxy);

                    // 8. Get AccountManager and Login to MiLink
                    mAccountManager = mVoipClass.getMethod("c").invoke(mVoipInstance);
                    Method bMethod = mAccountManager.getClass().getMethod(
                        "b", String.class, long.class, String.class, mJ6cClass, mJ6cClass
                    );

                    Object onLoginDoneProxy = Proxy.newProxyInstance(
                        mLoader,
                        new Class<?>[]{ mJ6cClass },
                        new InvocationHandler() {
                            @Override
                            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                                int code = -1;
                                if (args != null && args.length > 0 && args[0] instanceof Number) {
                                    code = ((Number) args[0]).intValue();
                                }
                                Log.i(TAG, "VoIP AccountManager onLoginDone code=" + code);
                                if (code == 1 || code == 2) {
                                    mIsLoggedIn = true;
                                    Log.i(TAG, "VoIP MiLink Login SUCCESS! Ready to make/receive calls.");
                                } else {
                                    Log.w(TAG, "VoIP MiLink Login failed with code=" + code);
                                }
                                return null;
                            }
                        }
                    );

                    Object onSyncDoneProxy = Proxy.newProxyInstance(
                        mLoader,
                        new Class<?>[]{ mJ6cClass },
                        new InvocationHandler() {
                            @Override
                            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                                Log.i(TAG, "VoIP AccountManager onSyncDone result=" + (args != null && args.length > 0 ? args[0] : null));
                                return null;
                            }
                        }
                    );

                    long uidNum = Long.parseLong(mUserId);
                    Log.i(TAG, "Logging in to MiLink for uid=" + uidNum + "...");
                    bMethod.invoke(mAccountManager, "micoapi", uidNum, mServiceToken, onLoginDoneProxy, onSyncDoneProxy);

                    // 9. Get SimpleOperator
                    mOperator = mVoipClass.getMethod("n").invoke(mVoipInstance);
                    mOperatorClass = mOperator.getClass();
                    Log.i(TAG, "Got SimpleOperator: " + mOperator);

                    // 10. Register VideoLayoutManager (J6.h) with SimpleOperator
                    setupVideoLayoutManager();

                    mIsInitialized = true;
                    Log.i(TAG, "Xiaomi VoIP initialization fully completed!");
                } catch (Throwable t) {
                    Log.e(TAG, "Failed to initialize Xiaomi VoIP", t);
                }
            }
        }).start();
    }

    public synchronized void call(final String targetDeviceId, final String targetDeviceName, final String targetUid, final String targetDeviceModel, final int targetDeviceType) {
        if (!isReady()) {
            notifyState(STATE_ERROR, "VoIP 尚未就绪，请稍后重试");
            return;
        }

        long parsedUid = 0;
        if (targetUid != null && !targetUid.trim().isEmpty()) {
            try {
                parsedUid = Long.parseLong(targetUid.trim());
            } catch (Exception e) {
                Log.w(TAG, "Failed to parse targetUid: " + targetUid, e);
            }
        }

        if (parsedUid <= 0) {
            notifyState(STATE_ERROR, "呼叫失败：目标音箱 VoIP UID 无效 (" + targetUid + ")");
            return;
        }

        final long contactUid = parsedUid;
        mTargetDeviceId = targetDeviceId;
        mTargetDeviceName = targetDeviceName;
        mTargetUid = contactUid;
        mCallDurationSec = 0;
        mMuted = false;
        mSpeakerphone = true;

        notifyState(STATE_CALLING, "正在呼叫 " + (targetDeviceName != null ? targetDeviceName : String.valueOf(contactUid)) + "...");
        playTone("voip_ringback", true);

        // Send cloud wake-up push (notifyMiLinkEnable) to target speaker
        notifyMiLinkEnable(String.valueOf(contactUid));

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    // Setup AudioManager for in-communication mode
                    setAudioModeInCall(true);

                    // Ensure SDK state is clean before dialing
                    cleanupCallSdk();

                    // Ensure VideoLayoutManager is registered before dialing
                    ensureVideoLayout();

                    // Wait 500ms for MiLink wake-up push to propagate to speaker
                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException ignored) {}

                    // Make call: s(long contactUid, int callType) -> 1 is audio call
                    Method sMethod = mOperatorClass.getMethod("s", long.class, int.class);
                    sMethod.invoke(mOperator, contactUid, 1);
                    Log.i(TAG, "Initiated Xiaomi VoIP audio call to uid=" + contactUid);
                } catch (Throwable t) {
                    Log.e(TAG, "Error initiating Xiaomi VoIP call", t);
                    stopTone();
                    setAudioModeInCall(false);
                    notifyState(STATE_ERROR, "发起呼叫失败: " + t.getMessage());
                }
            }
        }).start();
    }

    private void notifyMiLinkEnable(final String uid) {
        if (mServiceToken == null || mServiceToken.isEmpty() || uid == null || uid.isEmpty()) {
            Log.w(TAG, "Cannot notifyMiLinkEnable: mServiceToken or uid is empty");
            return;
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                String[] endpoints = new String[]{
                    "https://voip.mina.xiaoaisound.com/remote/voip/milink_enable",
                    "https://api2.mina.xiaoaisound.com/remote/voip/milink_enable",
                    "https://api.mina.mi.com/remote/voip/milink_enable"
                };

                for (String endpoint : endpoints) {
                    try {
                        java.net.URL url = new java.net.URL(endpoint);
                        java.net.HttpURLConnection conn = (java.net.HttpURLConnection) url.openConnection();
                        conn.setRequestMethod("POST");
                        conn.setConnectTimeout(4000);
                        conn.setReadTimeout(4000);
                        conn.setRequestProperty("User-Agent", "MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MI 8) Mobile");
                        conn.setRequestProperty("Cookie", "userId=" + mUserId + "; serviceToken=" + mServiceToken);
                        conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
                        conn.setDoOutput(true);

                        byte[] body = ("uid=" + uid).getBytes("UTF-8");
                        java.io.OutputStream os = conn.getOutputStream();
                        os.write(body);
                        os.flush();
                        os.close();

                        int code = conn.getResponseCode();
                        java.io.InputStream is = (code >= 200 && code < 400) ? conn.getInputStream() : conn.getErrorStream();
                        String resp = "";
                        if (is != null) {
                            java.io.ByteArrayOutputStream baos = new java.io.ByteArrayOutputStream();
                            byte[] buf = new byte[1024];
                            int n;
                            while ((n = is.read(buf)) != -1) {
                                baos.write(buf, 0, n);
                            }
                            resp = baos.toString("UTF-8");
                        }
                        Log.i(TAG, "notifyMiLinkEnable endpoint=" + endpoint + " code=" + code + " resp=" + resp);
                    } catch (Throwable t) {
                        Log.w(TAG, "notifyMiLinkEnable error on " + endpoint + ": " + t.getMessage());
                    }
                }

                // Also send through /voip/proxy
                try {
                    java.net.URL url = new java.net.URL("https://voip.mina.xiaoaisound.com/voip/proxy");
                    java.net.HttpURLConnection conn = (java.net.HttpURLConnection) url.openConnection();
                    conn.setRequestMethod("POST");
                    conn.setConnectTimeout(4000);
                    conn.setReadTimeout(4000);
                    conn.setRequestProperty("User-Agent", "MISoundBox/2.4.45 (Linux; U; Android 14; zh_CN; MI 8) Mobile");
                    conn.setRequestProperty("Cookie", "userId=" + mUserId + "; serviceToken=" + mServiceToken);
                    conn.setRequestProperty("Content-Type", "application/json;charset=UTF-8");
                    conn.setDoOutput(true);

                    String jsonBody = "{\"path\":\"/remote/voip/milink_enable\",\"method\":\"post\",\"params\":{\"uid\":\"" + uid + "\"},\"paramsType\":\"form\"}";
                    java.io.OutputStream os = conn.getOutputStream();
                    os.write(jsonBody.getBytes("UTF-8"));
                    os.flush();
                    os.close();

                    int code = conn.getResponseCode();
                    Log.i(TAG, "notifyMiLinkEnable via voip/proxy code=" + code);
                } catch (Throwable t) {
                    Log.w(TAG, "notifyMiLinkEnable proxy error: " + t.getMessage());
                }
            }
        }).start();
    }

    public synchronized void call(String targetDeviceId, String targetDeviceName) {
        call(targetDeviceId, targetDeviceName, null, null, 0);
    }

    public synchronized void hangup() {
        Log.i(TAG, "User requested hangup");
        stopDurationTimer();
        stopTone();
        playTone("voip_phone_hangup", false);

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (mOperator != null && mOperatorClass != null) {
                        Method oMethod = mOperatorClass.getMethod("o");
                        oMethod.invoke(mOperator);
                        Log.i(TAG, "Operator.o() (cancelCall) invoked");
                    }
                } catch (Throwable t) {
                    Log.w(TAG, "Error invoking operator.o()", t);
                }
                setAudioModeInCall(false);
            }
        }).start();

        notifyState(STATE_HANGUP, "通话已结束");
    }

    public synchronized void mute(final boolean mute) {
        mMuted = mute;
        if (mOperator != null && mOperatorClass != null) {
            try {
                Method uMethod = mOperatorClass.getMethod("u", boolean.class);
                uMethod.invoke(mOperator, mute);
                Log.i(TAG, "Operator.u() (muteLocalAudio) set to " + mute);
            } catch (Throwable t) {
                Log.w(TAG, "Error invoking operator.u()", t);
            }
        }
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    AudioManager am = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
                    if (am != null) {
                        am.setMicrophoneMute(mute);
                    }
                } catch (Throwable ignored) {}
            }
        });
    }

    public synchronized void speakerphone(final boolean enable) {
        mSpeakerphone = enable;
        if (mOperator != null && mOperatorClass != null) {
            try {
                Method lMethod = mOperatorClass.getMethod("l", boolean.class);
                lMethod.invoke(mOperator, enable);
                Log.i(TAG, "Operator.l() (setAudioSpeaker) set to " + enable);
            } catch (Throwable t) {
                Log.w(TAG, "Error invoking operator.l()", t);
            }
        }
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    AudioManager am = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
                    if (am != null) {
                        am.setSpeakerphoneOn(enable);
                    }
                    notifyAudioRoute(enable);
                } catch (Throwable ignored) {}
            }
        });
    }

    private void setupVideoLayoutManager() {
        if (mOperator == null || mOperatorClass == null || mJ6hClass == null) {
            return;
        }
        try {
            mMainHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mDummyFrameLayout == null && mContext != null) {
                        mDummyFrameLayout = new FrameLayout(mContext);
                    }
                }
            });

            mVideoLayoutProxy = Proxy.newProxyInstance(
                mLoader,
                new Class<?>[]{ mJ6hClass },
                new InvocationHandler() {
                    @Override
                    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                        String name = method.getName();
                        if ("h".equals(name)) {
                            if (mDummyFrameLayout == null && mContext != null) {
                                mDummyFrameLayout = new FrameLayout(mContext);
                            }
                            return mDummyFrameLayout;
                        }
                        if ("toString".equals(name)) {
                            return "J6hProxy@" + Integer.toHexString(hashCode());
                        }
                        if ("hashCode".equals(name)) {
                            return Integer.valueOf(1);
                        }
                        if ("equals".equals(name)) {
                            return Boolean.valueOf(proxy == (args != null && args.length > 0 ? args[0] : null));
                        }
                        return null;
                    }
                }
            );

            ensureVideoLayout();
            Log.i(TAG, "VideoLayoutManager proxy initialized successfully");
        } catch (Throwable t) {
            Log.w(TAG, "Failed to setup VideoLayoutManager", t);
        }
    }

    private void ensureVideoLayout() {
        if (mOperator == null || mOperatorClass == null || mVideoLayoutProxy == null || mJ6hClass == null) {
            return;
        }
        try {
            Method eMethod = mOperatorClass.getMethod("e", mJ6hClass, int.class);
            eMethod.invoke(mOperator, mVideoLayoutProxy, 1);
            Log.i(TAG, "Operator.e(mVideoLayoutProxy, 1) registered (VideoLayoutManager active)");
        } catch (Throwable t) {
            Log.w(TAG, "Error ensuring VideoLayoutManager: " + t.getMessage());
        }
    }

    private void cleanupCallSdk() {
        if (mOperator != null && mOperatorClass != null) {
            try {
                Method oMethod = mOperatorClass.getMethod("o");
                oMethod.invoke(mOperator);
                Log.i(TAG, "Operator.o() (cancelCall) invoked for SDK cleanup");
            } catch (Throwable t) {
                Log.w(TAG, "Error invoking operator.o()", t);
            }
        }
    }

    private void handleVoipEvent(String methodName, Object[] args) {
        Log.i(TAG, "handleVoipEvent: " + methodName);
        if ("c".equals(methodName)) {
            // onPeerAccepted(I6.d dVar)
            Log.i(TAG, "VoIP event: onPeerAccepted (Call Connected)");
            stopTone();
            playTone("voip_on", false);
            notifyState(STATE_CONNECTED, "通话中");
            startDurationTimer();
            speakerphone(mSpeakerphone);
            mute(mMuted);
        } else if ("l".equals(methodName)) {
            // onPeerCancelled(long uid)
            Log.i(TAG, "VoIP event: onPeerCancelled");
            cleanupCallSdk();
            stopDurationTimer();
            stopTone();
            playTone("voip_phone_hangup", false);
            setAudioModeInCall(false);
            notifyState(STATE_HANGUP, "对方已挂断");
        } else if ("o".equals(methodName)) {
            // onPeerBusy(long uid)
            Log.i(TAG, "VoIP event: onPeerBusy");
            cleanupCallSdk();
            stopDurationTimer();
            stopTone();
            playTone("voip_off", false);
            setAudioModeInCall(false);
            notifyState(STATE_HANGUP, "对方忙线中");
        } else if ("p".equals(methodName)) {
            // onAllPeersDisconnected(int type)
            Log.i(TAG, "VoIP event: onAllPeersDisconnected");
            cleanupCallSdk();
            stopDurationTimer();
            stopTone();
            playTone("voip_phone_hangup", false);
            setAudioModeInCall(false);
            notifyState(STATE_HANGUP, "通话已结束");
        } else if ("b".equals(methodName)) {
            // onPeerTimeout(long uid)
            Log.w(TAG, "VoIP event: onPeerTimeout");
            cleanupCallSdk();
            stopDurationTimer();
            stopTone();
            playTone("voip_phone_hangup", false);
            setAudioModeInCall(false);
            notifyState(STATE_ERROR, "对方未应答，呼叫超时");
        } else if ("e".equals(methodName)) {
            // onCallTimeout(I6.d dVar)
            Log.w(TAG, "VoIP event: onCallTimeout");
            cleanupCallSdk();
            stopDurationTimer();
            stopTone();
            playTone("voip_phone_hangup", false);
            setAudioModeInCall(false);
            notifyState(STATE_ERROR, "呼叫超时");
        } else if ("j".equals(methodName)) {
            // onHeartBeatTimeout(long uid)
            Log.w(TAG, "VoIP event: onHeartBeatTimeout");
            cleanupCallSdk();
            stopDurationTimer();
            stopTone();
            playTone("voip_off", false);
            setAudioModeInCall(false);
            notifyState(STATE_ERROR, "通话心跳超时");
        } else if ("h".equals(methodName)) {
            // onError(int type, int code, String msg)
            int errType = (args != null && args.length > 0 && args[0] instanceof Number) ? ((Number) args[0]).intValue() : 0;
            int errCode = (args != null && args.length > 1 && args[1] instanceof Number) ? ((Number) args[1]).intValue() : 0;
            String msg = (args != null && args.length > 2 && args[2] != null) ? args[2].toString() : "";
            Log.w(TAG, "VoIP event onError: type=" + errType + " code=" + errCode + " msg=" + msg);
            if ("stopPreview".equals(msg) || "muteLocalVideo".equals(msg) || "exchangeWindow".equals(msg)
                    || "setLocalMirrorType".equals(msg) || "setAudioSpeaker".equals(msg) || "muteLocalAudio".equals(msg)) {
                return;
            }
            cleanupCallSdk();
            stopDurationTimer();
            stopTone();
            playTone("voip_phone_hangup", false);
            setAudioModeInCall(false);
            notifyState(STATE_ERROR, "呼叫异常: " + msg);
        } else if ("r".equals(methodName)) {
            Log.i(TAG, "VoIP event: onCallIn (incoming call)");
        }
    }

    private void setAudioModeInCall(final boolean inCall) {
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    AudioManager am = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
                    if (am != null) {
                        if (inCall) {
                            am.setMode(AudioManager.MODE_IN_COMMUNICATION);
                            am.setSpeakerphoneOn(true);
                        } else {
                            am.setMode(AudioManager.MODE_NORMAL);
                            am.setSpeakerphoneOn(false);
                            am.setMicrophoneMute(false);
                        }
                    }
                } catch (Throwable t) {
                    Log.w(TAG, "Error configuring AudioManager: " + t.getMessage());
                }
            }
        });
    }

    private void playTone(final String toneName, final boolean loop) {
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    stopTone();
                    if (mContext == null) return;
                    int resId = mContext.getResources().getIdentifier(toneName, "raw", mContext.getPackageName());
                    if (resId == 0) {
                        Log.w(TAG, "Tone resource not found: " + toneName);
                        return;
                    }
                    mMediaPlayer = MediaPlayer.create(mContext, resId);
                    if (mMediaPlayer != null) {
                        mMediaPlayer.setLooping(loop);
                        mMediaPlayer.start();
                    }
                } catch (Throwable t) {
                    Log.w(TAG, "Error playing tone: " + toneName, t);
                }
            }
        });
    }

    private void stopTone() {
        if (mMediaPlayer != null) {
            try {
                mMediaPlayer.stop();
                mMediaPlayer.release();
            } catch (Throwable ignored) {}
            mMediaPlayer = null;
        }
    }

    private void startDurationTimer() {
        stopDurationTimer();
        mCallDurationSec = 0;
        mDurationRunnable = new Runnable() {
            @Override
            public void run() {
                mCallDurationSec++;
                notifyDuration(mCallDurationSec);
                mMainHandler.postDelayed(this, 1000);
            }
        };
        mMainHandler.postDelayed(mDurationRunnable, 1000);
    }

    private void stopDurationTimer() {
        if (mDurationRunnable != null) {
            mMainHandler.removeCallbacks(mDurationRunnable);
            mDurationRunnable = null;
        }
    }

    private void notifyState(final int state, final String statusText) {
        mState = state;
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    nativeOnVoipStateChanged(state, statusText);
                } catch (Throwable t) {
                    Log.w(TAG, "Error in nativeOnVoipStateChanged", t);
                }
            }
        });
    }

    private void notifyDuration(final int durationSec) {
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    nativeOnVoipDuration(durationSec);
                } catch (Throwable t) {
                    Log.w(TAG, "Error in nativeOnVoipDuration", t);
                }
            }
        });
    }

    private void notifyAudioRoute(final boolean isSpeaker) {
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    nativeOnVoipAudioRoute(isSpeaker);
                } catch (Throwable t) {
                    Log.w(TAG, "Error in nativeOnVoipAudioRoute", t);
                }
            }
        });
    }
}
