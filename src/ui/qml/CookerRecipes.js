// 纯米/米家智能电压力锅 108 道全量精选菜谱数据库
.pragma library

var categories = [
    {
        "id": "all",
        "name": "全部",
        "icon": "qrc:/icons/cooker-chef.svg",
        "count": 108
    },
    {
        "id": "meat",
        "name": "硬菜牛羊",
        "icon": "qrc:/icons/cooker-meat.svg",
        "count": 30
    },
    {
        "id": "poultry",
        "name": "家常禽肉",
        "icon": "qrc:/icons/cooker-chicken.svg",
        "count": 13
    },
    {
        "id": "soup",
        "name": "名贵靓汤",
        "icon": "qrc:/icons/cooker-soup.svg",
        "count": 23
    },
    {
        "id": "staple",
        "name": "特色主食",
        "icon": "qrc:/icons/cooker-rice.svg",
        "count": 22
    },
    {
        "id": "dessert",
        "name": "甜品糖水",
        "icon": "qrc:/icons/cooker-dessert.svg",
        "count": 17
    },
    {
        "id": "basic",
        "name": "辅助程序",
        "icon": "qrc:/icons/cooker-gear.svg",
        "count": 3
    }
];

var commonRecipeKeys = [
    "杂粮饭",
    "小米粥",
    "大米饭",
    "土豆炖排骨",
    "干贝排骨汤",
    "番茄牛腩",
    "豆类蹄筋",
    "黄焖鸡"
];

var allRecipes = [
    {
        "mode": "红烧肉",
        "name": "红烧肉",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "25 min",
        "estimatedTime": "约 50 min",
        "desc": "浓油赤酱，酥软入味，肥而不腻",
        "icon": "红"
    },
    {
        "mode": "红烧排骨",
        "name": "红烧排骨",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "脱骨入味，无油烟快速红烧",
        "icon": "红"
    },
    {
        "mode": "糖醋排骨",
        "name": "糖醋排骨",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "色泽明亮，肉质鲜软，酸甜开胃",
        "icon": "糖"
    },
    {
        "mode": "豆豉蒸排骨",
        "name": "豆豉蒸排骨",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "10 min",
        "estimatedTime": "约 20 min",
        "desc": "肉质鲜嫩多汁，豉香浓郁",
        "icon": "豆"
    },
    {
        "mode": "糯米蒸排骨",
        "name": "糯米蒸排骨",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "25 min",
        "estimatedTime": "约 50 min",
        "desc": "排骨酥烂，糯米吸足肉香软糯回甘",
        "icon": "糯"
    },
    {
        "mode": "冰糖肘子",
        "name": "冰糖肘子",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "35 min",
        "estimatedTime": "约 70 min",
        "desc": "宴席大菜，外皮红亮Q弹，入口即化",
        "icon": "冰"
    },
    {
        "mode": "梅干菜烧肉",
        "name": "梅干菜烧肉",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "25 min",
        "estimatedTime": "约 50 min",
        "desc": "咸甜适口，梅菜吸满五花肉油脂",
        "icon": "梅"
    },
    {
        "mode": "粉蒸肉",
        "name": "粉蒸肉",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "米粉软糯，肥瘦相间，经典蒸菜",
        "icon": "粉"
    },
    {
        "mode": "酸菜炖棒骨",
        "name": "酸菜炖棒骨",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "东北酸爽酸菜，炖至脱骨的猪棒骨",
        "icon": "酸"
    },
    {
        "mode": "酱棒骨",
        "name": "酱棒骨",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "酱香醇厚，大骨脱骨肉香",
        "icon": "酱"
    },
    {
        "mode": "红烧猪蹄",
        "name": "红烧猪蹄",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "35 min",
        "estimatedTime": "约 70 min",
        "desc": "满满胶原蛋白，软烂弹牙软糯",
        "icon": "红"
    },
    {
        "mode": "黄豆炖猪蹄",
        "name": "黄豆炖猪蹄",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "经典搭配，豆香与肉香完美交融",
        "icon": "黄"
    },
    {
        "mode": "南乳焖猪手",
        "name": "南乳焖猪手",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "粤式南乳风味，色泽红润喜庆",
        "icon": "南"
    },
    {
        "mode": "水晶肉皮冻",
        "name": "水晶肉皮冻",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "晶莹透明，Q弹爽滑凉菜",
        "icon": "水"
    },
    {
        "mode": "土豆炖牛肉",
        "name": "土豆炖牛肉",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "35 min",
        "estimatedTime": "约 70 min",
        "desc": "半筋半肉牛腩，土豆粉糯下饭神器",
        "icon": "土"
    },
    {
        "mode": "红烩牛肉",
        "name": "红烩牛肉",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "35 min",
        "estimatedTime": "约 70 min",
        "desc": "西式番茄香草浓郁风味",
        "icon": "红"
    },
    {
        "mode": "番茄牛腩",
        "name": "番茄牛腩",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "45 min",
        "estimatedTime": "约 90 min",
        "desc": "酸甜浓郁，牛腩软烂入味",
        "icon": "番"
    },
    {
        "mode": "卤牛腱",
        "name": "卤牛腱",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "入味不柴，切片紧实完整不碎",
        "icon": "卤"
    },
    {
        "mode": "炖牛筋",
        "name": "炖牛筋",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "45 min",
        "estimatedTime": "约 90 min",
        "desc": "软弹香浓，筋道可口",
        "icon": "炖"
    },
    {
        "mode": "酱牛骨头",
        "name": "酱牛骨头",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "贴骨牛肉鲜嫩醇香",
        "icon": "酱"
    },
    {
        "mode": "手抓羊肉",
        "name": "手抓羊肉",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "肉赤膘白，肥而不膻，西北原汁原味",
        "icon": "手"
    },
    {
        "mode": "枝竹羊腩煲",
        "name": "枝竹羊腩煲",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "传统粤菜，腐竹吸满浓郁羊肉汁",
        "icon": "枝"
    },
    {
        "mode": "萝卜焖羊肉",
        "name": "萝卜焖羊肉",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "25 min",
        "estimatedTime": "约 50 min",
        "desc": "冬季滋补，羊肉酥烂，白萝卜清甜",
        "icon": "萝"
    },
    {
        "mode": "酱羊蝎子",
        "name": "酱羊蝎子",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "骨髓香浓，肉烂入味",
        "icon": "酱"
    },
    {
        "mode": "土豆炖排骨",
        "name": "土豆炖排骨",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "软烂入味，经典家常",
        "icon": "土"
    },
    {
        "mode": "精炖猪肉",
        "name": "精炖猪肉",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "25 min",
        "estimatedTime": "约 45 min",
        "desc": "红烧软烂，肥瘦兼备",
        "icon": "精"
    },
    {
        "mode": "无水焗排骨",
        "name": "无水焗排骨",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "原汁焗香，浓缩肉汁",
        "icon": "无"
    },
    {
        "mode": "无水焗牛羊",
        "name": "无水焗牛羊",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 55 min",
        "desc": "浓缩原汁，醇厚肉香",
        "icon": "无"
    },
    {
        "mode": "焖炖牛羊",
        "name": "焖炖牛羊",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "锁汁劲道，软烂入味",
        "icon": "焖"
    },
    {
        "mode": "豆类蹄筋",
        "name": "豆类蹄筋",
        "category": "meat",
        "categoryName": "硬菜牛羊",
        "pressureTime": "35 min",
        "estimatedTime": "约 70 min",
        "desc": "高压软糯，浓滑蹄筋",
        "icon": "豆"
    },
    {
        "mode": "无水焗鸡",
        "name": "无水焗鸡",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "粤式豉油鸡做法，一滴水不放，鸡肉极其鲜嫩滑爽",
        "icon": "无"
    },
    {
        "mode": "黄焖鸡",
        "name": "黄焖鸡",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "8 min",
        "estimatedTime": "约 16 min",
        "desc": "浓油香菇滑鸡，超快手下饭大菜",
        "icon": "黄"
    },
    {
        "mode": "板栗焖鸡",
        "name": "板栗焖鸡",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "10 min",
        "estimatedTime": "约 20 min",
        "desc": "栗子香面清甜，鸡肉滑嫩多汁",
        "icon": "板"
    },
    {
        "mode": "小鸡炖蘑菇",
        "name": "小鸡炖蘑菇",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "12 min",
        "estimatedTime": "约 24 min",
        "desc": "东北名菜，野榛蘑与土鸡肉的浓香交织",
        "icon": "小"
    },
    {
        "mode": "香菇蒸鸡",
        "name": "香菇蒸鸡",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "10 min",
        "estimatedTime": "约 20 min",
        "desc": "清蒸原汁原味，低脂少油健康",
        "icon": "香"
    },
    {
        "mode": "红烧鸡爪",
        "name": "红烧鸡爪",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "一抿即脱骨，软糯浓郁胶质感",
        "icon": "红"
    },
    {
        "mode": "啤酒鸭",
        "name": "啤酒鸭",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "12 min",
        "estimatedTime": "约 24 min",
        "desc": "麦芽麦香去腥提味，麻辣鲜香鲜亮",
        "icon": "啤"
    },
    {
        "mode": "盐水鸭",
        "name": "盐水鸭",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "12 min",
        "estimatedTime": "约 27 min",
        "desc": "江南风味，皮白肉嫩，咸鲜可口",
        "icon": "盐"
    },
    {
        "mode": "酱鸭腿",
        "name": "酱鸭腿",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "12 min",
        "estimatedTime": "约 24 min",
        "desc": "浓油赤酱，快手解馋",
        "icon": "酱"
    },
    {
        "mode": "香辣卤鸭脖",
        "name": "香辣卤鸭脖",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "12 min",
        "estimatedTime": "约 24 min",
        "desc": "追剧下酒解馋小吃，绝味同款辣卤",
        "icon": "香"
    },
    {
        "mode": "卤鸭掌鸭翅",
        "name": "卤鸭掌鸭翅",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "12 min",
        "estimatedTime": "约 24 min",
        "desc": "醇厚卤香，筋道爽口",
        "icon": "卤"
    },
    {
        "mode": "眉豆煲鸡爪",
        "name": "眉豆煲鸡爪",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "粤式靓汤与炖菜，清润可口",
        "icon": "眉"
    },
    {
        "mode": "鲜炖鸡鸭",
        "name": "鲜炖鸡鸭",
        "category": "poultry",
        "categoryName": "家常禽肉",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "鲜嫩多汁，汤鲜味美",
        "icon": "鲜"
    },
    {
        "mode": "花胶鸡汤",
        "name": "花胶鸡汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "花胶软糯粘唇，汤色金黄浓郁鲜甜",
        "icon": "花"
    },
    {
        "mode": "老母鸡汤",
        "name": "老母鸡汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "慢火原汁高压聚香，传统逢年过节滋补",
        "icon": "老"
    },
    {
        "mode": "乌鸡虫草花",
        "name": "乌鸡虫草花",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "滋补养颜，汤水清甜澄澈",
        "icon": "乌"
    },
    {
        "mode": "人参鸡汤",
        "name": "人参鸡汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "鲜人参红枣枸杞，大补元气",
        "icon": "人"
    },
    {
        "mode": "猪肚鸡汤",
        "name": "猪肚鸡汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "广式白胡椒猪肚包鸡，暖胃祛寒",
        "icon": "猪"
    },
    {
        "mode": "乳鸽汤",
        "name": "乳鸽汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "一鸽胜九鸡，清润滋补",
        "icon": "乳"
    },
    {
        "mode": "笋干老鸭汤",
        "name": "笋干老鸭汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "笋干清鲜爽口，老鸭醇厚不腻",
        "icon": "笋"
    },
    {
        "mode": "酸萝卜鸭汤",
        "name": "酸萝卜鸭汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "川式酸萝卜老鸭汤，开胃生津消暑",
        "icon": "酸"
    },
    {
        "mode": "玉竹老鸭汤",
        "name": "玉竹老鸭汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "40 min",
        "estimatedTime": "约 90 min",
        "desc": "沙参玉竹润肺养阴",
        "icon": "玉"
    },
    {
        "mode": "当归羊肉汤",
        "name": "当归羊肉汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "35 min",
        "estimatedTime": "约 70 min",
        "desc": "汉方医圣名方，秋冬温补祛寒",
        "icon": "当"
    },
    {
        "mode": "牛尾清汤",
        "name": "牛尾清汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "60 min",
        "estimatedTime": "约 120 min",
        "desc": "白萝卜清炖牛尾，胶质丰厚汤鲜味浓",
        "icon": "牛"
    },
    {
        "mode": "萝卜牛腩汤",
        "name": "萝卜牛腩汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "50 min",
        "estimatedTime": "约 100 min",
        "desc": "健脾开胃，汤清肉烂",
        "icon": "萝"
    },
    {
        "mode": "玉米排骨汤",
        "name": "玉米排骨汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "家常快手鲜甜排骨汤",
        "icon": "玉"
    },
    {
        "mode": "海带排骨汤",
        "name": "海带排骨汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "鲜美补钙，矿物质丰富",
        "icon": "海"
    },
    {
        "mode": "薏米排骨汤",
        "name": "薏米排骨汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "清淡鲜甜，消水利湿",
        "icon": "薏"
    },
    {
        "mode": "干贝排骨汤",
        "name": "干贝排骨汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "25 min",
        "estimatedTime": "约 50 min",
        "desc": "干贝提鲜，益气补身",
        "icon": "干"
    },
    {
        "mode": "海底椰骨汤",
        "name": "海底椰骨汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "润燥止咳，清热降火",
        "icon": "海"
    },
    {
        "mode": "腌笃鲜",
        "name": "腌笃鲜",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "25 min",
        "estimatedTime": "约 50 min",
        "desc": "咸肉、鲜肉与春笋的至味碰撞",
        "icon": "腌"
    },
    {
        "mode": "肉骨茶",
        "name": "肉骨茶",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "南洋特色药膳排骨汤，胡椒药香浓郁",
        "icon": "肉"
    },
    {
        "mode": "罗宋汤",
        "name": "罗宋汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "番茄牛肉蔬菜浓汤，酸甜适口",
        "icon": "罗"
    },
    {
        "mode": "凤梨苦瓜汤",
        "name": "凤梨苦瓜汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "台湾特色，凤梨酸甜化解苦瓜甘微苦",
        "icon": "凤"
    },
    {
        "mode": "浓香煲汤",
        "name": "浓香煲汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "35 min",
        "estimatedTime": "约 55 min",
        "desc": "醇厚高汤，慢熬鲜香",
        "icon": "浓"
    },
    {
        "mode": "韩式土豆汤",
        "name": "韩式土豆汤",
        "category": "soup",
        "categoryName": "名贵靓汤",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "脊骨土豆，韩式浓郁香辣汤底",
        "icon": "韩"
    },
    {
        "mode": "大米饭",
        "name": "大米饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "15 min",
        "estimatedTime": "约 45 min",
        "desc": "柴火精煮，米粒晶莹饱满香软",
        "icon": "大"
    },
    {
        "mode": "标准煮饭",
        "name": "标准煮饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "12 min",
        "estimatedTime": "约 35 min",
        "desc": "劲道饱满，软硬适中日常饭",
        "icon": "标"
    },
    {
        "mode": "极速煮饭",
        "name": "极速煮饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "8 min",
        "estimatedTime": "约 30 min",
        "desc": "强火快蒸，超快解救饥饿",
        "icon": "极"
    },
    {
        "mode": "杂粮饭",
        "name": "杂粮饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "18 min",
        "estimatedTime": "约 30 min",
        "desc": "粗粮营养，富含膳食纤维",
        "icon": "杂"
    },
    {
        "mode": "热米饭",
        "name": "热米饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "8 min",
        "estimatedTime": "约 30 min",
        "desc": "蒸汽温热，饭菜松软如初",
        "icon": "热"
    },
    {
        "mode": "煲仔饭",
        "name": "煲仔饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "12 min",
        "estimatedTime": "约 24 min",
        "desc": "广式腊肠润泽米饭，颗粒饱满弹牙",
        "icon": "煲"
    },
    {
        "mode": "上海菜饭",
        "name": "上海菜饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "咸肉青菜米饭，童年江南滋味",
        "icon": "上"
    },
    {
        "mode": "羊肉手抓饭",
        "name": "羊肉手抓饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "羊油羊肉汁包裹每颗米粒，胡萝卜鲜甜",
        "icon": "羊"
    },
    {
        "mode": "牛肉焖饭",
        "name": "牛肉焖饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "12 min",
        "estimatedTime": "约 24 min",
        "desc": "牛肉洋葱土豆一锅端，省心美味",
        "icon": "牛"
    },
    {
        "mode": "腊味糯米饭",
        "name": "腊味糯米饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "广式生炒糯米饭风味，软糯香浓",
        "icon": "腊"
    },
    {
        "mode": "红豆饭",
        "name": "红豆饭",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "传统红豆赤饭，豆沙香糯营养丰富",
        "icon": "红"
    },
    {
        "mode": "皮蛋瘦肉粥",
        "name": "皮蛋瘦肉粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "广式经典咸粥，绵滑软烂咸鲜浓香",
        "icon": "皮"
    },
    {
        "mode": "杂粮米粥",
        "name": "杂粮米粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "20 min",
        "estimatedTime": "约 50 min",
        "desc": "慢炖细熬，稠滑绵烂养胃健脾",
        "icon": "杂"
    },
    {
        "mode": "美龄粥",
        "name": "美龄粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "17 min",
        "estimatedTime": "约 34 min",
        "desc": "民国名点，山药豆浆百合粳米，美容养颜",
        "icon": "美"
    },
    {
        "mode": "瑶柱排骨粥",
        "name": "瑶柱排骨粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "海味鲜美，粥底丝滑稠密",
        "icon": "瑶"
    },
    {
        "mode": "四红粥",
        "name": "四红粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "17 min",
        "estimatedTime": "约 34 min",
        "desc": "红豆、红枣、花生、红糖，补气养血",
        "icon": "四"
    },
    {
        "mode": "花生黑米粥",
        "name": "花生黑米粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "17 min",
        "estimatedTime": "约 34 min",
        "desc": "高粗纤维，甜润滋补",
        "icon": "花"
    },
    {
        "mode": "南瓜小米粥",
        "name": "南瓜小米粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "5 min",
        "estimatedTime": "约 10 min",
        "desc": "金黄香甜，养胃易消化",
        "icon": "南"
    },
    {
        "mode": "小米粥",
        "name": "小米粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "5 min",
        "estimatedTime": "约 10 min",
        "desc": "迅速熬出厚厚米油，养生早晚餐",
        "icon": "小"
    },
    {
        "mode": "八宝粥",
        "name": "八宝粥",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "30 min",
        "estimatedTime": "约 60 min",
        "desc": "杂粮豆类无需提前泡发，直接炖烂",
        "icon": "八"
    },
    {
        "mode": "咸蛋黄肉粽",
        "name": "咸蛋黄肉粽",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "45 min",
        "estimatedTime": "约 90 min",
        "desc": "粽叶飘香，咸蛋黄流油，大肉酥烂",
        "icon": "咸"
    },
    {
        "mode": "紫米蜜枣粽子",
        "name": "紫米蜜枣粽子",
        "category": "staple",
        "categoryName": "特色主食",
        "pressureTime": "45 min",
        "estimatedTime": "约 90 min",
        "desc": "甜糯清香，米香与枣甜融合",
        "icon": "紫"
    },
    {
        "mode": "银耳雪梨",
        "name": "银耳雪梨",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "70 min",
        "estimatedTime": "约 140 min",
        "desc": "冰糖雪梨银耳出胶极厚，润肺止咳",
        "icon": "银"
    },
    {
        "mode": "木瓜银耳羹",
        "name": "木瓜银耳羹",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "70 min",
        "estimatedTime": "约 140 min",
        "desc": "浓稠起胶，香甜多汁，滋润养生",
        "icon": "木"
    },
    {
        "mode": "银耳莲子羹",
        "name": "银耳莲子羹",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "60 min",
        "estimatedTime": "约 120 min",
        "desc": "养心安神，补血养颜经典糖水",
        "icon": "银"
    },
    {
        "mode": "桃胶皂角米",
        "name": "桃胶皂角米",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "60 min",
        "estimatedTime": "约 120 min",
        "desc": "平民燕窝，植物胶质满满",
        "icon": "桃"
    },
    {
        "mode": "桂花糖藕",
        "name": "桂花糖藕",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "糯米藕软糯甜润，浓浓桂花清香",
        "icon": "桂"
    },
    {
        "mode": "绿豆酿藕节",
        "name": "绿豆酿藕节",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "40 min",
        "estimatedTime": "约 80 min",
        "desc": "绿豆清香解暑，藕节爽脆有咬劲",
        "icon": "绿"
    },
    {
        "mode": "冰糖绿豆汤",
        "name": "冰糖绿豆汤",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "夏日清凉安神解暑，无需提前泡豆",
        "icon": "冰"
    },
    {
        "mode": "绿豆汤",
        "name": "绿豆汤",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "清热解暑，清心利水消夏饮品",
        "icon": "绿"
    },
    {
        "mode": "绿豆莲子汤",
        "name": "绿豆莲子汤",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "莲子粉糯，绿豆开花",
        "icon": "绿"
    },
    {
        "mode": "陈皮红豆汤",
        "name": "陈皮红豆汤",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "17 min",
        "estimatedTime": "约 34 min",
        "desc": "新会陈皮芳香理气，红豆起沙绵密",
        "icon": "陈"
    },
    {
        "mode": "红豆薏米水",
        "name": "红豆薏米水",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "祛湿轻体降脂排毒",
        "icon": "红"
    },
    {
        "mode": "柠檬薏米水",
        "name": "柠檬薏米水",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "17 min",
        "estimatedTime": "约 34 min",
        "desc": "美白消水肿，清爽柠檬风味",
        "icon": "柠"
    },
    {
        "mode": "燕麦牛奶",
        "name": "燕麦牛奶",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "高纤维饱腹感强，减脂健康餐",
        "icon": "燕"
    },
    {
        "mode": "芒果捞黑米",
        "name": "芒果捞黑米",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "25 min",
        "estimatedTime": "约 50 min",
        "desc": "椰汁黑米甜品，软弹香甜",
        "icon": "芒"
    },
    {
        "mode": "话梅芸豆",
        "name": "话梅芸豆",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "20 min",
        "estimatedTime": "约 40 min",
        "desc": "酸甜可口解腻小零嘴",
        "icon": "话"
    },
    {
        "mode": "蒸红薯",
        "name": "蒸红薯",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "15 min",
        "estimatedTime": "约 30 min",
        "desc": "红薯香甜出蜜，软面无筋",
        "icon": "蒸"
    },
    {
        "mode": "土豆泥",
        "name": "土豆泥",
        "category": "dessert",
        "categoryName": "甜品糖水",
        "pressureTime": "12 min",
        "estimatedTime": "约 24 min",
        "desc": "KFC 同款鸡汁奶香土豆泥",
        "icon": "土"
    },
    {
        "mode": "开盖收汁",
        "name": "开盖收汁",
        "category": "basic",
        "categoryName": "辅助程序",
        "pressureTime": "0 min",
        "estimatedTime": "约 15 min",
        "desc": "开盖大火翻炒收汁，浓郁入味",
        "icon": "开"
    },
    {
        "mode": "保温",
        "name": "保温",
        "category": "basic",
        "categoryName": "辅助程序",
        "pressureTime": "0 min",
        "estimatedTime": "持续恒温",
        "desc": "恒温锁鲜，随时享用热腾腾美食",
        "icon": "保"
    },
    {
        "mode": "再加热",
        "name": "再加热",
        "category": "basic",
        "categoryName": "辅助程序",
        "pressureTime": "5 min",
        "estimatedTime": "约 20 min",
        "desc": "微压快速热透，锁住原本风味",
        "icon": "再"
    }
];

var recipeMap = {};
for (var i = 0; i < allRecipes.length; ++i) {
    var r = allRecipes[i];
    recipeMap[r.mode] = r;
}

function getRecipe(mode) {
    if (!mode) return null;
    if (mode === "干贝排骨") mode = "干贝排骨汤";
    if (recipeMap[mode]) return recipeMap[mode];
    return {
        mode: mode,
        name: mode,
        category: "staple",
        categoryName: "特色主食",
        pressureTime: "15 min",
        estimatedTime: "约 45 min",
        desc: "智能压力烹饪",
        icon: getRecipeIcon(mode, "staple")
    };
}

function getCommonRecipes() {
    var list = [];
    for (var i = 0; i < commonRecipeKeys.length; ++i) {
        list.push(getRecipe(commonRecipeKeys[i]));
    }
    return list;
}

function getRecipesByCategory(catId, keyword) {
    var kw = (keyword || "").trim().toLowerCase();
    var res = [];
    for (var i = 0; i < allRecipes.length; ++i) {
        var item = allRecipes[i];
        if (catId && catId !== "all" && item.category !== catId) {
            continue;
        }
        if (kw !== "" && item.name.toLowerCase().indexOf(kw) === -1 && item.desc.toLowerCase().indexOf(kw) === -1) {
            continue;
        }
        res.push(item);
    }
    return res;
}

// 核心大厨精细化做法与食材数据库
var recipeDetailsDB = {
    "杂粮饭": {
        "practice": "高压焖煮",
        "ingredients": [
            "大米 150克",
            "黑米 30克",
            "糙米 30克",
            "红豆 20克",
            "燕麦仁 20克",
            "清水 300毫升"
        ],
        "steps": [
            "将大米与各类粗粮混合淘洗干净，杂粮吸水较慢，建议温水浸泡20分钟；",
            "将浸泡好的杂粮米倒入电压力锅内胆，加水至米水比约1:1.2刻度；",
            "盖上锅盖并旋转手柄至锁定位置，选择【杂粮饭】烹饪模式；",
            "程序自动完成聚能升温与保压焖煮（默认保压25分钟），深层软化杂粮硬壳；",
            "泄压完成后安全开盖，用饭勺将米饭自下而上轻柔翻拌均匀排湿，软糯清香。"
        ],
        "tips": "电压力锅的高温高压环境可快速瓦解糙米与豆类外皮的坚硬纤维，免除传统电饭锅需浸泡数小时的繁琐。"
    },
    "小米粥": {
        "practice": "微压慢熬",
        "ingredients": [
            "优质黄小米 80克",
            "纯净水 800毫升",
            "枸杞 10粒（出锅点缀）"
        ],
        "steps": [
            "黄小米用清水轻轻淘洗一遍，切勿用力揉搓以免表面富含的米脂流失；",
            "将淘好的小米倒入内胆，按米水比1:10注入纯净水；",
            "旋紧锅盖锁定手柄，启动【小米粥】烹饪程序；",
            "电压力锅在微压密闭状态下连续沸腾熬煮20分钟，充分激发小米米油；",
            "烹饪结束排汽泄压后开盖，撒入洗净的枸杞，静置片刻表面即可结出金黄丰润的米油皮。"
        ],
        "tips": "煮小米粥水量要一次加足，中途切勿开盖加水，微压聚热能让粥水稠滑米油浓厚，养胃温中。"
    },
    "大米饭": {
        "practice": "聚能焖香",
        "ingredients": [
            "精选大米 300克",
            "清水 360毫升",
            "植物油 2滴（增亮防粘）"
        ],
        "steps": [
            "大米淘洗2次至水质渐清，沥干水分备用；",
            "将大米平铺于压力锅内胆中，加入1.2倍清水，滴入2滴植物油使米粒晶莹润泽；",
            "盖好上盖并旋转至锁止状态，确认启动【大米饭】程序；",
            "压力锅智能加压升温，水分高压瞬间透芯，米芯完全熟化淀粉糖化；",
            "烹饪完成后自动泄压开盖，用饭勺打松翻匀静置1分钟，米香扑鼻，粒粒油亮。"
        ],
        "tips": "新米吸水略少（米水比1:1.1），陈米吸水略多（米水比1:1.25），高压焖饭锁水保香效果远胜普通蒸煮。"
    },
    "土豆炖排骨": {
        "practice": "浓香高压炖",
        "ingredients": [
            "精选肋排 500克",
            "黄心土豆 2个（约300克）",
            "生姜 15克",
            "大葱 1根",
            "生抽 25克",
            "老抽 10克",
            "蚝油 15克",
            "料酒 20克",
            "冰糖 15克",
            "温水 200毫升"
        ],
        "steps": [
            "排骨斩成4厘米小块，冷水下锅加入葱段姜片和料酒，大火焯水撇清浮沫，温水冲洗干净沥干；",
            "土豆去皮洗净，切滚刀大块备用（大块耐炖不易破碎化汤）；",
            "将焯好水的排骨放入压力锅内胆，加入姜片、生抽、老抽、蚝油、料酒、冰糖，倒入200毫升温水拌匀；",
            "合盖锁死手柄，选择【土豆炖排骨】模式，开始高压烹饪20分钟；",
            "烹饪结束后等待安全泄压，开盖加入土豆块，可再次加压5分钟或开启收汁功能让汤汁红亮裹汁装盘。"
        ],
        "tips": "压力锅烹饪锁水保汤，切忌加过多清水，200毫升即可达到浓汁脱骨的效果；土豆后放可保持形态软糯不散。"
    },
    "干贝排骨汤": {
        "practice": "鲜醇慢煲",
        "ingredients": [
            "新鲜猪肋排 500克",
            "干贝 30克",
            "生姜 15克",
            "料酒 15克",
            "食盐 适量",
            "白胡椒粉 少许",
            "纯净水 1200毫升"
        ],
        "steps": [
            "干贝提前用温水加少许料酒浸泡15分钟软化，撕成细丝，保留浸泡原汁；",
            "排骨洗净冷水下锅焯水3分钟去血污，捞出用温水洗净控水；",
            "将排骨、干贝及滤去底渣的干贝原汤倒入压力锅内胆，投入生姜片，注入1200毫升纯净水；",
            "旋转闭合锅盖并锁紧手柄，启动【干贝排骨汤】煲汤程序，设定保压25分钟；",
            "程序结束自动安全降压排汽后开盖，加入适量食用盐和少许白胡椒粉调味，汤清见底，鲜醇滋润。"
        ],
        "tips": "干贝富含丰富天然鲜味氨基酸，煲汤无需任何味精鸡精，咸鲜爽口，与排骨荤香相得益彰。"
    },
    "番茄牛腩": {
        "practice": "酸爽高压煨",
        "ingredients": [
            "新鲜牛腩 600克",
            "沙瓤番茄 3个",
            "洋葱 半个",
            "生姜 20克",
            "番茄沙司 30克",
            "生抽 20克",
            "料酒 25克",
            "冰糖 15克",
            "八角 1个",
            "香叶 2片"
        ],
        "steps": [
            "牛腩洗净切成3厘米见方大块，冷水入锅加料酒姜片焯水5分钟彻底去除血水，捞出温水洗净；",
            "番茄开水烫烫去皮，2个切碎丁（用于煮融成浓厚酸甜汤底），1个切大滚刀块出锅前使用；",
            "锅底铺上洋葱碎和番茄碎丁，放入焯好的牛腩块，加入生抽、番茄酱、冰糖、姜片、八角香叶，加入半碗温水；",
            "合上锅盖拧紧手柄，启动【番茄牛腩】模式，高压保压炖煮30分钟；",
            "泄压开盖后投入大块番茄，开启【收汁】程序翻匀收浓汤汁3分钟，酸爽开胃，牛腩软烂入味。"
        ],
        "tips": "牛腩筋膜丰富，电压力锅高压能快速软化结缔组织，番茄的天然果酸能进一步加速牛肉软化，汤浓肉酥。"
    },
    "豆类蹄筋": {
        "practice": "强压软糯",
        "ingredients": [
            "牛蹄筋或鲜猪蹄 500克",
            "大黄豆/白芸豆 150克",
            "生姜 20克",
            "大葱 1根",
            "生抽 30克",
            "老抽 15克",
            "料酒 30克",
            "冰糖 20克",
            "八角 2枚",
            "桂皮 1小片"
        ],
        "steps": [
            "豆类淘洗干净，沥干水分（高压锅无需提前泡发一整夜，直接加压亦可粉糯）；",
            "蹄筋或猪蹄切小段，冷水下锅加入料酒姜片大火焯透5分钟，捞出温水冲洗干净沥干；",
            "将黄豆先铺在内胆底层，放上蹄筋块，加入大葱段、姜片、八角、桂皮；",
            "调入生抽、老抽、料酒、冰糖，加入适量清水至刚好没过食材；",
            "旋转合盖锁死手柄，开启【豆类蹄筋】模式，强力保压40分钟；",
            "烹饪结束后待气压自然泄尽开盖，蹄筋胶质晶莹软滑，黄豆粉沙绵软，入口化渣。"
        ],
        "tips": "蹄筋韧性极强，普通锅需炖煮4小时以上，电压力锅的高压能将其胶原蛋白彻底软化成糯弹口感。"
    },
    "黄焖鸡": {
        "practice": "秘制黄焖",
        "ingredients": [
            "新鲜鸡腿肉或三黄鸡 600克",
            "干香菇 8朵",
            "青脆椒 1个",
            "红甜椒 1个",
            "生姜 20克",
            "大蒜 5瓣",
            "生抽 25克",
            "蚝油 20克",
            "黄豆酱 15克",
            "老抽 10克",
            "冰糖 10克",
            "料酒 20克"
        ],
        "steps": [
            "干香菇温水泡发后洗净切厚片，保留澄清的香菇原汤半碗；青红椒切菱形块；",
            "鸡肉剁成核桃大小块，冷水浸泡15分钟除血水后捞出沥干；",
            "鸡块放入压力锅内胆，加入姜片蒜瓣、香菇片、生抽、老抽、蚝油、黄豆酱、冰糖、料酒与香菇原汤抓匀；",
            "合紧锅盖锁好手柄，选择【黄焖鸡】模式，保压烹饪15分钟；",
            "完全泄压后开盖，投入青红椒块，利用锅底沸热余温或开盖收汁翻炒1分钟，鸡肉嫩滑多汁，香气扑鼻。"
        ],
        "tips": "以泡香菇的原汤做水基是黄焖鸡浓郁鲜美的核心秘诀；鸡肉不宜久压，15分钟保压刚好保持鲜嫩多汁。"
    },
    "红烧肉": {
        "practice": "红烧",
        "ingredients": [
            "五花肉 500克",
            "料酒 50克",
            "生抽 20克",
            "老抽 15克",
            "蚝油 20克",
            "冰糖 20克",
            "生姜 20克",
            "香葱 20克",
            "八角 1枚",
            "香叶 2片"
        ],
        "steps": [
            "选肥瘦相间的三层带皮五花肉，洗净切成约2.5厘米的均匀方块；",
            "冷水入锅加入姜片与料酒大火煮沸焯水3分钟，撇去浮沫捞出温水洗净沥干；",
            "处理好的肉块放入锅中，加入葱结、姜片、八角、香叶、生抽、老抽、蚝油、冰糖和料酒翻拌均匀；",
            "盖好锅盖锁死压力阀，启动【红烧肉】模式，高压保压25分钟；",
            "泄压完成后开盖，夹出葱姜香料，肉质红亮酥烂，肥而不腻，汤汁拌饭尤佳。"
        ],
        "tips": "高压红烧肉无需额外加水，五花肉自身油脂配合料酒酱汁在密闭高压下自成醇厚芡汁。"
    }
};

// 获取菜谱详尽做法指引与食材配方
function getRecipeDetail(recipeObj) {
    if (!recipeObj) return null;
    var name = recipeObj.name || recipeObj.mode || "";
    if (name === "干贝排骨") name = "干贝排骨汤";
    
    // 1. 若有特定专门配置，直接使用
    if (recipeDetailsDB[name]) {
        var db = recipeDetailsDB[name];
        return {
            name: name,
            practice: db.practice || recipeObj.categoryName || "大厨特调",
            desc: recipeObj.desc || "",
            ingredients: db.ingredients || [],
            steps: db.steps || [],
            tips: db.tips || ""
        };
    }

    // 2. 通用大厨做法智能生成器（针对 108 道全量菜谱）
    var cat = recipeObj.category || "meat";
    var catName = recipeObj.categoryName || "特色料理";
    var pt = recipeObj.pressureTime || "20 min";
    var ingredients = [];
    var steps = [];
    var tips = "";
    var practice = "智能压力烹饪";

    if (cat === "meat") {
        practice = name.indexOf("红烧") !== -1 ? "浓郁红烧" : (name.indexOf("炖") !== -1 ? "酥烂慢炖" : "高压酱卤");
        ingredients = [
            "主食材（精选肉类） 500克",
            "生姜 15克",
            "大葱 1根",
            "生抽 25克",
            "老抽 10克",
            "料酒 25克",
            "冰糖 15克",
            "八角 1枚",
            "香叶 2片",
            "适量温水"
        ];
        steps = [
            "将肉类食材切成适口块状，冷水下锅加入葱姜与料酒焯水去腥，捞出温水洗净沥干；",
            "将处理好的食材放入压力锅内胆，加入葱段、姜片、八角香叶等香料；",
            "调入生抽、老抽、料酒与冰糖，加入适量清水搅拌均匀；",
            "合盖并锁紧安全手柄，启动【" + name + "】烹饪程序（建议保压" + pt + "）；",
            "烹饪完毕完全泄压后开盖，依喜好开启收汁或直接出锅装盘享用。"
        ];
        tips = "肉类经电压力锅高压渗透，能瞬间锁住肉汁并软化结缔组织，肉质软烂脱骨而不柴。";
    } else if (cat === "poultry") {
        practice = "鲜香焖炖";
        ingredients = [
            "主禽肉食材 500~600克",
            "生姜 20克",
            "大蒜 5瓣",
            "生抽 25克",
            "蚝油 15克",
            "老抽 10克",
            "料酒 20克",
            "辅料配菜 适量",
            "适量高汤或温水"
        ];
        steps = [
            "禽肉斩成均匀块状，浸泡清洗干净血水并沥干备用；",
            "肉块倒入压力锅内胆，加入葱姜蒜粒及配菜；",
            "调入料酒、生抽、老抽与蚝油拌匀调味，加入少许温水；",
            "合盖锁严手柄，选择【" + name + "】烹饪模式；",
            "烹饪程序结束且安全排汽完成后开盖，翻匀装盘，鲜滑香浓。"
        ];
        tips = "禽肉烹饪时间不宜过长，电压力锅高压聚热保水性强，能完美保留鸡鸭肉的鲜嫩滑多汁。";
    } else if (cat === "soup") {
        practice = "原汁靓汤";
        ingredients = [
            "鲜肉骨主料 500克",
            "精选炖汤辅料 适量",
            "生姜 15克",
            "料酒 15克",
            "食盐 适量",
            "纯净水 1200~1500毫升"
        ];
        steps = [
            "将骨肉主料斩段，冷水下锅焯水3分钟撇去浮沫，捞出温水冲洗干净；",
            "洗净的食材与滋补辅料一同放入压力锅内胆，放入生姜片；",
            "注入足量纯净水（水位勿超过锅内最高煲汤刻度线）；",
            "盖严锅盖旋转锁死，启动【" + name + "】模式进行高压慢煲；",
            "泄压完成后揭盖，加入适量食用盐调味即可趁热享用。"
        ];
        tips = "煲汤讲究一气呵成，盐要在出锅泄压后再加，避免肉质中蛋白质过早凝固影响鲜香释出。";
    } else if (cat === "staple") {
        practice = "聚能焖香";
        ingredients = [
            "优质米粮/面食 250~300克",
            "搭配辅料 适量",
            "纯净水 按米水比约1:1.1~1.2",
            "植物油 少许"
        ];
        steps = [
            "米粮淘洗干净，沥去水分；若有杂粮可稍加温水湿润；",
            "主辅料一同铺入内胆，按刻度加入对应比例清水，滴入少许植物油；",
            "旋转合上锅盖至锁定位置，选择【" + name + "】程序；",
            "微压/高压精准温控加温，米粒充分吸水膨胀淀粉糊化；",
            "泄压开盖后用饭勺自下而上轻柔打松排湿，口感粒粒分明松软可口。"
        ];
        tips = "高压密闭焖煮能锁住大米自身的原香与水分，出锅后适当翻松排湿可让口感更弹糯有嚼劲。";
    } else if (cat === "dessert") {
        practice = "清甜慢熬";
        ingredients = [
            "甜品豆谷/银耳主料 100克",
            "老冰糖/红糖 30~50克",
            "红枣/枸杞 适量",
            "纯净水 1000毫升"
        ];
        steps = [
            "食材清洗干净，银耳撕成小朵，豆类洗净沥干；",
            "将处理好的食材与配料加入压力锅内胆，注入清水；",
            "锁紧锅盖手柄，启动【" + name + "】程序；",
            "高压密闭升温熬煮，促使豆类起沙开花、银耳快速出胶；",
            "排汽泄压后开盖，加入冰糖或红糖搅拌至溶化，温润清润。"
        ];
        tips = "甜品类胶质丰富，高压状态下熬煮能快速析出植物胶质与豆沙，细腻顺滑省时省力。";
    } else {
        practice = "智能微压";
        ingredients = [
            "待加热或处理食材 适量",
            "少许清水或食用油 适量"
        ];
        steps = [
            "将食材平整放入内胆中；",
            "合上压力锅盖并旋转手柄至安全锁止位置；",
            "启动对应程序，电压力锅将自动微压加温；",
            "烹饪结束后开盖享用。"
        ];
        tips = "操作时请确保锅盖旋转锁止到位，泄压前切勿强制开盖。";
    }

    return {
        name: name,
        practice: practice,
        desc: recipeObj.desc || "智能压力烹饪",
        ingredients: ingredients,
        steps: steps,
        tips: tips
    };
}

// 格式化剩余时间：有小时显示“X小时X分钟X秒”，无小时隐藏小时显示“X分钟X秒”
function formatCookerTime(rawVal) {
    if (rawVal === undefined || rawVal === null) return "--";

    // 1. 若传入的是数据模型对象 (如 liveModel, actionModel 或 HA 原生 attributes 字典)
    if (typeof rawVal === "object") {
        var isKeepWarm = Boolean(rawVal.cookerIsKeepWarm || rawVal.is_keep_warm ||
                                 (rawVal.phase && rawVal.phase.indexOf("保温") !== -1) ||
                                 (rawVal.cookerPhase && rawVal.cookerPhase.indexOf("保温") !== -1) ||
                                 (rawVal.state && rawVal.state.indexOf("保温") !== -1) ||
                                 (rawVal.stateText && rawVal.stateText.indexOf("保温") !== -1));

        // 若处于保温状态，优先提取已保温时间属性
        if (isKeepWarm) {
            var kwStr = rawVal.cookerKeepWarmFormatted || rawVal.keep_warm_formatted;
            if (kwStr) {
                return formatChineseTimeString(kwStr);
            }
            var kwSec = rawVal.cookerKeepWarmSeconds !== undefined ? rawVal.cookerKeepWarmSeconds : rawVal.keep_warm_seconds;
            if (typeof kwSec === "number" && kwSec > 0) {
                return formatFromSeconds(kwSec);
            }
        }

        var hms = rawVal.cookerRemainingTimeHms || rawVal.remaining_time_hms;
        if (hms && hms !== "00:00:00") {
            return formatHmsString(hms);
        }
        var zh = rawVal.cookerRemainingTimeFormatted || rawVal.remaining_time_formatted;
        if (zh && zh !== "0秒" && zh !== "00:00:00") {
            return formatChineseTimeString(zh);
        }
        var sec = rawVal.cookerTotalRemainingSeconds !== undefined ? rawVal.cookerTotalRemainingSeconds :
                  (rawVal.total_remaining_seconds !== undefined ? rawVal.total_remaining_seconds :
                  (rawVal.cookerRemainingSeconds !== undefined ? rawVal.cookerRemainingSeconds : rawVal.remaining_seconds));
        if (typeof sec === "number" && sec > 0) {
            return formatFromSeconds(sec);
        }
        var hours = rawVal.cookerRemainingHours !== undefined ? rawVal.cookerRemainingHours : rawVal.remaining_hours;
        var mins = rawVal.cookerRemainingMinutes !== undefined ? rawVal.cookerRemainingMinutes : rawVal.remaining_minutes;
        var secs = rawVal.cookerRemainingSeconds !== undefined ? rawVal.cookerRemainingSeconds : rawVal.remaining_seconds;
        if (hours !== undefined && mins !== undefined && secs !== undefined) {
            var h = parseInt(hours, 10) || 0;
            var m = parseInt(mins, 10) || 0;
            var s = parseInt(secs, 10) || 0;
            if (h > 0 || m > 0 || s > 0) {
                return formatHms(h, m, s);
            }
        }
        if (rawVal.cookerLeftTime !== undefined && rawVal.cookerLeftTime !== null) {
            return formatCookerTime(rawVal.cookerLeftTime);
        }
        return isKeepWarm ? "保温中" : "--";
    }

    var str = String(rawVal).trim();
    if (str === "" || str === "--" || str === "0" || str === "0.0" || str === "900") return "--";

    // 2. 中文字符串格式 (例如 HA 上报 "1分29秒", "1小时1分5秒", "45分钟")
    if (str.indexOf("分") !== -1 || str.indexOf("小时") !== -1 || str.indexOf("秒") !== -1) {
        return formatChineseTimeString(str);
    }

    // 3. 检查是否包含冒号格式 (HH:MM:SS 或 MM:SS)
    if (str.indexOf(":") !== -1) {
        return formatHmsString(str);
    }

    // 4. 纯数字
    var num = parseFloat(str);
    if (isNaN(num) || num <= 0) {
        return "--";
    }

    // 浮点数 (如 7.66666666, 1.48333333): 单位为分钟，换算为精确秒数
    if (str.indexOf(".") !== -1) {
        return formatFromSeconds(Math.round(num * 60));
    }

    // 整数: 若数值大于 240 (在单次烹饪场景下非正常分钟)，视为秒数
    if (num > 240) {
        return formatFromSeconds(num);
    }

    // 整数常规烹饪分钟 (如 45, 60, 15)
    return formatFromSeconds(num * 60);
}

function formatFromSeconds(totalSec) {
    var sec = Math.round(Number(totalSec));
    if (isNaN(sec) || sec <= 0) return "即将完成";
    var h = Math.floor(sec / 3600);
    var rem = sec % 3600;
    var m = Math.floor(rem / 60);
    var s = rem % 60;
    return formatHms(h, m, s);
}

function formatHmsString(hmsStr) {
    var parts = String(hmsStr).trim().split(":");
    if (parts.length === 3) {
        var h = parseInt(parts[0], 10) || 0;
        var m = parseInt(parts[1], 10) || 0;
        var s = parseInt(parts[2], 10) || 0;
        return formatHms(h, m, s);
    } else if (parts.length === 2) {
        var m = parseInt(parts[0], 10) || 0;
        var s = parseInt(parts[1], 10) || 0;
        return formatHms(0, m, s);
    }
    return hmsStr;
}

function formatChineseTimeString(zhStr) {
    var h = 0, m = 0, s = 0;
    var matchH = zhStr.match(/(\d+)\s*小时/);
    if (matchH) h = parseInt(matchH[1], 10) || 0;
    var matchM = zhStr.match(/(\d+)\s*(?:分钟|分)/);
    if (matchM) m = parseInt(matchM[1], 10) || 0;
    var matchS = zhStr.match(/(\d+)\s*秒/);
    if (matchS) s = parseInt(matchS[1], 10) || 0;

    if (!matchH && !matchM && !matchS) return zhStr;
    return formatHms(h, m, s);
}

function formatHms(hours, minutes, seconds) {
    if (hours > 0) {
        var res = hours + "小时";
        if (minutes > 0) {
            res += minutes + "分钟";
        }
        if (seconds > 0) {
            res += seconds + "秒";
        }
        return res;
    }

    // 没有小时，隐藏小时
    if (minutes > 0) {
        var res = minutes + "分钟";
        if (seconds > 0) {
            res += seconds + "秒";
        }
        return res;
    }

    if (seconds > 0) {
        return seconds + "秒";
    }

    return "即将完成";
}

function getCategoryIcon(id) {
    for (var i = 0; i < categories.length; ++i) {
        if (categories[i].id === id) return categories[i].icon;
    }
    return "qrc:/icons/cooker-chef.svg";
}

function getRecipeIcon(name, category) {
    var n = name || "";
    var c = category || "";

    // 1. 前 8 个核心常用食谱专属高精度图标（精准 1v1 严格对应）
    if (n === "杂粮饭" || n === "糙米饭") {
        return "qrc:/icons/recipe-multigrain-rice.svg";
    }
    if (n === "小米粥") {
        return "qrc:/icons/recipe-millet-congee.svg";
    }
    if (n === "大米饭" || n === "米饭" || n === "精煮饭" || n === "快煮饭") {
        return "qrc:/icons/recipe-white-rice.svg";
    }
    if (n === "土豆炖排骨" || (n.indexOf("土豆") !== -1 && n.indexOf("排骨") !== -1)) {
        return "qrc:/icons/recipe-pork-ribs.svg";
    }
    if (n === "干贝排骨汤" || (n.indexOf("干贝") !== -1 && n.indexOf("排骨") !== -1) || n.indexOf("干贝") !== -1) {
        return "qrc:/icons/recipe-scallop-soup.svg";
    }
    if (n === "番茄牛腩" || (n.indexOf("番茄") !== -1 && n.indexOf("牛") !== -1)) {
        return "qrc:/icons/recipe-beef-tomato.svg";
    }
    if (n === "豆类蹄筋" || n.indexOf("蹄筋") !== -1) {
        return "qrc:/icons/recipe-tendon-beans.svg";
    }
    if (n === "黄焖鸡" || n.indexOf("黄焖") !== -1) {
        return "qrc:/icons/recipe-braised-chicken.svg";
    }

    // 2. 其它菜谱根据品类分类匹配
    if (c === "meat" || n.indexOf("肉") !== -1 || n.indexOf("羊") !== -1) {
        return "qrc:/icons/cooker-meat.svg";
    }
    if (c === "poultry" || n.indexOf("鸡") !== -1 || n.indexOf("鸭") !== -1 || n.indexOf("禽") !== -1) {
        return "qrc:/icons/cooker-chicken.svg";
    }
    if (c === "soup" || n.indexOf("汤") !== -1 || n.indexOf("煲") !== -1) {
        return "qrc:/icons/cooker-soup.svg";
    }
    if (n.indexOf("粥") !== -1) {
        return "qrc:/icons/cooker-congee.svg";
    }
    if (c === "staple" || n.indexOf("饭") !== -1 || n.indexOf("米") !== -1 || n.indexOf("粮") !== -1) {
        return "qrc:/icons/cooker-rice.svg";
    }
    if (c === "dessert" || n.indexOf("甜") !== -1 || n.indexOf("膏") !== -1 || n.indexOf("银耳") !== -1 || n.indexOf("糖") !== -1) {
        return "qrc:/icons/cooker-dessert.svg";
    }
    if (c === "basic" || n.indexOf("洗") !== -1 || n.indexOf("蒸") !== -1 || n.indexOf("洁") !== -1) {
        return "qrc:/icons/cooker-gear.svg";
    }
    return "qrc:/icons/cooker-rice.svg";
}
