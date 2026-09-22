.pragma library
// WasherData.js - 美的智能滚筒洗衣机程序与参数元数据

var commonPrograms = [
    { key: "mixed_wash", name: "混合洗", icon: "qrc:/icons/washer-mix.svg", desc: "日常混纺织物标准洗涤，省心不伤衣", estTime: 45, defaultTemp: "30c", defaultSpeed: "800rpm" },
    { key: "fast_wash", name: "快洗", icon: "qrc:/icons/washer-quick.svg", desc: "夏季或轻度脏污衣物快速清洗，省时省水", estTime: 15, defaultTemp: "cold_water", defaultSpeed: "800rpm" },
    { key: "cotton", name: "棉麻洗", icon: "qrc:/icons/washer-cotton.svg", desc: "床单被罩、棉麻耐磨衣物深层去污", estTime: 55, defaultTemp: "40c", defaultSpeed: "1000rpm" },
    { key: "wool", name: "羊毛洗", icon: "qrc:/icons/washer-wool.svg", desc: "羊毛衫轻柔仿手洗，减少衣物缩水缩绒", estTime: 48, defaultTemp: "20c", defaultSpeed: "600rpm" },
    { key: "steam_sterilize_wash", name: "蒸汽除菌", icon: "qrc:/icons/washer-steam.svg", desc: "高温强劲蒸汽深层渗透，高效除菌除螨", estTime: 65, defaultTemp: "60c", defaultSpeed: "1000rpm" },
    { key: "single_dehytration", name: "单脱水", icon: "qrc:/icons/washer-spin.svg", desc: "针对手洗衣物或需额外脱水衣物高速甩干", estTime: 10, defaultTemp: "cold_water", defaultSpeed: "1200rpm" },
    { key: "air_wash", name: "空气洗", icon: "qrc:/icons/washer-air.svg", desc: "微蒸汽循环热风清新，免水洗去异味蓬松", estTime: 30, defaultTemp: "cold_water", defaultSpeed: "no_spin" },
    { key: "intelligent", name: "智能洗", icon: "qrc:/icons/washer-ai.svg", desc: "AI智能称重感知脏污，自动匹配洗涤节奏", estTime: 42, defaultTemp: "30c", defaultSpeed: "800rpm" }
];

var allPrograms = [
    { key: "mixed_wash", name: "混合洗", icon: "混", desc: "日常混纺织物标准洗涤", estTime: 45 },
    { key: "fast_wash", name: "快洗", icon: "快", desc: "夏季或轻度脏污衣物快速清洗", estTime: 15 },
    { key: "fast_wash_30", name: "30分快洗", icon: "3", desc: "30分钟中度快洗节奏", estTime: 30 },
    { key: "cotton", name: "棉麻洗", icon: "棉", desc: "床单被套、棉麻耐磨衣物", estTime: 55 },
    { key: "new_water_cotton", name: "水魔方棉麻", icon: "棉", desc: "水魔方冷水护色深层去污", estTime: 55 },
    { key: "wool", name: "羊毛洗", icon: "羊", desc: "羊毛制品柔性轻揉慢摇", estTime: 48 },
    { key: "silk", name: "真丝洗", icon: "真", desc: "丝绸面料防抽丝柔洗护色", estTime: 40 },
    { key: "underwear", name: "内衣洗", icon: "内", desc: "贴身内衣深层轻柔清洁", estTime: 45 },
    { key: "baby_clothes", name: "婴儿服", icon: "婴", desc: "多重温水漂洗，零洗涤剂残留", estTime: 65 },
    { key: "down_jacket", name: "羽绒服", icon: "羽", desc: "防止羽绒结块，慢摇蓬松", estTime: 50 },
    { key: "steam_sterilize_wash", name: "蒸汽除菌", icon: "蒸", desc: "高温蒸汽深层除菌除螨", estTime: 65 },
    { key: "remove_mite_wash", name: "除螨洗", icon: "除", desc: "持续高温冲刷，除净尘螨", estTime: 60 },
    { key: "air_wash", name: "空气洗", icon: "空", desc: "热风循环微蒸除味祛潮", estTime: 30 },
    { key: "single_dehytration", name: "单脱水", icon: "单", desc: "独立高速脱水甩干", estTime: 10 },
    { key: "rinsing_dehydration", name: "漂洗脱水", icon: "漂", desc: "加注清水漂洗后甩干", estTime: 22 },
    { key: "intelligent", name: "智能洗", icon: "智", desc: "智能感知重量与水流匹配", estTime: 42 },
    { key: "clean_stains", name: "特渍洗", icon: "特", desc: "针对油渍、果汁等顽固污垢", estTime: 68 },
    { key: "cold_wash", name: "冷水洗", icon: "冷", desc: "常温护色不伤纤维", estTime: 40 },
    { key: "shirt", name: "衬衫", icon: "衬", desc: "平整防皱免烫洗涤", estTime: 38 },
    { key: "jean", name: "牛仔", icon: "牛", desc: "重水流冲刷，锁色固色", estTime: 50 },
    { key: "sport_clothes", name: "运动服", icon: "运", desc: "快速排汗速干面料呵护", estTime: 35 },
    { key: "jacket", name: "外套大衣", icon: "外", desc: "厚实外衣适度浸泡冲刷", estTime: 52 },
    { key: "outdoor", name: "冲锋衣", icon: "冲", desc: "防水透气涂层保护洗", estTime: 45 },
    { key: "big", name: "大件洗", icon: "大", desc: "窗帘毯子强劲大水流", estTime: 70 },
    { key: "fiber", name: "化纤洗", icon: "化", desc: "聚酯化纤抗静电清洗", estTime: 40 },
    { key: "enzyme", name: "酵素洗", icon: "酵", desc: "活化生物酶强力去污", estTime: 58 },
    { key: "steep", name: "浸泡洗", icon: "浸", desc: "长时间静置浸泡软化污垢", estTime: 80 },
    { key: "deep_ssp", name: "深度洗", icon: "深", desc: "延长主洗阶段重度去污", estTime: 75 },
    { key: "ssp", name: "筒自洁", icon: "筒", desc: "95°C高温空转冲刷内筒除垢", estTime: 90 },
    { key: "new_clothes_wash", name: "新衣首洗", icon: "新", desc: "洗净新衣浮色与微小杂质", estTime: 25 },
    { key: "summer_wash", name: "夏日洗", icon: "夏", desc: "薄衣快节奏省电清洗", estTime: 28 },
    { key: "winter_wash", name: "冬日洗", icon: "冬", desc: "温水溶解洗涤剂深层去污", estTime: 60 },
    { key: "spring_autumn_wash", name: "春秋洗", icon: "春", desc: "四季中温日常清洗", estTime: 45 },
    { key: "single_drying", name: "单烘干", icon: "单", desc: "热风循环烘干衣物", estTime: 60 }
];

var temperatures = [
    { key: "cold_water", name: "冷水" },
    { key: "20c", name: "20°C" },
    { key: "30c", name: "30°C" },
    { key: "40c", name: "40°C" },
    { key: "60c", name: "60°C" },
    { key: "95c", name: "95°C煮洗" }
];

var spinSpeeds = [
    { key: "no_spin", name: "不脱水" },
    { key: "400rpm", name: "400转" },
    { key: "600rpm", name: "600转" },
    { key: "800rpm", name: "800转" },
    { key: "1000rpm", name: "1000转" },
    { key: "1200rpm", name: "1200转" },
    { key: "1400rpm", name: "1400转" }
];

var rinseCounts = [
    { key: "1_time", name: "1次" },
    { key: "2_times", name: "2次" },
    { key: "3_times", name: "3次" },
    { key: "4_times", name: "4次" }
];

var waterLevels = [
    { key: "auto", name: "自动" },
    { key: "l1", name: "1档" },
    { key: "l2", name: "2档" },
    { key: "l3", name: "3档" },
    { key: "l4", name: "4档" }
];

var detergents = [
    { key: "smart", name: "智能投放" },
    { key: "off", name: "关闭投放" },
    { key: "l1", name: "1档 (轻量)" },
    { key: "l2", name: "2档 (标准)" },
    { key: "l3", name: "3档 (增量)" },
    { key: "l4", name: "4档 (重度)" }
];

function getProgramName(key) {
    if (!key || key === "unknown") return "标准程序";
    for (var i = 0; i < allPrograms.length; ++i) {
        if (allPrograms[i].key === key) return allPrograms[i].name;
    }
    if (key.indexOf("cotton") !== -1) return "水魔方棉麻";
    if (key.indexOf("fast") !== -1) return "快洗";
    if (key.indexOf("mixed") !== -1) return "混合洗";
    return key;
}

function getProgramDesc(key) {
    if (!key) return "";
    for (var i = 0; i < allPrograms.length; ++i) {
        if (allPrograms[i].key === key) return allPrograms[i].desc;
    }
    return "";
}

function getProgramIcon(key) {
    for (var i = 0; i < allPrograms.length; ++i) {
        if (allPrograms[i].key === key) return allPrograms[i].icon;
    }
    return "洗";
}

function getProgramEstTime(key) {
    for (var i = 0; i < allPrograms.length; ++i) {
        if (allPrograms[i].key === key) return allPrograms[i].estTime;
    }
    return 45;
}

function getTempName(key) {
    for (var i = 0; i < temperatures.length; ++i) {
        if (temperatures[i].key === key) return temperatures[i].name;
    }
    return key || "30°C";
}

function getSpeedName(key) {
    for (var i = 0; i < spinSpeeds.length; ++i) {
        if (spinSpeeds[i].key === key) return spinSpeeds[i].name;
    }
    return key || "800转";
}

function getRinseName(key) {
    for (var i = 0; i < rinseCounts.length; ++i) {
        if (rinseCounts[i].key === key) return rinseCounts[i].name;
    }
    return key || "2次";
}

function getWaterLevelName(key) {
    for (var i = 0; i < waterLevels.length; ++i) {
        if (waterLevels[i].key === key) return waterLevels[i].name;
    }
    return key || "自动";
}

function getDetergentName(key) {
    for (var i = 0; i < detergents.length; ++i) {
        if (detergents[i].key === key) return detergents[i].name;
    }
    return key || "智能投放";
}

function getRunningStatusText(status, power, progress) {
    if (power === "off") return "已关机";
    var s = (status || "").toLowerCase();
    var p = (progress || "").toLowerCase();
    if (s === "pause") return "已暂停";
    if (s === "end" || s === "finish" || p === "end") return "洗涤完成";
    if (p === "wash" || s === "wash") return "洗涤中";
    if (p === "rinse" || s === "rinse") return "漂洗中";
    if (p === "spin" || p === "dehydration" || s === "spin" || s === "dehydration") return "脱水中";
    if (p === "drying" || p === "dry" || s === "drying" || s === "dry") return "烘干中";
    if (p === "soak" || s === "soak") return "浸泡中";
    if (s === "start" || s === "run" || s === "running") return "运行中";
    if (s === "idle") return "空闲";
    if (s === "standby") return "待机";
    if (s === "off") return "已关机";
    return s || "待机";
}

function getProgressText(progress) {
    var p = (progress || "").toLowerCase();
    if (p === "idle" || p === "standby" || p === "none") return "待命";
    if (p === "soak") return "浸泡";
    if (p === "wash") return "主洗";
    if (p === "rinse") return "漂洗";
    if (p === "spin" || p === "dehydration") return "脱水";
    if (p === "drying" || p === "dry") return "烘干";
    if (p === "end" || p === "finish") return "完成";
    return p || "准备就绪";
}

function isRunning(status, power, progress, controlStatus) {
    if (power === "off") return false;
    var s = (status || "").toLowerCase();
    var p = (progress || "").toLowerCase();
    if (s === "start" || s === "run" || s === "running" || s === "wash" || s === "rinse" || s === "spin" || s === "dehydration" || s === "drying" || s === "dry" || s === "soak" || s === "pause") {
        return true;
    }
    if (p === "wash" || p === "rinse" || p === "spin" || p === "dehydration" || p === "drying" || p === "dry" || p === "soak") {
        return true;
    }
    if (controlStatus === "on" && s !== "idle" && s !== "standby" && s !== "off" && s !== "end") {
        return true;
    }
    return false;
}

function getStatusColor(status, power, progress) {
    if (power === "off") return "#78909c";
    var s = (status || "").toLowerCase();
    var p = (progress || "").toLowerCase();
    if (s === "pause") return "#fbbf24";     // 暂停琥珀金
    if (s === "end" || s === "finish" || p === "end") return "#4ade80";       // 成功翠绿
    if (p === "rinse" || s === "rinse") return "#2dd4bf";     // 碧水青
    if (p === "wash" || s === "wash") return "#38bdf8";       // 科技水蓝
    if (p === "spin" || p === "dehydration" || s === "spin" || s === "dehydration") return "#a855f7"; // 强劲紫
    if (p === "drying" || p === "dry" || s === "drying" || s === "dry") return "#f97316";   // 温暖橙
    return "#38bdf8";                        // 默认浅蓝
}

function getProgramIcon(key, name) {
    var k = (key || "").toLowerCase();
    var n = name || "";
    if (k === "mixed_wash" || n.indexOf("混合") !== -1) return "qrc:/icons/washer-mix.svg";
    if (k.indexOf("fast") !== -1 || n.indexOf("快") !== -1) return "qrc:/icons/washer-quick.svg";
    if (k === "cotton" || n.indexOf("棉") !== -1 || n.indexOf("麻") !== -1) return "qrc:/icons/washer-cotton.svg";
    if (k === "wool" || n.indexOf("羊毛") !== -1 || n.indexOf("羽绒") !== -1 || n.indexOf("真丝") !== -1) return "qrc:/icons/washer-wool.svg";
    if (k === "steam_sterilize_wash" || k === "remove_mite_wash" || n.indexOf("除菌") !== -1 || n.indexOf("除螨") !== -1 || n.indexOf("蒸汽") !== -1) return "qrc:/icons/washer-steam.svg";
    if (k.indexOf("dehytration") !== -1 || k.indexOf("dehydration") !== -1 || n.indexOf("脱水") !== -1 || n.indexOf("甩干") !== -1 || n.indexOf("漂洗") !== -1) return "qrc:/icons/washer-spin.svg";
    if (k === "air_wash" || k.indexOf("drying") !== -1 || n.indexOf("空气") !== -1 || n.indexOf("烘干") !== -1) return "qrc:/icons/washer-air.svg";
    if (k === "intelligent" || n.indexOf("智能") !== -1) return "qrc:/icons/washer-ai.svg";
    return "qrc:/icons/washer-general.svg";
}
