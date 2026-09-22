package org.qtproject.example.home_gui;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;
import android.view.Display;
import android.view.Window;
import android.view.WindowManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.lang.reflect.Method;

/**
 * Android 原生屏幕物理背光调节与屏幕硬件检测辅助类
 */
public class AndroidBrightnessHelper {
    private static final String TAG = "AndroidBrightness";

    /**
     * 读取 Android 系统当前的屏幕亮度 (0.0f ~ 1.0f)
     */
    public static float getSystemBrightness(Activity activity) {
        if (activity == null) {
            return 1.0f;
        }
        try {
            int val = Settings.System.getInt(
                    activity.getContentResolver(),
                    Settings.System.SCREEN_BRIGHTNESS,
                    128
            );
            return Math.max(0.01f, Math.min(1.0f, (float) val / 255.0f));
        } catch (Exception e) {
            return 0.8f;
        }
    }

    /**
     * 设置当前窗口的屏幕物理背光亮度 (0.0f ~ 1.0f)
     * 设置为 0.0f 时彻底关断物理背光输出 (灭屏)
     */
    public static void setWindowBrightness(final Activity activity, final float brightness) {
        if (activity == null) {
            return;
        }
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Window window = activity.getWindow();
                    if (window != null) {
                        WindowManager.LayoutParams lp = window.getAttributes();
                        lp.screenBrightness = Math.max(0.0f, Math.min(1.0f, brightness));
                        window.setAttributes(lp);
                    }
                } catch (Exception ignored) {
                }
            }
        });
    }

    /**
     * 智能探测当前 Android 设备的屏幕硬件材质 ("OLED" 或 "LCD")
     */
    public static String detectPanelType(Activity activity) {
        if (activity == null) {
            return "OLED";
        }

        // 1. Android 官方 API 探测：广色域 (Wide Color Gamut) 与 HDR 能力
        // 现代 OLED 手机几乎 100% 支持 Wide Color Gamut (Display P3) 或 HDR
        try {
            WindowManager wm = (WindowManager) activity.getSystemService(Context.WINDOW_SERVICE);
            if (wm != null) {
                Display display = wm.getDefaultDisplay();
                if (display != null) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        if (display.isWideColorGamut()) {
                            Log.i(TAG, "Display.isWideColorGamut() is true -> detected OLED");
                            return "OLED";
                        }
                    }
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                        Display.HdrCapabilities hdr = display.getHdrCapabilities();
                        if (hdr != null && hdr.getSupportedHdrTypes() != null && hdr.getSupportedHdrTypes().length > 0) {
                            Log.i(TAG, "Display.getHdrCapabilities() has supported types -> detected OLED");
                            return "OLED";
                        }
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "Failed to check Display capabilities: " + e.getMessage());
        }

        // 2. 扫描 Android 底层 SystemProperties
        String[] propKeys = new String[]{
                "ro.vendor.display.type",
                "ro.display.type",
                "ro.config.oled",
                "ro.hardware.oled",
                "ro.boot.panel_name",
                "ro.vendor.panel.name",
                "vendor.display.lcd",
                "ro.boot.lcd"
        };

        for (String key : propKeys) {
            try {
                Class<?> spClass = Class.forName("android.os.SystemProperties");
                Method getMethod = spClass.getMethod("get", String.class, String.class);
                String val = (String) getMethod.invoke(null, key, "");
                if (val != null && !val.trim().isEmpty()) {
                    String lower = val.toLowerCase();
                    Log.i(TAG, "Display prop " + key + "=" + val);

                    // 华为 ro.config.oled / ro.hardware.oled 为 true 时为 OLED
                    if ((key.equals("ro.config.oled") || key.equals("ro.hardware.oled")) && val.equalsIgnoreCase("true")) {
                        return "OLED";
                    }

                    // 小米 vendor.display.lcd: 0 表示 OLED (无LCD背光), 1 表示 LCD
                    if (key.equals("vendor.display.lcd")) {
                        if (val.equals("0")) {
                            return "OLED";
                        } else if (val.equals("1")) {
                            return "LCD";
                        }
                    }

                    if (lower.contains("oled") || lower.contains("amoled") || lower.contains("ea8061") || lower.contains("sofef")) {
                        return "OLED";
                    }
                    if (lower.contains("lcd") || lower.contains("tft") || lower.contains("ips")) {
                        return "LCD";
                    }
                }
            } catch (Exception ignored) {
            }
        }

        // 3. 读取 Linux 内核启动引导命令行 /proc/cmdline
        try {
            File cmdFile = new File("/proc/cmdline");
            if (cmdFile.exists() && cmdFile.canRead()) {
                BufferedReader reader = new BufferedReader(new FileReader(cmdFile));
                String cmdline = reader.readLine();
                reader.close();
                if (cmdline != null) {
                    String lower = cmdline.toLowerCase();
                    if (lower.contains("oled") || lower.contains("amoled") || lower.contains("samsung")) {
                        Log.i(TAG, "Detected OLED from /proc/cmdline");
                        return "OLED";
                    }
                    if (lower.contains("nt35596") || lower.contains("ili9881") || lower.contains("hx8394") || lower.contains("tft")) {
                        Log.i(TAG, "Detected LCD from /proc/cmdline");
                        return "LCD";
                    }
                }
            }
        } catch (Exception ignored) {
        }

        // 4. 读取 /sys/devices/virtual/graphics/fb0/name 或 /sys/class/drm/
        try {
            File fbNameFile = new File("/sys/devices/virtual/graphics/fb0/name");
            if (fbNameFile.exists() && fbNameFile.canRead()) {
                BufferedReader reader = new BufferedReader(new FileReader(fbNameFile));
                String name = reader.readLine();
                reader.close();
                if (name != null) {
                    String lower = name.toLowerCase();
                    if (lower.contains("oled") || lower.contains("amoled") || lower.contains("samsung") || lower.contains("amb")) {
                        return "OLED";
                    }
                }
            }
        } catch (Exception ignored) {
        }

        // 智能家居中控屏、86盒、工业平板 90% 以上为 LCD 屏；
        // 若前面均未探测到明确的 OLED/HDR/P3 特征，稳妥默认返回 LCD
        Log.i(TAG, "No OLED signature found. Defaulting display type to LCD");
        return "LCD";
    }
}
