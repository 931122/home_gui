#include "platformhelper.h"

void PlatformHelper::applyEnvironment(const PlatformConfig &config, int argc, char *argv[])
{
    bool hasExplicitPlatformArgument = false;
    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == QStringLiteral("-platform")) {
            hasExplicitPlatformArgument = true;
            break;
        }
    }

    const bool hasPredefinedPlatform = !qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM");
    // 即使平台由脚本指定，也统一使用轻量 Basic Controls，减少嵌入式首屏加载成本。
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArrayLiteral("Basic"));
    const bool softwareRendering = config.renderMode.compare(QStringLiteral("linuxfb"), Qt::CaseInsensitive) == 0;
    if (hasExplicitPlatformArgument || hasPredefinedPlatform) {
        if (config.reducedEffects || softwareRendering) {
            qputenv("HOME_GUI_REDUCED_EFFECTS", QByteArrayLiteral("1"));
        } else {
            qunsetenv("HOME_GUI_REDUCED_EFFECTS");
        }
        return;
    }

    QByteArray qpaPlatform = config.renderMode.toUtf8();
#if defined(Q_OS_ANDROID)
    qpaPlatform = QByteArrayLiteral("android");
#elif defined(Q_OS_MACOS)
    // macOS 不支持 eglfs/linuxfb，开发机上自动回退到 cocoa。
    if (qpaPlatform == "eglfs" || qpaPlatform == "linuxfb") {
        qpaPlatform = QByteArrayLiteral("cocoa");
    }
#elif defined(Q_OS_WIN)
    // Windows 开发环境同理回退到桌面平台插件。
    if (qpaPlatform == "eglfs" || qpaPlatform == "linuxfb") {
        qpaPlatform = QByteArrayLiteral("windows");
    }
#elif defined(Q_OS_LINUX)
    // 本机 Linux 桌面环境（X11 或 Wayland）下，不能直接操作 fb0，回退到桌面窗口系统。
    if (isDesktopEnvironment()) {
        if (qpaPlatform == "eglfs" || qpaPlatform == "linuxfb") {
            if (!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY") && qEnvironmentVariableIsEmpty("DISPLAY")) {
                qpaPlatform = QByteArrayLiteral("wayland");
            } else {
                qpaPlatform = QByteArrayLiteral("xcb");
            }
        }
    }
#endif

    // 统一在启动时固定 UI 风格，避免不同平台样式差异过大。
    qputenv("QT_QPA_PLATFORM", qpaPlatform);
    // 低性能或纯软件光栅化平台可以通过这个环境变量让界面主动减特效。
    if (config.reducedEffects || softwareRendering) {
        qputenv("HOME_GUI_REDUCED_EFFECTS", QByteArrayLiteral("1"));
    } else {
        qunsetenv("HOME_GUI_REDUCED_EFFECTS");
    }
}

bool PlatformHelper::isDesktopEnvironment()
{
#if defined(Q_OS_ANDROID)
    return false;
#elif defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    return true;
#elif defined(Q_OS_LINUX)
#if defined(HOME_GUI_CROSS_COMPILE)
    return false;
#else
    return !qEnvironmentVariableIsEmpty("DISPLAY") || !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
#endif
#else
    return false;
#endif
}

namespace {
struct CookerModeEntry {
    const char *name;
    const char *slug;
};

static const CookerModeEntry s_cookerModeMapping[] = {
    { "大米饭", "white_rice" },
    { "标准煮饭", "standard_rice" },
    { "极速煮饭", "quick_rice" },
    { "杂粮饭", "multigrain_rice" },
    { "热米饭", "reheat_rice" },
    { "开盖收汁", "open_lid_saute" },
    { "保温", "keep_warm" },
    { "再加热", "reheat" },
    { "红烧肉", "braised_pork_belly" },
    { "红烧排骨", "braised_ribs" },
    { "糖醋排骨", "sweet_sour_ribs" },
    { "豆豉蒸排骨", "steamed_ribs_black_bean" },
    { "糯米蒸排骨", "steamed_ribs_sticky_rice" },
    { "土豆炖排骨", "stewed_ribs_potatoes" },
    { "精炖猪肉", "braised_pork" },
    { "无水焗排骨", "waterless_baked_ribs" },
    { "无水焗鸡", "waterless_baked_chicken" },
    { "无水焗牛羊", "waterless_baked_beef_lamb" },
    { "黄焖鸡", "braised_chicken_huangmen" },
    { "小鸡炖蘑菇", "chicken_stew_mushrooms" },
    { "香菇蒸鸡", "steamed_chicken_mushrooms" },
    { "鲜炖鸡鸭", "braised_poultry" },
    { "红烧鸡爪", "braised_chicken_feet" },
    { "板栗焖鸡", "braised_chicken_chestnuts" },
    { "啤酒鸭", "beer_duck" },
    { "盐水鸭", "salted_duck" },
    { "酱鸭腿", "soy_sauce_duck_legs" },
    { "香辣卤鸭脖", "spicy_duck_necks" },
    { "卤鸭掌鸭翅", "braised_duck_feet_wings" },
    { "眉豆煲鸡爪", "chicken_feet_soup_beans" },
    { "土豆炖牛肉", "beef_stew_potatoes" },
    { "番茄牛腩", "beef_brisket_tomatoes" },
    { "红烩牛肉", "braised_beef_stew" },
    { "卤牛腱", "spiced_beef_shank" },
    { "炖牛筋", "stewed_beef_tendon" },
    { "酱牛骨头", "braised_beef_bones" },
    { "焖炖牛羊", "braised_beef_mutton" },
    { "手抓羊肉", "boiled_lamb_chops" },
    { "枝竹羊腩煲", "lamb_brisket_casserole" },
    { "萝卜焖羊肉", "lamb_stew_radish" },
    { "酱羊蝎子", "braised_lamb_spine" },
    { "冰糖肘子", "rock_sugar_pork_shank" },
    { "梅干菜烧肉", "braised_pork_preserved_mustard" },
    { "酸菜炖棒骨", "pork_bones_sauerkraut" },
    { "酱棒骨", "soy_braised_pork_bones" },
    { "粉蒸肉", "steamed_pork_rice_flour" },
    { "红烧猪蹄", "braised_pork_trotters" },
    { "黄豆炖猪蹄", "trotters_stew_soybeans" },
    { "南乳焖猪手", "trotters_fermented_tofu" },
    { "水晶肉皮冻", "crystal_pork_skin_jelly" },
    { "豆类蹄筋", "beans_and_tendons" },
    { "浓香煲汤", "rich_soup" },
    { "花胶鸡汤", "fish_maw_chicken_soup" },
    { "老母鸡汤", "hen_soup" },
    { "乌鸡虫草花", "silkie_chicken_cordyceps" },
    { "人参鸡汤", "ginseng_chicken_soup" },
    { "猪肚鸡汤", "pork_stomach_chicken_soup" },
    { "乳鸽汤", "pigeon_soup" },
    { "笋干老鸭汤", "duck_soup_dried_bamboo" },
    { "酸萝卜鸭汤", "duck_soup_pickled_radish" },
    { "玉竹老鸭汤", "duck_soup_solomonseal" },
    { "当归羊肉汤", "mutton_soup_angelica" },
    { "牛尾清汤", "clear_oxtail_soup" },
    { "萝卜牛腩汤", "beef_brisket_radish_soup" },
    { "玉米排骨汤", "ribs_soup_sweet_corn" },
    { "海带排骨汤", "ribs_soup_kelp" },
    { "薏米排骨汤", "ribs_soup_barley" },
    { "干贝排骨汤", "ribs_soup_scallop" },
    { "海底椰骨汤", "pork_bone_soup_sea_coconut" },
    { "腌笃鲜", "yan_du_xian_soup" },
    { "肉骨茶", "bak_kut_teh" },
    { "罗宋汤", "borscht" },
    { "韩式土豆汤", "korean_gamjatang" },
    { "凤梨苦瓜汤", "pineapple_bitter_melon_soup" },
    { "煲仔饭", "claypot_rice" },
    { "上海菜饭", "shanghai_vegetable_rice" },
    { "羊肉手抓饭", "lamb_pilaf" },
    { "牛肉焖饭", "beef_braised_rice" },
    { "腊味糯米饭", "cured_meat_sticky_rice" },
    { "红豆饭", "red_bean_rice" },
    { "皮蛋瘦肉粥", "congee_preserved_egg_pork" },
    { "杂粮米粥", "multigrain_congee" },
    { "美龄粥", "meiling_congee" },
    { "瑶柱排骨粥", "ribs_congee_scallop" },
    { "四红粥", "four_red_congee" },
    { "花生黑米粥", "peanut_black_rice_congee" },
    { "南瓜小米粥", "pumpkin_millet_congee" },
    { "小米粥", "millet_congee" },
    { "八宝粥", "eight_treasure_congee" },
    { "咸蛋黄肉粽", "salted_egg_pork_zongzi" },
    { "紫米蜜枣粽子", "purple_rice_date_zongzi" },
    { "银耳雪梨", "snow_pear_white_fungus" },
    { "木瓜银耳羹", "papaya_white_fungus_soup" },
    { "银耳莲子羹", "white_fungus_lotus_seed_soup" },
    { "桃胶皂角米", "peach_gum_snow_lotus_seeds" },
    { "桂花糖藕", "sweet_osmanthus_lotus_root" },
    { "绿豆酿藕节", "mung_bean_stuffed_lotus_root" },
    { "冰糖绿豆汤", "sweet_mung_bean_soup" },
    { "绿豆汤", "mung_bean_soup" },
    { "绿豆莲子汤", "mung_bean_lotus_seed_soup" },
    { "陈皮红豆汤", "tangerine_peel_red_bean_soup" },
    { "红豆薏米水", "red_bean_barley_drink" },
    { "柠檬薏米水", "lemon_barley_water" },
    { "燕麦牛奶", "oatmeal_milk" },
    { "芒果捞黑米", "mango_black_rice_dessert" },
    { "话梅芸豆", "preserved_plum_kidney_beans" },
    { "蒸红薯", "steamed_sweet_potatoes" },
    { "土豆泥", "mashed_potatoes" }
};
static const int s_cookerModeMappingCount = sizeof(s_cookerModeMapping) / sizeof(s_cookerModeMapping[0]);
}

QString PlatformHelper::cookerModeToSlug(const QString &mode)
{
    const QString trimmed = mode.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("white_rice");
    }
    for (int i = 0; i < s_cookerModeMappingCount; ++i) {
        if (trimmed == QLatin1String(s_cookerModeMapping[i].name) || trimmed == QLatin1String(s_cookerModeMapping[i].slug)) {
            return QString::fromLatin1(s_cookerModeMapping[i].slug);
        }
    }
    return trimmed;
}

QString PlatformHelper::cookerSlugToMode(const QString &slug)
{
    const QString trimmed = slug.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("大米饭");
    }
    for (int i = 0; i < s_cookerModeMappingCount; ++i) {
        if (trimmed == QLatin1String(s_cookerModeMapping[i].slug)) {
            return QString::fromUtf8(s_cookerModeMapping[i].name);
        }
    }
    return trimmed;
}

QString PlatformHelper::cookerTasteToSlug(const QString &taste)
{
    const QString trimmed = taste.trimmed();
    if (trimmed == QStringLiteral("软糯") || trimmed == QStringLiteral("soft_glutinous")) {
        return QStringLiteral("soft_glutinous");
    }
    if (trimmed == QStringLiteral("适中") || trimmed == QStringLiteral("standard")) {
        return QStringLiteral("standard");
    }
    if (trimmed == QStringLiteral("弹润") || trimmed == QStringLiteral("嚼劲") || trimmed == QStringLiteral("springy") || trimmed == QStringLiteral("chewy")) {
        return QStringLiteral("springy");
    }
    return trimmed.isEmpty() ? QStringLiteral("standard") : trimmed;
}

QString PlatformHelper::cookerSlugToTaste(const QString &slug)
{
    const QString trimmed = slug.trimmed().toLower();
    if (trimmed == QStringLiteral("soft_glutinous") || trimmed == QStringLiteral("0") || trimmed == QStringLiteral("软糯")) {
        return QStringLiteral("软糯");
    }
    if (trimmed == QStringLiteral("standard") || trimmed == QStringLiteral("1") || trimmed == QStringLiteral("适中")) {
        return QStringLiteral("适中");
    }
    if (trimmed == QStringLiteral("springy") || trimmed == QStringLiteral("chewy") || trimmed == QStringLiteral("2") || trimmed == QStringLiteral("弹润") || trimmed == QStringLiteral("嚼劲")) {
        return QStringLiteral("弹润");
    }
    return trimmed.isEmpty() ? QStringLiteral("适中") : slug;
}

