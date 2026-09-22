#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>
#include <QVariantList>

#include "core/appconfig.h"

class QNetworkReply;

class WeatherService : public QObject
{
    Q_OBJECT

public:
    explicit WeatherService(QObject *parent = nullptr);

    QString summary() const;
    QString location() const;
    QString detail() const;
    QVariantList forecast() const;
    QVariantMap current() const;

    void applyConfig(const WeatherConfig &config, bool networkOnline);
    void setNetworkOnline(bool online);
    void refresh();

signals:
    void stateChanged();

private:
    void scheduleRefresh();
    void cancelRequests();
    void resolveAndFetchWeather(const QString &city, const QString &areaId, quint64 serial);
    void fetchWeatherIndex(const QString &areaId, quint64 serial);
    void setState(const QString &summary,
                  const QString &location = QString(),
                  const QString &detail = QString(),
                  const QVariantList &forecast = QVariantList(),
                  const QVariantMap &current = QVariantMap());

    QNetworkAccessManager m_networkAccessManager;
    QTimer m_refreshTimer;
    WeatherConfig m_config;
    QPointer<QNetworkReply> m_searchReply;
    QPointer<QNetworkReply> m_weatherReply;
    QString m_resolvedAreaId;
    QString m_resolvedCity;
    QString m_resolvedProvince;
    QString m_summary;
    QString m_location;
    QString m_detail;
    QVariantList m_forecast;
    QVariantMap m_current;
    quint64 m_requestSerial = 0;
    bool m_networkOnline = true;
};

#endif // WEATHERSERVICE_H
