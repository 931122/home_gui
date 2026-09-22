#include "holidayservice.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

namespace {

QString yearUrl(int year, int sourceIndex)
{
    switch (sourceIndex) {
    case 0:
        return QStringLiteral("https://raw.githubusercontent.com/NateScarlet/holiday-cn/master/%1.json")
                .arg(year);
    case 1:
        return QStringLiteral("https://cdn.jsdelivr.net/gh/NateScarlet/holiday-cn@master/%1.json")
                .arg(year);
    default:
        return QString();
    }
}

} // namespace

HolidayService::HolidayService(QObject *parent)
    : QObject(parent)
    , m_networkAccessManager(new QNetworkAccessManager(this))
{
}

QString HolidayService::currentBadge() const
{
    return m_currentBadge;
}

QString HolidayService::currentDetail() const
{
    return m_currentDetail;
}

QString HolidayService::badgeForDate(const QDate &date) const
{
    if (!date.isValid()) {
        return QString();
    }

    const auto yearIt = m_yearData.constFind(date.year());
    if (yearIt != m_yearData.constEnd()) {
        const QString key = date.toString(QStringLiteral("yyyy-MM-dd"));
        const auto dayIt = yearIt->constFind(key);
        if (dayIt != yearIt->constEnd()) {
            if (dayIt->isHoliday) {
                return QStringLiteral("休");
            }
            if (dayIt->isWorkday) {
                return QStringLiteral("班");
            }
        }
        return QString();
    }

    return QString();
}

void HolidayService::ensureDateLoaded(const QDate &date)
{
    if (!date.isValid()) {
        return;
    }

    ensureYearLoaded(date.year());
}

void HolidayService::setNetworkOnline(bool online)
{
    if (m_networkOnline == online) {
        return;
    }

    m_networkOnline = online;
    if (m_networkOnline && m_currentDate.isValid()) {
        ensureYearLoaded(m_currentDate.year());
    }
}

void HolidayService::setCurrentDate(const QDate &date)
{
    if (m_currentDate == date) {
        return;
    }

    m_currentDate = date;
    if (m_currentDate.isValid()) {
        ensureYearLoaded(m_currentDate.year());
    }
    updateCurrentState();
}

void HolidayService::ensureYearLoaded(int year)
{
    if (m_yearData.contains(year)) {
        return;
    }

    if (loadYearFromCache(year)) {
        updateCurrentState();
        return;
    }

    if (m_networkOnline) {
        requestYear(year);
    }
}

void HolidayService::requestYear(int year, int sourceIndex)
{
    if (!m_networkOnline) {
        return;
    }

    const QString url = yearUrl(year, sourceIndex);
    if (url.isEmpty()) {
        return;
    }

    if (m_activeReply) {
        if (m_activeRequestYear == year && m_activeRequestSourceIndex == sourceIndex) {
            return;
        }
        // Clear m_activeReply BEFORE calling abort(). Qt may fire the finished signal
        // synchronously inside abort(), and the lambda must see a null m_activeReply so it
        // returns early instead of retrying for the superseded request.
        QNetworkReply *staleReply = m_activeReply;
        m_activeReply.clear();
        m_activeRequestYear = 0;
        m_activeRequestSourceIndex = -1;
        staleReply->abort();
        staleReply->deleteLater();
    }

    m_activeRequestYear = year;
    m_activeRequestSourceIndex = sourceIndex;

    QNetworkRequest request{QUrl(url)};
    QNetworkReply *reply = m_networkAccessManager->get(request);
    m_activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, year, sourceIndex]() {
        // Guard against both the abort path (m_activeReply cleared) and a later supersede
        // (m_activeReply points to a different reply).
        if (!m_activeReply || m_activeReply.data() != reply) {
            return;
        }

        m_activeReply.clear();
        m_activeRequestYear = 0;
        m_activeRequestSourceIndex = -1;

        const bool ok = reply->error() == QNetworkReply::NoError;
        const QByteArray payload = ok && reply->isReadable() ? reply->readAll() : QByteArray();
        reply->deleteLater();

        if (!ok) {
            // 先走 raw.githubusercontent.com，再回退 jsDelivr。
            requestYear(year, sourceIndex + 1);
            return;
        }

        if (applyYearPayload(year, payload)) {
            storeYearToCache(year, payload);
            updateCurrentState();
            return;
        }

        requestYear(year, sourceIndex + 1);
    });
}

QString HolidayService::cacheFilePath(int year) const
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return baseDir + QStringLiteral("/holiday-cache/%1.json").arg(year);
}

bool HolidayService::loadYearFromCache(int year)
{
    QFile file(cacheFilePath(year));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    return applyYearPayload(year, file.readAll());
}

bool HolidayService::storeYearToCache(int year, const QByteArray &payload) const
{
    const QString path = cacheFilePath(year);
    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(payload);
    return file.error() == QFile::NoError;
}

bool HolidayService::applyYearPayload(int year, const QByteArray &payload)
{
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (!document.isObject()) {
        return false;
    }

    const QJsonArray days = document.object().value(QStringLiteral("days")).toArray();
    QHash<QString, HolidayDay> yearMap;
    for (const QJsonValue &value : days) {
        const QJsonObject object = value.toObject();
        const QString date = object.value(QStringLiteral("date")).toString().trimmed();
        if (date.isEmpty()) {
            continue;
        }

        HolidayDay day;
        day.valid = true;
        const bool isOffDay = object.value(QStringLiteral("isOffDay")).toBool(
                    object.value(QStringLiteral("isHoliday")).toBool(false));
        day.isHoliday = isOffDay;
        day.isWorkday = !day.isHoliday;
        yearMap.insert(date, day);
    }

    m_yearData.insert(year, yearMap);
    return true;
}

void HolidayService::updateCurrentState()
{
    QString nextBadge = badgeForDate(m_currentDate);
    QString nextDetail;

    if (m_currentDate.isValid() && m_yearData.contains(m_currentDate.year())) {
        const auto yearIt = m_yearData.constFind(m_currentDate.year());
        const QString key = m_currentDate.toString(QStringLiteral("yyyy-MM-dd"));
        const auto dayIt = yearIt->constFind(key);
        if (dayIt != yearIt->constEnd()) {
            if (dayIt->isHoliday) {
                nextDetail = tr("Holiday");
            } else if (dayIt->isWorkday) {
                nextDetail = tr("Workday");
            }
        }
    }

    if (m_currentBadge == nextBadge && m_currentDetail == nextDetail) {
        return;
    }

    m_currentBadge = nextBadge;
    m_currentDetail = nextDetail;
    emit stateChanged();
}
