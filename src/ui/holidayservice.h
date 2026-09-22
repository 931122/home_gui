#ifndef HOLIDAYSERVICE_H
#define HOLIDAYSERVICE_H

#include <QDate>
#include <QHash>
#include <QObject>
#include <QPointer>

class QNetworkAccessManager;
class QNetworkReply;

class HolidayService : public QObject
{
    Q_OBJECT

public:
    explicit HolidayService(QObject *parent = nullptr);

    QString currentBadge() const;
    QString currentDetail() const;
    QString badgeForDate(const QDate &date) const;
    void ensureDateLoaded(const QDate &date);

    void setNetworkOnline(bool online);
    void setCurrentDate(const QDate &date);

signals:
    void stateChanged();

private:
    struct HolidayDay {
        bool valid = false;
        bool isHoliday = false;
        bool isWorkday = false;
    };

    void ensureYearLoaded(int year);
    void requestYear(int year, int sourceIndex = 0);
    QString cacheFilePath(int year) const;
    bool loadYearFromCache(int year);
    bool storeYearToCache(int year, const QByteArray &payload) const;
    bool applyYearPayload(int year, const QByteArray &payload);
    void updateCurrentState();

    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QPointer<QNetworkReply> m_activeReply;
    QHash<int, QHash<QString, HolidayDay>> m_yearData;
    QDate m_currentDate;
    QString m_currentBadge;
    QString m_currentDetail;
    bool m_networkOnline = true;
    int m_activeRequestYear = 0;
    int m_activeRequestSourceIndex = -1;
};

#endif
