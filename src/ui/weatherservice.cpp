#include "weatherservice.h"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrlQuery>

namespace {

struct ConditionInfo {
    QString name;
    QString emoji;
};

const QHash<QString, ConditionInfo>& conditionMap()
{
    static const QHash<QString, ConditionInfo> map = {
        {QStringLiteral("d00"), {QStringLiteral("晴"), QStringLiteral("☀️")}},
        {QStringLiteral("d01"), {QStringLiteral("多云"), QStringLiteral("🌤")}},
        {QStringLiteral("d02"), {QStringLiteral("阴"), QStringLiteral("☁️")}},
        {QStringLiteral("d03"), {QStringLiteral("阵雨"), QStringLiteral("🌦")}},
        {QStringLiteral("d04"), {QStringLiteral("雷阵雨"), QStringLiteral("⛈")}},
        {QStringLiteral("d05"), {QStringLiteral("雷阵雨伴有冰雹"), QStringLiteral("⛈")}},
        {QStringLiteral("d06"), {QStringLiteral("雨夹雪"), QStringLiteral("🌨")}},
        {QStringLiteral("d07"), {QStringLiteral("小雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d08"), {QStringLiteral("中雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d09"), {QStringLiteral("大雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d10"), {QStringLiteral("暴雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d11"), {QStringLiteral("大暴雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d12"), {QStringLiteral("特大暴雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d13"), {QStringLiteral("阵雪"), QStringLiteral("🌨")}},
        {QStringLiteral("d14"), {QStringLiteral("小雪"), QStringLiteral("❄️")}},
        {QStringLiteral("d15"), {QStringLiteral("中雪"), QStringLiteral("❄️")}},
        {QStringLiteral("d16"), {QStringLiteral("大雪"), QStringLiteral("❄️")}},
        {QStringLiteral("d17"), {QStringLiteral("暴雪"), QStringLiteral("❄️")}},
        {QStringLiteral("d18"), {QStringLiteral("雾"), QStringLiteral("🌁")}},
        {QStringLiteral("d19"), {QStringLiteral("冻雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d20"), {QStringLiteral("沙尘暴"), QStringLiteral("🌪")}},
        {QStringLiteral("d21"), {QStringLiteral("小到中雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d22"), {QStringLiteral("中到大雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d23"), {QStringLiteral("大到暴雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d24"), {QStringLiteral("暴雨到大暴雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d25"), {QStringLiteral("大暴雨到特大暴雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d26"), {QStringLiteral("小到中雪"), QStringLiteral("🌨")}},
        {QStringLiteral("d27"), {QStringLiteral("中到大雪"), QStringLiteral("❄️")}},
        {QStringLiteral("d28"), {QStringLiteral("大到暴雪"), QStringLiteral("❄️")}},
        {QStringLiteral("d29"), {QStringLiteral("浮尘"), QStringLiteral("🌫")}},
        {QStringLiteral("d30"), {QStringLiteral("扬沙"), QStringLiteral("🌫")}},
        {QStringLiteral("d31"), {QStringLiteral("强沙尘暴"), QStringLiteral("🌪")}},
        {QStringLiteral("d32"), {QStringLiteral("浓雾"), QStringLiteral("🌁")}},
        {QStringLiteral("d49"), {QStringLiteral("强浓雾"), QStringLiteral("🌁")}},
        {QStringLiteral("d53"), {QStringLiteral("霾"), QStringLiteral("🌫")}},
        {QStringLiteral("d54"), {QStringLiteral("中度霾"), QStringLiteral("🌫")}},
        {QStringLiteral("d55"), {QStringLiteral("重度霾"), QStringLiteral("🌫")}},
        {QStringLiteral("d56"), {QStringLiteral("严重霾"), QStringLiteral("🌫")}},
        {QStringLiteral("d57"), {QStringLiteral("大雾"), QStringLiteral("🌁")}},
        {QStringLiteral("d58"), {QStringLiteral("特强浓雾"), QStringLiteral("🌁")}},
        {QStringLiteral("d301"), {QStringLiteral("雨"), QStringLiteral("🌧")}},
        {QStringLiteral("d302"), {QStringLiteral("雪"), QStringLiteral("❄️")}}
    };
    return map;
}

QString weatherEmoji(const QString &code, const QString &rawName)
{
    Q_UNUSED(code);
    Q_UNUSED(rawName);
    return QString();
}

QString weatherIconSource(const QString &code, const QString &rawName)
{
    QString c = code.trimmed();
    if (!c.isEmpty() && !c.startsWith(QLatin1Char('d'))) {
        c = QStringLiteral("d") + c;
    }
    if (c == QStringLiteral("d00")) return QStringLiteral("qrc:/icons/weather-sunny.svg");
    if (c == QStringLiteral("d01")) return QStringLiteral("qrc:/icons/weather-cloudy.svg");
    if (c == QStringLiteral("d02")) return QStringLiteral("qrc:/icons/weather-overcast.svg");
    if (c == QStringLiteral("d04") || c == QStringLiteral("d05")) return QStringLiteral("qrc:/icons/weather-thunder.svg");
    if (c == QStringLiteral("d03") || c == QStringLiteral("d07") || c == QStringLiteral("d08") || c == QStringLiteral("d09") || c == QStringLiteral("d10") || c == QStringLiteral("d11") || c == QStringLiteral("d12") || c == QStringLiteral("d19") || c == QStringLiteral("d21") || c == QStringLiteral("d22") || c == QStringLiteral("d23") || c == QStringLiteral("d24") || c == QStringLiteral("d25") || c == QStringLiteral("d301")) {
        return QStringLiteral("qrc:/icons/weather-rain.svg");
    }
    if (c == QStringLiteral("d06") || c == QStringLiteral("d13") || c == QStringLiteral("d14") || c == QStringLiteral("d15") || c == QStringLiteral("d16") || c == QStringLiteral("d17") || c == QStringLiteral("d26") || c == QStringLiteral("d27") || c == QStringLiteral("d28") || c == QStringLiteral("d302")) {
        return QStringLiteral("qrc:/icons/weather-snow.svg");
    }
    if (c == QStringLiteral("d18") || c == QStringLiteral("d20") || c == QStringLiteral("d29") || c == QStringLiteral("d30") || c == QStringLiteral("d31") || c == QStringLiteral("d32") || c == QStringLiteral("d49") || c == QStringLiteral("d53") || c == QStringLiteral("d54") || c == QStringLiteral("d55") || c == QStringLiteral("d56") || c == QStringLiteral("d57") || c == QStringLiteral("d58")) {
        return QStringLiteral("qrc:/icons/weather-fog.svg");
    }

    if (rawName.contains(QStringLiteral("晴"))) return QStringLiteral("qrc:/icons/weather-sunny.svg");
    if (rawName.contains(QStringLiteral("雷"))) return QStringLiteral("qrc:/icons/weather-thunder.svg");
    if (rawName.contains(QStringLiteral("雪"))) return QStringLiteral("qrc:/icons/weather-snow.svg");
    if (rawName.contains(QStringLiteral("雨"))) return QStringLiteral("qrc:/icons/weather-rain.svg");
    if (rawName.contains(QStringLiteral("阴"))) return QStringLiteral("qrc:/icons/weather-overcast.svg");
    if (rawName.contains(QStringLiteral("云"))) return QStringLiteral("qrc:/icons/weather-cloudy.svg");
    if (rawName.contains(QStringLiteral("雾")) || rawName.contains(QStringLiteral("霾")) || rawName.contains(QStringLiteral("沙")) || rawName.contains(QStringLiteral("尘"))) {
        return QStringLiteral("qrc:/icons/weather-fog.svg");
    }
    return QStringLiteral("qrc:/icons/weather-cloudy.svg");
}

QString weatherName(const QString &code, const QString &rawName)
{
    if (!rawName.trimmed().isEmpty()) {
        return rawName.trimmed();
    }
    QString c = code.trimmed();
    if (!c.isEmpty() && !c.startsWith(QLatin1Char('d'))) {
        c = QStringLiteral("d") + c;
    }
    if (conditionMap().contains(c)) {
        return conditionMap().value(c).name;
    }
    return QStringLiteral("晴");
}

QString weekdayString(int dayOfWeek)
{
    static const QStringList days = {
        QString(), QStringLiteral("周一"), QStringLiteral("周二"), QStringLiteral("周三"),
        QStringLiteral("周四"), QStringLiteral("周五"), QStringLiteral("周六"), QStringLiteral("周日")
    };
    if (dayOfWeek >= 1 && dayOfWeek <= 7) {
        return days.at(dayOfWeek);
    }
    return QStringLiteral("周一");
}

QString normalizeTemp(const QString &raw)
{
    QString s = raw.trimmed();
    s.remove(QStringLiteral("℃")).remove(QStringLiteral("°C")).remove(QStringLiteral("°"));
    bool ok = false;
    const double d = s.toDouble(&ok);
    if (ok) {
        return QString::number(qRound(d));
    }
    return s.isEmpty() ? QStringLiteral("--") : s;
}

QNetworkRequest makeChinaWeatherRequest(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36"));
    req.setRawHeader("Referer", "https://m.weather.com.cn/");
    return req;
}

QJsonObject extractJsonObject(const QString &text, const QString &varName)
{
    const QRegularExpression re(QStringLiteral("\\b%1\\s*=\\s*\\{").arg(QRegularExpression::escape(varName)));
    const QRegularExpressionMatch match = re.match(text);
    if (!match.hasMatch()) {
        return QJsonObject();
    }

    const int startIdx = match.capturedEnd() - 1;
    int depth = 0;
    bool inString = false;
    QChar quoteChar;
    bool escaping = false;

    for (int i = startIdx; i < text.length(); ++i) {
        const QChar c = text.at(i);
        if (inString) {
            if (escaping) {
                escaping = false;
            } else if (c == QLatin1Char('\\')) {
                escaping = true;
            } else if (c == quoteChar) {
                inString = false;
            }
        } else {
            if (c == QLatin1Char('"') || c == QLatin1Char('\'')) {
                inString = true;
                quoteChar = c;
            } else if (c == QLatin1Char('{')) {
                depth++;
            } else if (c == QLatin1Char('}')) {
                depth--;
                if (depth == 0) {
                    const QString jsonStr = text.mid(startIdx, i - startIdx + 1);
                    return QJsonDocument::fromJson(jsonStr.toUtf8()).object();
                }
            }
        }
    }
    return QJsonObject();
}

} // namespace

WeatherService::WeatherService(QObject *parent)
    : QObject(parent)
{
    connect(&m_refreshTimer, &QTimer::timeout, this, &WeatherService::refresh);
}

QString WeatherService::summary() const
{
    return m_summary;
}

QString WeatherService::location() const
{
    return m_location;
}

QString WeatherService::detail() const
{
    return m_detail;
}

QVariantList WeatherService::forecast() const
{
    return m_forecast;
}

QVariantMap WeatherService::current() const
{
    return m_current;
}

void WeatherService::applyConfig(const WeatherConfig &config, bool networkOnline)
{
    if (m_config.city != config.city || m_config.areaId != config.areaId) {
        m_resolvedAreaId.clear();
        m_resolvedCity.clear();
        m_resolvedProvince.clear();
    }
    m_config = config;
    m_networkOnline = networkOnline;
    scheduleRefresh();
}

void WeatherService::setNetworkOnline(bool online)
{
    if (m_networkOnline == online) {
        return;
    }

    m_networkOnline = online;
    scheduleRefresh();
}

void WeatherService::refresh()
{
    if (!m_config.enabled || !m_networkOnline) {
        return;
    }

    cancelRequests();
    const quint64 requestSerial = ++m_requestSerial;

    QString city = m_config.city.trimmed();
    if (city.isEmpty()) {
        city = QStringLiteral("历城");
    }
    const QString areaId = m_config.areaId.trimmed();

    resolveAndFetchWeather(city, areaId, requestSerial);
}

void WeatherService::resolveAndFetchWeather(const QString &city, const QString &areaId, quint64 serial)
{
    if (!areaId.isEmpty()) {
        m_resolvedAreaId = areaId;
        m_resolvedCity = city;
        fetchWeatherIndex(areaId, serial);
        return;
    }

    if (!m_resolvedAreaId.isEmpty() && m_resolvedCity == city) {
        fetchWeatherIndex(m_resolvedAreaId, serial);
        return;
    }

    // 通过中国天气网城市搜索接口解析 areaId
    QUrl searchUrl(QStringLiteral("https://toy1.weather.com.cn/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("cityname"), city);
    query.addQueryItem(QStringLiteral("_"), QString::number(QDateTime::currentMSecsSinceEpoch()));
    searchUrl.setQuery(query);

    m_searchReply = m_networkAccessManager.get(makeChinaWeatherRequest(searchUrl));
    connect(m_searchReply, &QNetworkReply::finished, this, [this, reply = m_searchReply, city, serial]() {
        if (!reply) {
            return;
        }

        const QByteArray rawBytes = reply->readAll();
        reply->deleteLater();
        if (m_searchReply == reply) {
            m_searchReply.clear();
        }

        if (serial != m_requestSerial || !m_config.enabled || !m_networkOnline) {
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            setState(tr("城市搜索失败"), city, reply->errorString());
            return;
        }

        QString text = QString::fromUtf8(rawBytes).trimmed();
        if (text.startsWith(QLatin1Char('(')) && text.endsWith(QLatin1Char(')'))) {
            text = text.mid(1, text.length() - 2).trimmed();
        }

        const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
        const QJsonArray array = doc.array();
        QString foundAreaId;
        QString foundProvince;
        for (int i = 0; i < array.size(); ++i) {
            const QJsonObject obj = array.at(i).toObject();
            const QString ref = obj.value(QStringLiteral("ref")).toString();
            const QStringList parts = ref.split(QLatin1Char('~'));
            if (!parts.isEmpty() && parts.first().length() <= 12 && parts.size() >= 4) {
                foundAreaId = parts.first().trimmed();
                if (parts.size() >= 10) {
                    foundProvince = parts.at(9).trimmed();
                }
                break;
            }
        }

        if (foundAreaId.isEmpty()) {
            setState(tr("未匹配到城市代码"), city);
            return;
        }

        m_resolvedAreaId = foundAreaId;
        m_resolvedCity = city;
        if (!foundProvince.isEmpty()) {
            m_resolvedProvince = foundProvince;
        }
        fetchWeatherIndex(foundAreaId, serial);
    });
}

void WeatherService::fetchWeatherIndex(const QString &areaId, quint64 serial)
{
    QUrl weatherUrl(QStringLiteral("https://d1.weather.com.cn/weather_index/%1.html").arg(areaId));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("_"), QString::number(QDateTime::currentMSecsSinceEpoch()));
    weatherUrl.setQuery(query);

    m_weatherReply = m_networkAccessManager.get(makeChinaWeatherRequest(weatherUrl));
    connect(m_weatherReply, &QNetworkReply::finished, this, [this, reply = m_weatherReply, areaId, serial]() {
        if (!reply) {
            return;
        }

        const QByteArray payload = reply->readAll();
        reply->deleteLater();
        if (m_weatherReply == reply) {
            m_weatherReply.clear();
        }

        if (serial != m_requestSerial || !m_config.enabled || !m_networkOnline) {
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            setState(tr("天气获取失败"), QString(), reply->errorString());
            return;
        }

        const QString fullText = QString::fromUtf8(payload);
        const QJsonObject dataSK = extractJsonObject(fullText, QStringLiteral("dataSK"));
        const QJsonObject fc = extractJsonObject(fullText, QStringLiteral("fc"));
        const QJsonObject dataZS = extractJsonObject(fullText, QStringLiteral("dataZS"));

        if (dataSK.isEmpty()) {
            setState(tr("天气数据解析为空"));
            return;
        }

        // 1. 实时天气提取
        const QString weatherCode = dataSK.value(QStringLiteral("weathercode")).toString();
        const QString weather = weatherName(weatherCode, dataSK.value(QStringLiteral("weather")).toString());
        const QString emoji = weatherEmoji(weatherCode, weather);
        const QString temp = normalizeTemp(dataSK.value(QStringLiteral("temp")).toString());
        const QString cityName = dataSK.value(QStringLiteral("cityname")).toString(!m_config.city.isEmpty() ? m_config.city : QStringLiteral("历城"));

        const QString nextSummary = QStringLiteral("%1 %2°C").arg(weather, temp);
        QString nextLocation;
        if (!m_resolvedProvince.isEmpty()) {
            nextLocation = QStringLiteral("%1 · %2").arg(m_resolvedProvince, cityName);
        } else if (m_config.areaId == QStringLiteral("101010100") || m_config.city == QStringLiteral("历城") || cityName == QStringLiteral("历城")) {
            nextLocation = QStringLiteral("山东 · %1").arg(cityName);
        } else {
            nextLocation = cityName;
        }

        // 湿度、风向、风力、空气质量
        const QString sd = dataSK.value(QStringLiteral("sd")).toString(QStringLiteral("60%"));
        const QString wd = dataSK.value(QStringLiteral("WD")).toString(QStringLiteral("微风"));
        const QString ws = dataSK.value(QStringLiteral("WS")).toString();
        const QString aqi = dataSK.value(QStringLiteral("aqi")).toString();

        // 提取舒适度或穿衣等贴心生活指数
        QString lifeHint;
        QString lifeTitle;
        const QJsonObject zsObj = dataZS.value(QStringLiteral("zs")).toObject();
        if (!zsObj.isEmpty()) {
            if (zsObj.contains(QStringLiteral("ct_des_s"))) {
                lifeTitle = QStringLiteral("穿衣");
                lifeHint = zsObj.value(QStringLiteral("ct_des_s")).toString();
            } else if (zsObj.contains(QStringLiteral("co_des_s"))) {
                lifeTitle = QStringLiteral("舒适度");
                lifeHint = zsObj.value(QStringLiteral("co_des_s")).toString();
            }
        }

        const QString reportTime = dataSK.value(QStringLiteral("time")).toString();
        int aqiVal = aqi.toInt();
        QString aqiText;
        QString aqiColor;
        if (!aqi.isEmpty() && aqiVal > 0) {
            if (aqiVal <= 50) {
                aqiText = QStringLiteral("优");
                aqiColor = QStringLiteral("#34d399");
            } else if (aqiVal <= 100) {
                aqiText = QStringLiteral("良");
                aqiColor = QStringLiteral("#38bdf8");
            } else if (aqiVal <= 150) {
                aqiText = QStringLiteral("轻度");
                aqiColor = QStringLiteral("#fbbf24");
            } else if (aqiVal <= 200) {
                aqiText = QStringLiteral("中度");
                aqiColor = QStringLiteral("#fb923c");
            } else {
                aqiText = QStringLiteral("重度");
                aqiColor = QStringLiteral("#f87171");
            }
        }

        QString nextDetail = QStringLiteral("体感 %1°C   湿度 %2   %3 %4").arg(temp, sd, wd, ws);
        if (!aqi.isEmpty()) {
            nextDetail += QStringLiteral("   AQI: %1").arg(aqi);
        }
        if (!lifeHint.isEmpty()) {
            nextDetail += QStringLiteral("\n%1: %2").arg(lifeTitle, lifeHint);
        }

        QVariantMap currentMap;
        currentMap.insert(QStringLiteral("temp"), temp);
        currentMap.insert(QStringLiteral("weather"), weather);
        currentMap.insert(QStringLiteral("emoji"), emoji);
        currentMap.insert(QStringLiteral("icon"), weatherIconSource(weatherCode, weather));
        currentMap.insert(QStringLiteral("cityName"), cityName);
        currentMap.insert(QStringLiteral("location"), nextLocation);
        currentMap.insert(QStringLiteral("humidity"), sd);
        currentMap.insert(QStringLiteral("windDirection"), wd);
        currentMap.insert(QStringLiteral("windPower"), ws);
        currentMap.insert(QStringLiteral("aqi"), aqi);
        currentMap.insert(QStringLiteral("aqiText"), aqiText);
        currentMap.insert(QStringLiteral("aqiColor"), aqiColor);
        currentMap.insert(QStringLiteral("lifeTitle"), lifeTitle);
        currentMap.insert(QStringLiteral("lifeHint"), lifeHint);
        currentMap.insert(QStringLiteral("reportTime"), reportTime);

        // 2. 预报天气提取（今天、明天、后天）
        QVariantList nextForecast;
        const QJsonArray forecastList = fc.value(QStringLiteral("f")).toArray();
        const QDate today = QDate::currentDate();

        for (int i = 0; i < forecastList.size() && i < 3; ++i) {
            const QJsonObject dayObj = forecastList.at(i).toObject();

            QString label;
            if (i == 0) {
                label = QStringLiteral("今天");
            } else if (i == 1) {
                label = QStringLiteral("明天");
            } else {
                label = QStringLiteral("后天");
            }

            const QString week = weekdayString(today.addDays(i).dayOfWeek());
            const QString fa = dayObj.value(QStringLiteral("fa")).toString();
            const QString fb = dayObj.value(QStringLiteral("fb")).toString();
            const QString dayWeather = weatherName(fa, QString());
            const QString nightWeather = weatherName(fb, QString());

            QString weatherText = dayWeather;
            if (!nightWeather.isEmpty() && dayWeather != nightWeather) {
                weatherText = QStringLiteral("%1转%2").arg(dayWeather, nightWeather);
            }

            const QString dayEmoji = weatherEmoji(fa, dayWeather);
            const QString dayIcon = weatherIconSource(fa, dayWeather);
            const QString highTemp = normalizeTemp(dayObj.value(QStringLiteral("fc")).toString());
            const QString lowTemp = normalizeTemp(dayObj.value(QStringLiteral("fd")).toString());

            const QString dayWindDir = dayObj.value(QStringLiteral("fe")).toString();
            const QString dayWindPower = dayObj.value(QStringLiteral("fg")).toString();
            const QString nightWindDir = dayObj.value(QStringLiteral("ff")).toString();
            const QString nightWindPower = dayObj.value(QStringLiteral("fh")).toString();

            QVariantMap item;
            item.insert(QStringLiteral("label"), label);
            item.insert(QStringLiteral("week"), week);
            item.insert(QStringLiteral("emoji"), dayEmoji);
            item.insert(QStringLiteral("icon"), dayIcon);
            item.insert(QStringLiteral("weather"), weatherText);
            item.insert(QStringLiteral("dayWeather"), dayWeather);
            item.insert(QStringLiteral("nightWeather"), nightWeather);
            item.insert(QStringLiteral("highTemp"), highTemp);
            item.insert(QStringLiteral("lowTemp"), lowTemp);
            item.insert(QStringLiteral("tempRange"), QStringLiteral("%1~%2°C").arg(lowTemp, highTemp));
            item.insert(QStringLiteral("wind"), QStringLiteral("白天%1%2  夜间%3%4").arg(dayWindDir, dayWindPower, nightWindDir, nightWindPower));
            item.insert(QStringLiteral("dayWind"), QStringLiteral("%1 %2").arg(dayWindDir, dayWindPower).trimmed());
            item.insert(QStringLiteral("nightWind"), QStringLiteral("%1 %2").arg(nightWindDir, nightWindPower).trimmed());
            nextForecast.append(item);
        }

        setState(nextSummary, nextLocation, nextDetail, nextForecast, currentMap);
    });
}

void WeatherService::scheduleRefresh()
{
    if (!m_networkOnline) {
        cancelRequests();
        m_refreshTimer.stop();
        setState(tr("Network disconnected"));
        return;
    }

    if (!m_config.enabled) {
        cancelRequests();
        m_refreshTimer.stop();
        setState(tr("Weather disabled"));
        return;
    }

    refresh();
    m_refreshTimer.start(qMax(1, m_config.refreshMinutes) * 60 * 1000);
}

void WeatherService::cancelRequests()
{
    ++m_requestSerial;

    if (m_searchReply) {
        m_searchReply->abort();
        m_searchReply->deleteLater();
        m_searchReply.clear();
    }
    if (m_weatherReply) {
        m_weatherReply->abort();
        m_weatherReply->deleteLater();
        m_weatherReply.clear();
    }
}

void WeatherService::setState(const QString &summary,
                              const QString &location,
                              const QString &detail,
                              const QVariantList &forecast,
                              const QVariantMap &current)
{
    if (m_summary == summary
        && m_location == location
        && m_detail == detail
        && m_forecast == forecast
        && m_current == current) {
        return;
    }

    m_summary = summary;
    m_location = location;
    m_detail = detail;
    m_forecast = forecast;
    m_current = current;
    emit stateChanged();
}
