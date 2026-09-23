package org.qtproject.example.home_gui;

import android.app.Activity;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.PowerManager;
import android.provider.Settings;

public final class AndroidGlassRuntime implements SensorEventListener {
    private static AndroidGlassRuntime instance;
    private static volatile float tiltX;
    private static volatile float tiltY;

    private final Activity activity;
    private final SensorManager sensorManager;

    private AndroidGlassRuntime(Activity activity) {
        this.activity = activity;
        sensorManager = (SensorManager) activity.getSystemService(Context.SENSOR_SERVICE);
        if (sensorManager != null) {
            Sensor sensor = sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY);
            if (sensor == null) {
                sensor = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            }
            if (sensor != null) {
                sensorManager.registerListener(this, sensor, SensorManager.SENSOR_DELAY_UI);
            }
        }
    }

    public static synchronized void start(Activity activity) {
        if (instance == null && activity != null) {
            instance = new AndroidGlassRuntime(activity);
        }
    }

    public static float tiltX() { return tiltX; }
    public static float tiltY() { return tiltY; }

    public static boolean highContrast() {
        Activity activity = currentActivity();
        if (activity == null) return false;
        try {
            return Settings.Secure.getInt(activity.getContentResolver(),
                    "high_text_contrast_enabled", 0) == 1;
        } catch (Exception ignored) {
            return false;
        }
    }

    public static boolean reduceMotion() {
        Activity activity = currentActivity();
        if (activity == null) return false;
        try {
            return Settings.Global.getFloat(activity.getContentResolver(), Settings.Global.ANIMATOR_DURATION_SCALE, 1.0f) == 0.0f
                    || Settings.Global.getFloat(activity.getContentResolver(), Settings.Global.TRANSITION_ANIMATION_SCALE, 1.0f) == 0.0f;
        } catch (Exception ignored) {
            return false;
        }
    }

    public static boolean batterySaver() {
        Activity activity = currentActivity();
        if (activity == null) return false;
        PowerManager power = (PowerManager) activity.getSystemService(Context.POWER_SERVICE);
        return power != null && power.isPowerSaveMode();
    }

    private static Activity currentActivity() {
        return instance == null ? null : instance.activity;
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        float x = event.values[0] / SensorManager.GRAVITY_EARTH;
        float y = event.values[1] / SensorManager.GRAVITY_EARTH;
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
        if (rotation == 1) {
            tiltX = -y;
            tiltY = x;
        } else if (rotation == 2) {
            tiltX = -x;
            tiltY = -y;
        } else if (rotation == 3) {
            tiltX = y;
            tiltY = -x;
        } else {
            tiltX = x;
            tiltY = y;
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {}
}
