.pragma library

var presetDishes = [
    {
        name: "煮鸡蛋",
        key: "eggs",
        time: 15,
        icon: "qrc:/icons/egg.svg",
        desc: "全熟嫩滑 · 水开10分",
        badge: "营养早餐",
        color: "#fbbf24"
    },
    {
        name: "蒸地瓜",
        key: "sweet_potato",
        time: 30,
        icon: "qrc:/icons/sweet_potato.svg",
        desc: "软糯流蜜 · 熟透无硬心",
        badge: "粗粮主食",
        color: "#f97316"
    },
    {
        name: "蒸芋头",
        key: "taro",
        time: 25,
        icon: "qrc:/icons/taro.svg",
        desc: "粉糯起沙 · 筷子一扎即透",
        badge: "香浓可口",
        color: "#a855f7"
    },
    {
        name: "蒸玉米",
        key: "corn",
        time: 20,
        icon: "qrc:/icons/cooker.svg",
        desc: "甜脆多汁 · 锁住鲜甜",
        badge: "轻食粗粮",
        color: "#eab308"
    },
    {
        name: "蒸包子",
        key: "buns",
        time: 12,
        icon: "qrc:/icons/cooker.svg",
        desc: "松软宣腾 · 馅料多汁",
        badge: "面点速食",
        color: "#38bdf8"
    },
    {
        name: "蒸南瓜",
        key: "pumpkin",
        time: 18,
        icon: "qrc:/icons/cooker.svg",
        desc: "绵软香甜 · 入口即化",
        badge: "养胃首选",
        color: "#f59e0b"
    },
    {
        name: "热牛奶",
        key: "milk",
        time: 5,
        icon: "qrc:/icons/cooker.svg",
        desc: "温和适口 · 暖胃醒神",
        badge: "快热饮品",
        color: "#ec4899"
    },
    {
        name: "热饭菜",
        key: "leftovers",
        time: 8,
        icon: "qrc:/icons/cooker.svg",
        desc: "蒸汽循环 · 快速均匀回热",
        badge: "一键快温",
        color: "#10b981"
    }
];

function formatRemainTime(seconds) {
    if (seconds <= 0) return "00:00";
    var m = Math.floor(seconds / 60);
    var s = seconds % 60;
    var mStr = m < 10 ? "0" + m : "" + m;
    var sStr = s < 10 ? "0" + s : "" + s;
    return mStr + ":" + sStr;
}
