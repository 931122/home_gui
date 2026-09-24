#include "homeassistantmodule.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QWebSocket>

#include "core/globalstate.h"
#include "core/platformhelper.h"

namespace {

QString normalizedBaseUrl(const HomeAssistantConfig &config)
{
    QString baseUrl = config.baseUrl.trimmed();
    while (baseUrl.endsWith(QLatin1Char('/'))) {
        baseUrl.chop(1);
    }
    return baseUrl;
}

QUrl webSocketUrl(const HomeAssistantConfig &config)
{
    QUrl url(normalizedBaseUrl(config));
    if (!url.isValid() || url.isEmpty()) {
        return QUrl();
    }

    url.setScheme(url.scheme() == QStringLiteral("https")
                  ? QStringLiteral("wss")
                  : QStringLiteral("ws"));
    url.setPath(QStringLiteral("/api/websocket"));
    url.setQuery(QUrlQuery());
    return url;
}

QString normalizedEntityStateText(const QString &state)
{
    const QString normalized = state.trimmed().toLower();
    if (normalized == QStringLiteral("on")) {
        return HomeAssistantWorker::tr("On");
    }
    if (normalized == QStringLiteral("off")) {
        return HomeAssistantWorker::tr("Off");
    }
    if (normalized == QStringLiteral("open")) {
        return HomeAssistantWorker::tr("Open");
    }
    if (normalized == QStringLiteral("opening")) {
        return HomeAssistantWorker::tr("Opening");
    }
    if (normalized == QStringLiteral("closed")) {
        return HomeAssistantWorker::tr("Closed");
    }
    if (normalized == QStringLiteral("closing")) {
        return HomeAssistantWorker::tr("Closing");
    }
    if (normalized == QStringLiteral("unavailable")) {
        return HomeAssistantWorker::tr("Unavailable");
    }
    if (normalized == QStringLiteral("idle") || normalized == QStringLiteral("standby") || normalized == QStringLiteral("0")
        || normalized == QStringLiteral("待机") || normalized == QStringLiteral("空闲") || normalized == QStringLiteral("关机")) {
        return QStringLiteral("待机");
    }
    if (normalized == QStringLiteral("busy") || normalized == QStringLiteral("cooking")
        || normalized == QStringLiteral("work") || normalized == QStringLiteral("working")
        || normalized == QStringLiteral("running") || normalized == QStringLiteral("1")
        || normalized == QStringLiteral("heating") || normalized == QStringLiteral("boiling")
        || normalized == QStringLiteral("stewing") || normalized.contains(QStringLiteral("烹饪"))
        || normalized.contains(QStringLiteral("工作中")) || normalized.contains(QStringLiteral("运行中"))
        || normalized.contains(QStringLiteral("煮饭")) || normalized.contains(QStringLiteral("加热"))) {
        return QStringLiteral("烹饪中");
    }
    if (normalized == QStringLiteral("pressure") || normalized == QStringLiteral("pressure_keeping")
        || normalized == QStringLiteral("pressure_holding") || normalized.contains(QStringLiteral("保压"))) {
        return QStringLiteral("保压中");
    }
    if (normalized == QStringLiteral("keep warm") || normalized == QStringLiteral("keep_warm")
        || normalized == QStringLiteral("warm") || normalized == QStringLiteral("warming")
        || normalized == QStringLiteral("3") || normalized.contains(QStringLiteral("保温"))) {
        return QStringLiteral("保温中");
    }
    if (normalized == QStringLiteral("delay") || normalized == QStringLiteral("appointment")
        || normalized == QStringLiteral("reserve") || normalized == QStringLiteral("4")
        || normalized.contains(QStringLiteral("预约"))) {
        return QStringLiteral("预约中");
    }
    if (normalized == QStringLiteral("paused") || normalized == QStringLiteral("pause")
        || normalized == QStringLiteral("2") || normalized.contains(QStringLiteral("暂停"))) {
        return QStringLiteral("已暂停");
    }
    if (normalized == QStringLiteral("exhaust") || normalized == QStringLiteral("exhausting")
        || normalized == QStringLiteral("depressurize") || normalized == QStringLiteral("depressurizing")
        || normalized.contains(QStringLiteral("排气")) || normalized.contains(QStringLiteral("泄压"))) {
        return QStringLiteral("泄压中");
    }
    if (normalized == QStringLiteral("complete") || normalized == QStringLiteral("completed")
        || normalized == QStringLiteral("finish") || normalized == QStringLiteral("finished")
        || normalized == QStringLiteral("done") || normalized == QStringLiteral("5")
        || normalized.contains(QStringLiteral("完成"))) {
        return QStringLiteral("已完成");
    }
    if (normalized.isEmpty() || normalized == QStringLiteral("unknown")) {
        return HomeAssistantWorker::tr("Unknown");
    }
    return state.trimmed();
}

bool entityStateIsActive(const QString &state)
{
    const QString normalized = state.trimmed().toLower();
    if (normalized.isEmpty() || normalized == QStringLiteral("unknown")
            || normalized == QStringLiteral("unavailable")
            || normalized == QStringLiteral("off")
            || normalized == QStringLiteral("idle")
            || normalized == QStringLiteral("standby")
            || normalized == QStringLiteral("0")
            || normalized == QStringLiteral("5")
            || normalized == QStringLiteral("complete")
            || normalized == QStringLiteral("completed")
            || normalized == QStringLiteral("finished")
            || normalized == QStringLiteral("done")
            || normalized == QStringLiteral("待机")
            || normalized == QStringLiteral("空闲")
            || normalized == QStringLiteral("关机")
            || normalized == QStringLiteral("已完成")
            || normalized == QStringLiteral("完成")
            || normalized == QStringLiteral("离线")) {
        return false;
    }

    return normalized == QStringLiteral("on")
            || normalized == QStringLiteral("open")
            || normalized == QStringLiteral("opening")
            || normalized == QStringLiteral("unlocked")
            || normalized == QStringLiteral("home")
            || normalized == QStringLiteral("playing")
            || normalized == QStringLiteral("busy")
            || normalized == QStringLiteral("cooking")
            || normalized == QStringLiteral("running")
            || normalized == QStringLiteral("working")
            || normalized == QStringLiteral("keep warm")
            || normalized == QStringLiteral("keep_warm")
            || normalized == QStringLiteral("warm")
            || normalized == QStringLiteral("1")
            || normalized == QStringLiteral("2")
            || normalized == QStringLiteral("3")
            || normalized == QStringLiteral("4")
            || normalized.contains(QStringLiteral("烹饪"))
            || normalized.contains(QStringLiteral("工作"))
            || normalized.contains(QStringLiteral("运行"))
            || normalized.contains(QStringLiteral("加热"))
            || normalized.contains(QStringLiteral("保压"))
            || normalized.contains(QStringLiteral("保温"))
            || normalized.contains(QStringLiteral("预约"))
            || normalized.contains(QStringLiteral("泄压"))
            || normalized.contains(QStringLiteral("排气"))
            || normalized.contains(QStringLiteral("收汁"));
}

qreal normalizedLightBrightness(const QJsonObject &attributes)
{
    const int rawBrightness = attributes.value(QStringLiteral("brightness")).toInt(-1);
    if (rawBrightness < 0) {
        return -1.0;
    }
    return qBound(0.0, rawBrightness / 255.0, 1.0);
}

qreal normalizedVolumeLevel(const QJsonObject &attributes)
{
    const qreal volume = attributes.value(QStringLiteral("volume_level")).toDouble(-1.0);
    if (volume < 0.0) {
        return -1.0;
    }
    return qBound(0.0, volume, 1.0);
}

qreal normalizedCoverPosition(const QJsonObject &attributes)
{
    const int position = attributes.value(QStringLiteral("current_position")).toInt(-1);
    if (position < 0) {
        return -1.0;
    }
    return qBound(0.0, position / 100.0, 1.0);
}

} // namespace

bool HomeAssistantWorker::findAction(const QString &actionName,
                                     HomeAssistantActionConfig &matchedAction) const
{
    for (const HomeAssistantActionConfig &action : m_config.actions) {
        if (action.name == actionName || action.entityId == actionName) {
            matchedAction = action;
            return true;
        }
    }
    return false;
}

void HomeAssistantWorker::initialize(const HomeAssistantConfig &config)
{
    m_config = config;
    emit actionStatesChanged(buildActionStates());
    if (m_networkAccessManager == nullptr) {
        m_networkAccessManager = new QNetworkAccessManager(this);
    }
    if (m_reconnectTimer == nullptr) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
            initialize(m_config);
        });
    }

    m_reconnectTimer->stop();
    // 每次初始化都递增 generation，使之前的异步回调失效。
    m_requestGeneration++;
    abortCurrentReply();
    disconnectWebSocket();

    if (!config.enabled) {
        emit statusChanged(tr("HA disabled"));
        m_reconnectDelayMs = 5000;
        return;
    }

    const QString baseUrl = normalizedBaseUrl(config);
    if (baseUrl.isEmpty() || config.token.trimmed().isEmpty()) {
        emit statusChanged(tr("HA config incomplete"));
        m_reconnectDelayMs = 5000;
        return;
    }

    QNetworkRequest request(QUrl(baseUrl + QStringLiteral("/api/")));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(config.token.trimmed()).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_networkAccessManager->get(request);
    const int currentGen = m_requestGeneration;
    trackReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, currentGen]() {
        if (m_requestGeneration != currentGen) {
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            const QString errorText = reply->property("timedOut").toBool()
                ? tr("request timeout")
                : reply->errorString();
            emit statusChanged(tr("HA connect failed: %1").arg(errorText));
            startReconnectTimer();
            return;
        }

        connectWebSocket();
    });
}

void HomeAssistantWorker::shutdown()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_reconnectDelayMs = 5000;
    m_requestGeneration++;
    abortCurrentReply();
    disconnectWebSocket();
    m_entityStates.clear();
    emit actionStatesChanged(QVariantList());
    emit statusChanged(tr("HA stopped"));
}

void HomeAssistantWorker::startReconnectTimer()
{
    if (!m_config.enabled || (m_reconnectTimer && m_reconnectTimer->isActive())) {
        return;
    }

    // 使用指数退避策略：每次失败倍增等待时间，上限 60 秒。
    if (m_reconnectTimer) {
        m_reconnectTimer->start(m_reconnectDelayMs);
        m_reconnectDelayMs = qMin(60000, m_reconnectDelayMs * 2);
    }
}

void HomeAssistantWorker::abortCurrentReply()
{
    // 拷贝一份，避免在遍历时因为 finished 信号触发 remove() 导致迭代器失效。
    const QSet<QNetworkReply *> replies = m_activeReplies;
    m_activeReplies.clear();

    for (QNetworkReply *reply : replies) {
        if (reply) {
            reply->abort();
        }
    }
}

void HomeAssistantWorker::trackReply(QNetworkReply *reply, int timeoutMs)
{
    if (!reply) {
        return;
    }

    m_activeReplies.insert(reply);

    // 设置请求超时。
    QTimer *timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [reply]() {
        if (reply->isRunning()) {
            reply->setProperty("timedOut", true);
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_activeReplies.remove(reply);
        reply->deleteLater();
    });
    timer->start(timeoutMs);
}

void HomeAssistantWorker::sendServiceRequest(const HomeAssistantActionConfig &action,
                                             const QString &service,
                                             const QJsonObject &payload,
                                             const QString &successStatus)
{
    const QString baseUrl = normalizedBaseUrl(m_config);
    if (baseUrl.isEmpty() || m_config.token.trimmed().isEmpty()) {
        emit statusChanged(tr("HA config incomplete"));
        return;
    }

    const QUrl url(baseUrl + QStringLiteral("/api/services/%1/%2")
                   .arg(action.domain, service));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_config.token.trimmed()).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_networkAccessManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const int currentGen = m_requestGeneration;
    trackReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, successStatus, currentGen]() {
        if (m_requestGeneration != currentGen) {
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            const QString errorText = reply->property("timedOut").toBool()
                ? tr("request timeout")
                : reply->errorString();
            emit statusChanged(tr("HA action failed: %1").arg(errorText));
        } else {
            emit statusChanged(successStatus);
        }
    });
}

void HomeAssistantWorker::triggerAction(const QString &actionName)
{
    if (!m_config.enabled) {
        emit statusChanged(tr("HA disabled"));
        return;
    }
    if (m_networkAccessManager == nullptr) {
        emit statusChanged(tr("HA not initialized"));
        return;
    }

    HomeAssistantActionConfig matchedAction;
    if (!findAction(actionName, matchedAction)) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    QJsonObject payload = matchedAction.data;
    if (!matchedAction.entityId.trimmed().isEmpty()) {
        payload.insert(QStringLiteral("entity_id"), matchedAction.entityId.trimmed());
    }

    QString service = matchedAction.service;
    if (matchedAction.domain.compare(QStringLiteral("light"), Qt::CaseInsensitive) == 0) {
        service = QStringLiteral("toggle");
    } else if (matchedAction.domain.compare(QStringLiteral("switch"), Qt::CaseInsensitive) == 0
               || matchedAction.domain.compare(QStringLiteral("input_boolean"), Qt::CaseInsensitive) == 0) {
        service = QStringLiteral("toggle");
    } else if (matchedAction.domain.compare(QStringLiteral("cover"), Qt::CaseInsensitive) == 0) {
        const QString entityState = m_entityStates.value(matchedAction.entityId.trimmed())
                                        .value(QStringLiteral("state")).toString().trimmed().toLower();
        service = entityState == QStringLiteral("open") || entityState == QStringLiteral("opening")
                ? QStringLiteral("close_cover")
                : QStringLiteral("open_cover");
    } else if (matchedAction.domain.compare(QStringLiteral("lock"), Qt::CaseInsensitive) == 0) {
        const QString entityState = m_entityStates.value(matchedAction.entityId.trimmed())
                                        .value(QStringLiteral("state")).toString().trimmed().toLower();
        service = entityState == QStringLiteral("locked")
                ? QStringLiteral("unlock")
                : QStringLiteral("lock");
    } else if (matchedAction.domain.compare(QStringLiteral("media_player"), Qt::CaseInsensitive) == 0) {
        service = QStringLiteral("media_play_pause");
    }

    sendServiceRequest(matchedAction,
                       service,
                       payload,
                       tr("HA action sent: %1").arg(actionName));
}

void HomeAssistantWorker::callActionService(const QString &actionName, const QString &service)
{
    if (!m_config.enabled) {
        emit statusChanged(tr("HA disabled"));
        return;
    }
    if (m_networkAccessManager == nullptr) {
        emit statusChanged(tr("HA not initialized"));
        return;
    }

    HomeAssistantActionConfig matchedAction;
    if (!findAction(actionName, matchedAction)) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    const QString normalizedService = service.trimmed();
    if (normalizedService.isEmpty()) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    QJsonObject payload = matchedAction.data;
    if (!matchedAction.entityId.trimmed().isEmpty()) {
        payload.insert(QStringLiteral("entity_id"), matchedAction.entityId.trimmed());
    }

    sendServiceRequest(matchedAction,
                       normalizedService,
                       payload,
                       tr("HA action sent: %1").arg(actionName));
}

void HomeAssistantWorker::callCustomService(const QString &domain, const QString &service, const QVariantMap &data)
{
    if (!m_config.enabled) {
        emit statusChanged(tr("HA disabled"));
        return;
    }
    if (m_networkAccessManager == nullptr) {
        emit statusChanged(tr("HA not initialized"));
        return;
    }

    const QString baseUrl = normalizedBaseUrl(m_config);
    if (baseUrl.isEmpty() || m_config.token.trimmed().isEmpty()) {
        emit statusChanged(tr("HA config incomplete"));
        return;
    }

    const QUrl url(baseUrl + QStringLiteral("/api/services/%1/%2").arg(domain.trimmed(), service.trimmed()));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_config.token.trimmed()).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    const QJsonObject payload = QJsonObject::fromVariantMap(data);
    QNetworkReply *reply = m_networkAccessManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const int currentGen = m_requestGeneration;
    trackReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, domain, service, currentGen]() {
        if (m_requestGeneration != currentGen) {
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            const QString errorText = reply->property("timedOut").toBool()
                ? tr("request timeout")
                : reply->errorString();
            emit statusChanged(tr("HA service failed: %1").arg(errorText));
        } else {
            emit statusChanged(tr("HA service %1.%2 sent").arg(domain, service));
            // 操作后延时刷新状态
            QTimer::singleShot(600, this, &HomeAssistantWorker::refreshActionStates);
        }
    });
}


void HomeAssistantWorker::setLightBrightness(const QString &actionName, qreal brightness)
{
    if (!m_config.enabled) {
        emit statusChanged(tr("HA disabled"));
        return;
    }
    if (m_networkAccessManager == nullptr) {
        emit statusChanged(tr("HA not initialized"));
        return;
    }

    HomeAssistantActionConfig matchedAction;
    if (!findAction(actionName, matchedAction)) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    if (matchedAction.domain.compare(QStringLiteral("light"), Qt::CaseInsensitive) != 0) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    QJsonObject payload = matchedAction.data;
    if (!matchedAction.entityId.trimmed().isEmpty()) {
        payload.insert(QStringLiteral("entity_id"), matchedAction.entityId.trimmed());
    }
    payload.insert(QStringLiteral("brightness_pct"),
                   qBound(1, qRound(qBound(0.01, brightness, 1.0) * 100.0), 100));

    sendServiceRequest(matchedAction,
                       QStringLiteral("turn_on"),
                       payload,
                       tr("HA brightness updated: %1").arg(actionName));
}

void HomeAssistantWorker::setCoverPosition(const QString &actionName, qreal position)
{
    if (!m_config.enabled) {
        emit statusChanged(tr("HA disabled"));
        return;
    }
    if (m_networkAccessManager == nullptr) {
        emit statusChanged(tr("HA not initialized"));
        return;
    }

    HomeAssistantActionConfig matchedAction;
    if (!findAction(actionName, matchedAction)) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    if (matchedAction.domain.compare(QStringLiteral("cover"), Qt::CaseInsensitive) != 0) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    QJsonObject payload = matchedAction.data;
    if (!matchedAction.entityId.trimmed().isEmpty()) {
        payload.insert(QStringLiteral("entity_id"), matchedAction.entityId.trimmed());
    }
    payload.insert(QStringLiteral("position"),
                   qBound(0, qRound(qBound(0.0, position, 1.0) * 100.0), 100));

    sendServiceRequest(matchedAction,
                       QStringLiteral("set_cover_position"),
                       payload,
                       tr("HA cover updated: %1").arg(actionName));
}

void HomeAssistantWorker::setMediaPlayerVolume(const QString &actionName, qreal volume)
{
    if (!m_config.enabled) {
        emit statusChanged(tr("HA disabled"));
        return;
    }
    if (m_networkAccessManager == nullptr) {
        emit statusChanged(tr("HA not initialized"));
        return;
    }

    HomeAssistantActionConfig matchedAction;
    if (!findAction(actionName, matchedAction)) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    if (matchedAction.domain.compare(QStringLiteral("media_player"), Qt::CaseInsensitive) != 0) {
        emit statusChanged(tr("HA action not found: %1").arg(actionName));
        return;
    }

    QJsonObject payload = matchedAction.data;
    if (!matchedAction.entityId.trimmed().isEmpty()) {
        payload.insert(QStringLiteral("entity_id"), matchedAction.entityId.trimmed());
    }
    payload.insert(QStringLiteral("volume_level"), qBound(0.0, volume, 1.0));

    sendServiceRequest(matchedAction,
                       QStringLiteral("volume_set"),
                       payload,
                       tr("HA volume updated: %1").arg(actionName));
}

void HomeAssistantWorker::refreshActionStates()
{
    if (m_webSocket != nullptr && m_webSocket->state() == QAbstractSocket::ConnectedState) {
        const QJsonObject command = {
            {QStringLiteral("id"), m_nextCommandId++},
            {QStringLiteral("type"), QStringLiteral("get_states")}
        };
        m_getStatesCommandId = static_cast<qint64>(command.value(QStringLiteral("id")).toDouble());
        m_webSocket->sendTextMessage(QString::fromUtf8(QJsonDocument(command).toJson(QJsonDocument::Compact)));
        return;
    }

    startFetchingActionStates();
}

void HomeAssistantWorker::startFetchingActionStates()
{
    if (!m_config.enabled || m_networkAccessManager == nullptr) {
        return;
    }

    fetchNextActionState(0, QVariantList(), m_requestGeneration);
}

void HomeAssistantWorker::fetchNextActionState(int index, QVariantList result, int generation, bool hasError)
{
    // 如果代数不匹配，说明配置已变或模块已停止，直接终止递归。
    if (m_requestGeneration != generation) {
        return;
    }

    if (index >= m_config.actions.size()) {
        emit actionStatesChanged(result);
        if (hasError) {
            emit statusChanged(tr("HA state refresh failed"));
        }
        return;
    }

    const HomeAssistantActionConfig &action = m_config.actions.at(index);
    QVariantMap item;
    item.insert(QStringLiteral("name"), action.name);
    item.insert(QStringLiteral("entityId"), action.entityId);
    item.insert(QStringLiteral("domain"), action.domain);
    item.insert(QStringLiteral("available"), false);
    item.insert(QStringLiteral("state"), QString());
    item.insert(QStringLiteral("stateText"), tr("Unknown"));
    item.insert(QStringLiteral("active"), false);
    if (action.slideToTurnOff) {
        item.insert(QStringLiteral("slideToTurnOff"), true);
        item.insert(QStringLiteral("slideToClose"), true);
    }

    if (action.entityId.trimmed().isEmpty()) {
        result.append(item);
        fetchNextActionState(index + 1, result, generation, hasError);
        return;
    }

    const QString baseUrl = normalizedBaseUrl(m_config);
    QNetworkRequest request(QUrl(baseUrl + QStringLiteral("/api/states/%1").arg(action.entityId.trimmed())));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_config.token.trimmed()).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_networkAccessManager->get(request);
    trackReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, index, item, result, generation, hasError]() mutable {
        if (m_requestGeneration != generation) {
            return;
        }

        bool currentError = hasError;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
            const QJsonObject object = document.object();
            const QString state = object.value(QStringLiteral("state")).toString().trimmed().toLower();
            const QJsonObject attributes = object.value(QStringLiteral("attributes")).toObject();
            const QString friendlyName = attributes.value(QStringLiteral("friendly_name")).toString().trimmed();
            const qreal brightness = normalizedLightBrightness(attributes);
            const qreal coverPosition = normalizedCoverPosition(attributes);
            const qreal volumeLevel = normalizedVolumeLevel(attributes);

            item.insert(QStringLiteral("available"), state != QStringLiteral("unavailable"));
            item.insert(QStringLiteral("state"), state);
            item.insert(QStringLiteral("stateText"), normalizedEntityStateText(state));
            item.insert(QStringLiteral("active"), entityStateIsActive(state));
            if (brightness >= 0.0) {
                item.insert(QStringLiteral("brightness"), brightness);
            }
            if (coverPosition >= 0.0) {
                item.insert(QStringLiteral("position"), coverPosition);
            }
            if (volumeLevel >= 0.0) {
                item.insert(QStringLiteral("volume"), volumeLevel);
            }
            if (!friendlyName.isEmpty()) {
                item.insert(QStringLiteral("friendlyName"), friendlyName);
            }
        } else {
            currentError = true;
        }

        result.append(item);
        fetchNextActionState(index + 1, result, generation, currentError);
    });
}

void HomeAssistantWorker::connectWebSocket()
{
    disconnectWebSocket();

    const QUrl url = webSocketUrl(m_config);
    if (!url.isValid() || url.isEmpty()) {
        emit statusChanged(tr("HA state refresh failed: %1").arg(tr("HA config incomplete")));
        return;
    }

    m_getStatesCommandId = 0;
    m_subscribeStatesCommandId = 0;
    m_nextCommandId = 1;
    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &HomeAssistantWorker::handleWebSocketMessage);
    connect(m_webSocket, &QWebSocket::disconnected, this, [this]() {
        emit statusChanged(tr("HA websocket disconnected"));
        emit actionStatesChanged(buildActionStates());
        startReconnectTimer();
    });
    connect(m_webSocket,
            &QWebSocket::errorOccurred,
            this,
            [this](QAbstractSocket::SocketError) {
        if (m_webSocket != nullptr) {
            emit statusChanged(tr("HA websocket failed: %1").arg(m_webSocket->errorString()));
            startReconnectTimer();
        }
    });
    m_webSocket->open(url);
}

void HomeAssistantWorker::disconnectWebSocket()
{
    if (m_webSocket == nullptr) {
        return;
    }
    m_webSocket->blockSignals(true);
    m_webSocket->close();
    m_webSocket->deleteLater();
    m_webSocket = nullptr;
    m_getStatesCommandId = 0;
    m_subscribeStatesCommandId = 0;
}

void HomeAssistantWorker::handleWebSocketMessage(const QString &message)
{
    const QJsonObject object = QJsonDocument::fromJson(message.toUtf8()).object();
    const QString type = object.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("auth_required")) {
        const QJsonObject auth = {
            {QStringLiteral("type"), QStringLiteral("auth")},
            {QStringLiteral("access_token"), m_config.token.trimmed()}
        };
        m_webSocket->sendTextMessage(QString::fromUtf8(QJsonDocument(auth).toJson(QJsonDocument::Compact)));
        return;
    }

    if (type == QStringLiteral("auth_ok")) {
        // WebSocket 鉴权成功后才认为 HA 连接真正可用，避免 REST 可达但 WS 失败时退避被反复重置。
        m_reconnectDelayMs = 5000;
        emit statusChanged(tr("HA connected: %1 actions").arg(m_config.actions.size()));

        QJsonObject getStatesCommand = {
            {QStringLiteral("id"), m_nextCommandId++},
            {QStringLiteral("type"), QStringLiteral("get_states")}
        };
        m_getStatesCommandId = static_cast<qint64>(getStatesCommand.value(QStringLiteral("id")).toDouble());
        m_webSocket->sendTextMessage(QString::fromUtf8(QJsonDocument(getStatesCommand).toJson(QJsonDocument::Compact)));

        QJsonObject subscribeCommand = {
            {QStringLiteral("id"), m_nextCommandId++},
            {QStringLiteral("type"), QStringLiteral("subscribe_events")},
            {QStringLiteral("event_type"), QStringLiteral("state_changed")}
        };
        m_subscribeStatesCommandId = static_cast<qint64>(subscribeCommand.value(QStringLiteral("id")).toDouble());
        m_webSocket->sendTextMessage(QString::fromUtf8(QJsonDocument(subscribeCommand).toJson(QJsonDocument::Compact)));
        return;
    }

    if (type == QStringLiteral("auth_invalid")) {
        emit statusChanged(tr("HA websocket failed: %1").arg(object.value(QStringLiteral("message")).toString()));
        return;
    }

    if (type == QStringLiteral("result")) {
        const qint64 id = static_cast<qint64>(object.value(QStringLiteral("id")).toDouble());
        const bool success = object.value(QStringLiteral("success")).toBool(false);
        if (!success) {
            emit statusChanged(tr("HA state refresh failed: %1").arg(object.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString()));
            return;
        }

        if (id == m_getStatesCommandId) {
            m_entityStates.clear();
            const QJsonArray states = object.value(QStringLiteral("result")).toArray();
            for (const QJsonValue &stateValue : states) {
                const QJsonObject stateObject = stateValue.toObject();
                const QString entityId = stateObject.value(QStringLiteral("entity_id")).toString().trimmed();
                if (!entityId.isEmpty()) {
                    QVariantMap item;
                    const QString rawState = stateObject.value(QStringLiteral("state")).toString().trimmed();
                    item.insert(QStringLiteral("state"), rawState.toLower());
                    item.insert(QStringLiteral("rawState"), rawState);
                    const QJsonObject attributes = stateObject.value(QStringLiteral("attributes")).toObject();
                    item.insert(QStringLiteral("attributes"), attributes.toVariantMap());
                    item.insert(QStringLiteral("friendlyName"),
                                attributes.value(QStringLiteral("friendly_name")).toString().trimmed());
                    const qreal brightness = normalizedLightBrightness(attributes);
                    const qreal coverPosition = normalizedCoverPosition(attributes);
                    const qreal volumeLevel = normalizedVolumeLevel(attributes);
                    if (brightness >= 0.0) {
                        item.insert(QStringLiteral("brightness"), brightness);
                    }
                    if (coverPosition >= 0.0) {
                        item.insert(QStringLiteral("position"), coverPosition);
                    }
                    if (volumeLevel >= 0.0) {
                        item.insert(QStringLiteral("volume"), volumeLevel);
                    }
                    m_entityStates.insert(entityId, item);
                }
            }
            emit actionStatesChanged(buildActionStates());
        }
        return;
    }

    if (type == QStringLiteral("event")
        && static_cast<qint64>(object.value(QStringLiteral("id")).toDouble()) == m_subscribeStatesCommandId) {
        const QJsonObject event = object.value(QStringLiteral("event")).toObject();
        const QJsonObject data = event.value(QStringLiteral("data")).toObject();
        const QString entityId = data.value(QStringLiteral("entity_id")).toString().trimmed();
        if (entityId.isEmpty()) {
            return;
        }

        const QJsonObject newState = data.value(QStringLiteral("new_state")).toObject();
        QVariantMap item;
        const QString rawState = newState.value(QStringLiteral("state")).toString().trimmed();
        item.insert(QStringLiteral("state"), rawState.toLower());
        item.insert(QStringLiteral("rawState"), rawState);
        const QJsonObject attributes = newState.value(QStringLiteral("attributes")).toObject();
        item.insert(QStringLiteral("attributes"), attributes.toVariantMap());
        item.insert(QStringLiteral("friendlyName"),
                    attributes.value(QStringLiteral("friendly_name")).toString().trimmed());
        const qreal brightness = normalizedLightBrightness(attributes);
        const qreal coverPosition = normalizedCoverPosition(attributes);
        const qreal volumeLevel = normalizedVolumeLevel(attributes);
        if (brightness >= 0.0) {
            item.insert(QStringLiteral("brightness"), brightness);
        }
        if (coverPosition >= 0.0) {
            item.insert(QStringLiteral("position"), coverPosition);
        }
        if (volumeLevel >= 0.0) {
            item.insert(QStringLiteral("volume"), volumeLevel);
        }
        m_entityStates.insert(entityId, item);
        emit actionStatesChanged(buildActionStates());
    }
}

QVariantList HomeAssistantWorker::buildActionStates() const
{
    QVariantList result;
    for (const HomeAssistantActionConfig &action : m_config.actions) {
        QVariantMap item;
        item.insert(QStringLiteral("name"), action.name);
        item.insert(QStringLiteral("entityId"), action.entityId);
        item.insert(QStringLiteral("domain"), action.domain);
        item.insert(QStringLiteral("available"), false);
        item.insert(QStringLiteral("state"), QString());
        item.insert(QStringLiteral("stateText"), tr("Unknown"));
        item.insert(QStringLiteral("active"), false);
        if (action.slideToTurnOff) {
            item.insert(QStringLiteral("slideToTurnOff"), true);
            item.insert(QStringLiteral("slideToClose"), true);
        }

        const QVariantMap entityState = m_entityStates.value(action.entityId.trimmed());
        const QString state = entityState.value(QStringLiteral("state")).toString().trimmed().toLower();
        const QString friendlyName = entityState.value(QStringLiteral("friendlyName")).toString().trimmed();
        const qreal brightness = entityState.value(QStringLiteral("brightness"), -1.0).toReal();
        const qreal coverPosition = entityState.value(QStringLiteral("position"), -1.0).toReal();
        const qreal volumeLevel = entityState.value(QStringLiteral("volume"), -1.0).toReal();
        if (!state.isEmpty()) {
            item.insert(QStringLiteral("available"), state != QStringLiteral("unavailable"));
            item.insert(QStringLiteral("state"), state);
            item.insert(QStringLiteral("stateText"), normalizedEntityStateText(state));
            item.insert(QStringLiteral("active"), entityStateIsActive(state));
        }
        if (brightness >= 0.0) {
            item.insert(QStringLiteral("brightness"), brightness);
        }
        if (coverPosition >= 0.0) {
            item.insert(QStringLiteral("position"), coverPosition);
        }
        if (volumeLevel >= 0.0) {
            item.insert(QStringLiteral("volume"), volumeLevel);
        }
        if (!friendlyName.isEmpty()) {
            item.insert(QStringLiteral("friendlyName"), friendlyName);
        }

        const bool isCooker = action.domain.compare(QStringLiteral("cooker"), Qt::CaseInsensitive) == 0
                           || action.domain.compare(QStringLiteral("chunmi_pre_cooker"), Qt::CaseInsensitive) == 0
                           || action.entityId.contains(QStringLiteral("eh1"))
                           || action.entityId.contains(QStringLiteral("ya_li_guo"))
                           || action.name.contains(QStringLiteral("饭煲"))
                           || action.name.contains(QStringLiteral("压力锅"));
        if (isCooker) {
            item.insert(QStringLiteral("isCooker"), true);

            QString statusExact;
            QString statusFallback;
            QString runningModeVal;
            QString selectedModeVal;
            QString leftTimeExact;
            QString leftTimeFallback;
            QString tempExact;
            QString tempFallback;

            for (auto it = m_entityStates.constBegin(); it != m_entityStates.constEnd(); ++it) {
                const QString &eid = it.key();
                const QVariantMap &stateMap = it.value();
                const QString stateVal = stateMap.value(QStringLiteral("rawState"), stateMap.value(QStringLiteral("state"))).toString().trimmed();
                const QVariantMap attrs = stateMap.value(QStringLiteral("attributes")).toMap();

                // 1. 工作状态实体 (优先专用中文实体 gong_zuo_zhuang_tai，其次回退至通用 status 实体)
                if (eid.contains(QStringLiteral("gong_zuo_zhuang_tai"))) {
                    if (attrs.contains(QStringLiteral("status_zh"))) {
                        statusExact = attrs.value(QStringLiteral("status_zh")).toString().trimmed();
                    } else if (!stateVal.isEmpty() && stateVal != QStringLiteral("unknown") && stateVal != QStringLiteral("unavailable")) {
                        statusExact = stateVal;
                    }
                    if (attrs.contains(QStringLiteral("is_cooking"))) {
                        item.insert(QStringLiteral("cookerIsCooking"), attrs.value(QStringLiteral("is_cooking")));
                    }
                    if (attrs.contains(QStringLiteral("is_keep_warm"))) {
                        item.insert(QStringLiteral("cookerIsKeepWarm"), attrs.value(QStringLiteral("is_keep_warm")));
                    }
                    if (attrs.contains(QStringLiteral("is_order"))) {
                        item.insert(QStringLiteral("cookerIsOrder"), attrs.value(QStringLiteral("is_order")));
                    }
                    if (attrs.contains(QStringLiteral("phase"))) {
                        item.insert(QStringLiteral("cookerPhase"), attrs.value(QStringLiteral("phase")));
                    }
                    if (attrs.contains(QStringLiteral("phase_zh"))) {
                        item.insert(QStringLiteral("cookerPhaseZh"), attrs.value(QStringLiteral("phase_zh")));
                    }
                    if (attrs.contains(QStringLiteral("status_code"))) {
                        item.insert(QStringLiteral("cookerStatusCode"), attrs.value(QStringLiteral("status_code")));
                    }
                    if (attrs.contains(QStringLiteral("phase_code"))) {
                        item.insert(QStringLiteral("cookerPhaseCode"), attrs.value(QStringLiteral("phase_code")));
                    }
                } else if (eid.contains(QStringLiteral("peng_ren_jie_duan"))) {
                    QString phaseText = attrs.contains(QStringLiteral("phase_zh"))
                                      ? attrs.value(QStringLiteral("phase_zh")).toString().trimmed()
                                      : QString();
                    if (phaseText.isEmpty() && !stateVal.isEmpty() && stateVal != QStringLiteral("unknown")) {
                        phaseText = stateVal;
                    }
                    if (!phaseText.isEmpty()) {
                        item.insert(QStringLiteral("cookerPhaseZh"), phaseText);
                    }
                    item.insert(QStringLiteral("cookerPhaseSlug"), stateVal);
                } else if ((eid.contains(QStringLiteral("ya_li_guo")) || eid.contains(QStringLiteral("cooker")) || eid.contains(QStringLiteral("chun_mi")) || eid.contains(QStringLiteral("eh1")))
                           && (eid.contains(QStringLiteral("status")) || eid.contains(QStringLiteral("work_state")) || eid.endsWith(QStringLiteral("_state")))) {
                    if (!stateVal.isEmpty() && stateVal != QStringLiteral("unknown") && stateVal != QStringLiteral("unavailable")) {
                        statusFallback = stateVal;
                    }
                }

                // 2. 口感偏好
                if (eid.contains(QStringLiteral("kou_gan_pian_hao")) || eid.contains(QStringLiteral("taste"))) {
                    QString tasteVal = attrs.contains(QStringLiteral("taste_name"))
                                       ? attrs.value(QStringLiteral("taste_name")).toString().trimmed()
                                       : QString();
                    if (tasteVal.isEmpty() && !stateVal.isEmpty() && stateVal != QStringLiteral("unknown")) {
                        tasteVal = PlatformHelper::cookerSlugToTaste(stateVal);
                    }
                    if (!tasteVal.isEmpty()) {
                        item.insert(QStringLiteral("cookerTaste"), tasteVal);
                    }
                    item.insert(QStringLiteral("cookerTasteEntity"), eid);
                    item.insert(QStringLiteral("cookerTasteSlug"), attrs.value(QStringLiteral("taste_slug"), stateVal).toString());
                    QVariantList zhTasteOptions;
                    zhTasteOptions << QStringLiteral("软糯") << QStringLiteral("适中") << QStringLiteral("弹润");
                    item.insert(QStringLiteral("cookerTasteOptions"), zhTasteOptions);
                }
                // 3. 保压时间
                else if (eid.contains(QStringLiteral("bao_ya_shi_jian")) || eid.contains(QStringLiteral("pressure_time"))) {
                    bool ok = false;
                    const double pt = stateVal.toDouble(&ok);
                    if (ok) {
                        item.insert(QStringLiteral("cookerPressureTime"), pt);
                    }
                    item.insert(QStringLiteral("cookerPressureTimeEntity"), eid);
                    if (attrs.contains(QStringLiteral("min"))) {
                        item.insert(QStringLiteral("cookerPressureMin"), attrs.value(QStringLiteral("min")).toDouble());
                    }
                    if (attrs.contains(QStringLiteral("max"))) {
                        item.insert(QStringLiteral("cookerPressureMax"), attrs.value(QStringLiteral("max")).toDouble());
                    }
                    if (attrs.contains(QStringLiteral("step"))) {
                        item.insert(QStringLiteral("cookerPressureStep"), attrs.value(QStringLiteral("step")).toDouble());
                    }
                    if (attrs.contains(QStringLiteral("estimated_total_time"))) {
                        item.insert(QStringLiteral("cookerEstimatedTotalTime"), attrs.value(QStringLiteral("estimated_total_time")));
                    }
                    if (attrs.contains(QStringLiteral("estimated_total_minutes"))) {
                        item.insert(QStringLiteral("cookerEstimatedTotalMinutes"), attrs.value(QStringLiteral("estimated_total_minutes")));
                    }
                    if (attrs.contains(QStringLiteral("base_overhead_minutes"))) {
                        item.insert(QStringLiteral("cookerBaseOverheadMinutes"), attrs.value(QStringLiteral("base_overhead_minutes")));
                    }
                    if (attrs.contains(QStringLiteral("holding_duration_range"))) {
                        item.insert(QStringLiteral("cookerHoldingDurationRange"), attrs.value(QStringLiteral("holding_duration_range")));
                    }
                }
                // 4. 当前压力 (kPa)
                else if (eid.contains(QStringLiteral("dang_qian_ya_li")) || eid.contains(QStringLiteral("current_pressure"))) {
                    if (!stateVal.isEmpty() && stateVal != QStringLiteral("unknown")) {
                        item.insert(QStringLiteral("cookerPressure"), stateVal);
                    }
                }
                // 5. 烹饪模式（同时兼容模式选择 select 实体和运行模式 sensor 实体）
                else if (eid.contains(QStringLiteral("peng_ren_mo_shi")) || eid.contains(QStringLiteral("gong_zuo_mo_shi"))
                         || ((eid.contains(QStringLiteral("ya_li_guo")) || eid.contains(QStringLiteral("cooker")))
                             && (eid.contains(QStringLiteral("mode")) || eid.contains(QStringLiteral("mo_shi"))))) {
                    if (eid.startsWith(QStringLiteral("select."))) {
                        QString modeName = attrs.value(QStringLiteral("mode_name")).toString().trimmed();
                        if (modeName.isEmpty() && !stateVal.isEmpty() && stateVal != QStringLiteral("unknown")) {
                            modeName = PlatformHelper::cookerSlugToMode(stateVal);
                        }
                        if (!modeName.isEmpty() && modeName != QStringLiteral("unknown")) {
                            selectedModeVal = modeName;
                        }
                        item.insert(QStringLiteral("cookerModeEntity"), eid);
                        item.insert(QStringLiteral("cookerModeSlug"), stateVal);
                        if (attrs.contains(QStringLiteral("options"))) {
                            item.insert(QStringLiteral("cookerModeOptions"), attrs.value(QStringLiteral("options")));
                        }
                        if (attrs.contains(QStringLiteral("all_modes_estimated_time"))) {
                            item.insert(QStringLiteral("cookerAllModesEstimatedTime"), attrs.value(QStringLiteral("all_modes_estimated_time")));
                        }
                    } else if (eid.startsWith(QStringLiteral("sensor."))) {
                        QString sensorMode = attrs.value(QStringLiteral("selected_preset_mode")).toString().trimmed();
                        if (sensorMode.isEmpty()) {
                            sensorMode = stateVal;
                        }
                        if (!sensorMode.isEmpty() && sensorMode != QStringLiteral("unknown")) {
                            runningModeVal = sensorMode;
                        }
                    }
                    if (attrs.contains(QStringLiteral("estimated_cooking_time"))) {
                        item.insert(QStringLiteral("cookerEstimatedCookingTime"), attrs.value(QStringLiteral("estimated_cooking_time")));
                    }
                    if (attrs.contains(QStringLiteral("estimated_cooking_minutes"))) {
                        item.insert(QStringLiteral("cookerEstimatedCookingMinutes"), attrs.value(QStringLiteral("estimated_cooking_minutes")));
                    }
                    if (attrs.contains(QStringLiteral("holding_duration"))) {
                        item.insert(QStringLiteral("cookerHoldingDurationText"), attrs.value(QStringLiteral("holding_duration")));
                    }
                    if (attrs.contains(QStringLiteral("holding_duration_range"))) {
                        item.insert(QStringLiteral("cookerHoldingDurationRange"), attrs.value(QStringLiteral("holding_duration_range")));
                    }
                    if (attrs.contains(QStringLiteral("recipe_description"))) {
                        item.insert(QStringLiteral("cookerRecipeDescription"), attrs.value(QStringLiteral("recipe_description")));
                    }
                    if (attrs.contains(QStringLiteral("recipe_ingredients"))) {
                        item.insert(QStringLiteral("cookerRecipeIngredients"), attrs.value(QStringLiteral("recipe_ingredients")));
                    }
                    if (attrs.contains(QStringLiteral("recipe_steps"))) {
                        item.insert(QStringLiteral("cookerRecipeSteps"), attrs.value(QStringLiteral("recipe_steps")));
                    }
                    if (attrs.contains(QStringLiteral("recipe_practice_text"))) {
                        item.insert(QStringLiteral("cookerRecipePracticeText"), attrs.value(QStringLiteral("recipe_practice_text")));
                    }
                    if (attrs.contains(QStringLiteral("recipe_tips"))) {
                        item.insert(QStringLiteral("cookerRecipeTips"), attrs.value(QStringLiteral("recipe_tips")));
                    }
                    if (attrs.contains(QStringLiteral("practice"))) {
                        item.insert(QStringLiteral("cookerRecipePractice"), attrs.value(QStringLiteral("practice")));
                    }
                }
                // 6. 剩余时间采集 (第一优先级：专用中文实体 sheng_yu_shi_jian；第二优先级：备用 left_time)
                else if (eid.contains(QStringLiteral("sheng_yu_shi_jian"))) {
                    if (!stateVal.isEmpty() && stateVal != QStringLiteral("unknown") && stateVal != QStringLiteral("unavailable") && stateVal != QStringLiteral("0") && stateVal != QStringLiteral("900")) {
                        leftTimeExact = stateVal;
                    }
                    if (attrs.contains(QStringLiteral("remaining_time_formatted"))) {
                        item.insert(QStringLiteral("cookerRemainingTimeFormatted"), attrs.value(QStringLiteral("remaining_time_formatted")));
                    }
                    if (attrs.contains(QStringLiteral("remaining_time_hms"))) {
                        item.insert(QStringLiteral("cookerRemainingTimeHms"), attrs.value(QStringLiteral("remaining_time_hms")));
                    }
                    if (attrs.contains(QStringLiteral("remaining_hours"))) {
                        item.insert(QStringLiteral("cookerRemainingHours"), attrs.value(QStringLiteral("remaining_hours")));
                    }
                    if (attrs.contains(QStringLiteral("remaining_minutes"))) {
                        item.insert(QStringLiteral("cookerRemainingMinutes"), attrs.value(QStringLiteral("remaining_minutes")));
                    }
                    if (attrs.contains(QStringLiteral("remaining_seconds"))) {
                        item.insert(QStringLiteral("cookerRemainingSeconds"), attrs.value(QStringLiteral("remaining_seconds")));
                    }
                    if (attrs.contains(QStringLiteral("total_remaining_seconds"))) {
                        item.insert(QStringLiteral("cookerTotalRemainingSeconds"), attrs.value(QStringLiteral("total_remaining_seconds")));
                    }
                    if (attrs.contains(QStringLiteral("unit_of_measurement"))) {
                        item.insert(QStringLiteral("cookerLeftTimeUnit"), attrs.value(QStringLiteral("unit_of_measurement")));
                    }
                    if (attrs.contains(QStringLiteral("preset_estimated_total_time"))) {
                        item.insert(QStringLiteral("cookerPresetEstimatedTime"), attrs.value(QStringLiteral("preset_estimated_total_time")));
                    }
                    if (attrs.contains(QStringLiteral("preset_estimated_total_minutes"))) {
                        item.insert(QStringLiteral("cookerPresetEstimatedMinutes"), attrs.value(QStringLiteral("preset_estimated_total_minutes")));
                    }
                    if (attrs.contains(QStringLiteral("selected_holding_duration"))) {
                        item.insert(QStringLiteral("cookerSelectedHoldingDuration"), attrs.value(QStringLiteral("selected_holding_duration")));
                    }
                    if (attrs.contains(QStringLiteral("keep_warm_formatted"))) {
                        item.insert(QStringLiteral("cookerKeepWarmFormatted"), attrs.value(QStringLiteral("keep_warm_formatted")));
                    }
                    if (attrs.contains(QStringLiteral("keep_warm_seconds"))) {
                        item.insert(QStringLiteral("cookerKeepWarmSeconds"), attrs.value(QStringLiteral("keep_warm_seconds")));
                    }
                    if (attrs.contains(QStringLiteral("phase"))) {
                        item.insert(QStringLiteral("cookerPhase"), attrs.value(QStringLiteral("phase")));
                    }
                } else if (eid.contains(QStringLiteral("left_time")) || eid.contains(QStringLiteral("remaining_time"))) {
                    if (!stateVal.isEmpty() && stateVal != QStringLiteral("unknown") && stateVal != QStringLiteral("unavailable") && stateVal != QStringLiteral("0") && stateVal != QStringLiteral("900")) {
                        leftTimeFallback = stateVal;
                    }
                    if (!item.contains(QStringLiteral("cookerLeftTimeUnit")) && attrs.contains(QStringLiteral("unit_of_measurement"))) {
                        item.insert(QStringLiteral("cookerLeftTimeUnit"), attrs.value(QStringLiteral("unit_of_measurement")));
                    }
                }
                // 7. 锅内温度采集 (优先专用中文实体 guo_nei_wen_du，其次回退至 temperature 实体)
                else if (eid.contains(QStringLiteral("guo_nei_wen_du"))) {
                    if (!stateVal.isEmpty() && stateVal != QStringLiteral("unknown") && stateVal != QStringLiteral("unavailable")) {
                        tempExact = stateVal;
                    }
                } else if (eid.contains(QStringLiteral("temperature")) || eid.contains(QStringLiteral("temp"))) {
                    if (!stateVal.isEmpty() && stateVal != QStringLiteral("unknown") && stateVal != QStringLiteral("unavailable")) {
                        tempFallback = stateVal;
                    }
                }
                // 8. 锅盖合盖状态
                else if (eid.contains(QStringLiteral("guo_gai_he_gai")) || eid.contains(QStringLiteral("lid"))) {
                    QString lidText = attrs.value(QStringLiteral("status_zh")).toString().trimmed();
                    if (lidText.isEmpty()) {
                        if (stateVal == QStringLiteral("opened") || stateVal == QStringLiteral("open")) {
                            lidText = QStringLiteral("未合好/开盖");
                        } else if (stateVal == QStringLiteral("closed") || stateVal == QStringLiteral("close")) {
                            lidText = QStringLiteral("已合盖到位");
                        } else {
                            lidText = stateVal;
                        }
                    }
                    item.insert(QStringLiteral("cookerLidStatus"), lidText);
                }
                // 9. 手柄锁止状态
                else if (eid.contains(QStringLiteral("shou_bing_suo_zhi")) || eid.contains(QStringLiteral("lock"))) {
                    QString lockText = attrs.value(QStringLiteral("status_zh")).toString().trimmed();
                    if (lockText.isEmpty()) {
                        if (stateVal == QStringLiteral("unlocked") || stateVal == QStringLiteral("unlock")) {
                            lockText = QStringLiteral("未锁紧");
                        } else if (stateVal == QStringLiteral("locked") || stateVal == QStringLiteral("lock")) {
                            lockText = QStringLiteral("已旋转锁死");
                        } else {
                            lockText = stateVal;
                        }
                    }
                    item.insert(QStringLiteral("cookerLockStatus"), lockText);
                }
            }

            // 模式决策：若有传感器指示正在运行某模式，则当前模式为该运行模式；否则为选择的模式
            const QString finalCurrentMode = !runningModeVal.isEmpty() ? runningModeVal : selectedModeVal;
            if (!finalCurrentMode.isEmpty()) {
                item.insert(QStringLiteral("cookerCurrentMode"), finalCurrentMode);
            }
            if (!runningModeVal.isEmpty()) {
                item.insert(QStringLiteral("cookerRunningMode"), runningModeVal);
            }
            if (!selectedModeVal.isEmpty()) {
                item.insert(QStringLiteral("cookerSelectedMode"), selectedModeVal);
            }

            // 状态决策：优先精确状态，若未匹配到则使用回退状态，最后回退自身实体
            QString cookerStatusText = !statusExact.isEmpty() ? statusExact : statusFallback;
            if (cookerStatusText.isEmpty()) {
                const QVariantMap selfEntityState = m_entityStates.value(action.entityId.trimmed());
                cookerStatusText = selfEntityState.value(QStringLiteral("rawState"), selfEntityState.value(QStringLiteral("state"))).toString().trimmed();
            }

            if (!cookerStatusText.isEmpty()) {
                item.insert(QStringLiteral("state"), cookerStatusText);
                item.insert(QStringLiteral("stateText"), normalizedEntityStateText(cookerStatusText));
                item.insert(QStringLiteral("active"), entityStateIsActive(cookerStatusText));
                item.insert(QStringLiteral("available"), cookerStatusText != QStringLiteral("unavailable"));
            }

            const bool active = item.value(QStringLiteral("active")).toBool();
            const bool isKeepWarm = item.value(QStringLiteral("cookerIsKeepWarm")).toBool()
                                 || cookerStatusText.contains(QStringLiteral("保温"));
            if (isKeepWarm) {
                item.insert(QStringLiteral("isKeepWarm"), true);
                item.insert(QStringLiteral("cookerIsKeepWarm"), true);
            }

            const QString finalTemp = !tempExact.isEmpty() ? tempExact : tempFallback;
            if (!finalTemp.isEmpty()) {
                item.insert(QStringLiteral("cookerTemperature"), finalTemp);
            }

            const QString finalLeftTime = !leftTimeExact.isEmpty() ? leftTimeExact : leftTimeFallback;
            if (isKeepWarm && item.contains(QStringLiteral("cookerKeepWarmFormatted"))) {
                item.insert(QStringLiteral("cookerLeftTime"), item.value(QStringLiteral("cookerKeepWarmFormatted")).toString());
            } else if (!active || finalLeftTime == QStringLiteral("0") || finalLeftTime == QStringLiteral("0.0") || finalLeftTime == QStringLiteral("900") || finalLeftTime.isEmpty()) {
                item.insert(QStringLiteral("cookerLeftTime"), QString());
            } else {
                item.insert(QStringLiteral("cookerLeftTime"), finalLeftTime);
            }
        } else {
            const bool isWasher = action.domain.compare(QStringLiteral("washer"), Qt::CaseInsensitive) == 0
                               || action.domain.compare(QStringLiteral("washing_machine"), Qt::CaseInsensitive) == 0
                               || action.entityId.contains(QStringLiteral("washer"))
                               || action.entityId.contains(QStringLiteral("xi_yi"))
                               || action.name.contains(QStringLiteral("洗衣机"));
            if (isWasher) {
                item.insert(QStringLiteral("isWasher"), true);

                // 动态提取洗衣机设备特征 ID（如从 midea_123456789012345 提取 123456789012345，或使用 washer）
                QString washerDeviceId;
                static const QRegularExpression devIdRegex(QStringLiteral(R"((?:midea_)?(\d{6,})|(?:washer_[a-zA-Z0-9]+))"));
                const auto match = devIdRegex.match(action.entityId);
                if (match.hasMatch()) {
                    washerDeviceId = match.captured(1).isEmpty() ? match.captured(0) : match.captured(1);
                }
                if (washerDeviceId.isEmpty()) {
                    washerDeviceId = QStringLiteral("washer");
                }
                item.insert(QStringLiteral("washerDeviceId"), washerDeviceId);
                item.insert(QStringLiteral("washerModel"), QStringLiteral("SmartWasher"));

                QString runningStatus;
                QString progress;
                QString powerStatus = QStringLiteral("off");
                QString controlStatus = QStringLiteral("off");
                QString remainTime;
                bool hasDoorSensor = false;
                bool doorOpened = false;
                bool childLocked = false;
                bool detergentLack = false;
                QString program;
                QVariantList programOptions;
                QString temperature;
                QVariantList tempOptions;
                QString spinSpeed;
                QVariantList speedOptions;
                QString soakCount;
                QVariantList rinseOptions;
                QString waterLevel;
                QVariantList waterOptions;
                QString detergent;
                QVariantList detergentOptions;
                bool nightly = false;
                bool windDispel = false;
                double waterUsage = 0.0;
                double powerUsage = 0.0;
                QString lanIp;

                for (auto it = m_entityStates.constBegin(); it != m_entityStates.constEnd(); ++it) {
                    const QString &eid = it.key();
                    const bool matchDevice = (!washerDeviceId.isEmpty() && eid.contains(washerDeviceId))
                                          || eid.contains(QStringLiteral("washer"))
                                          || eid.contains(QStringLiteral("xi_yi"));
                    if (!matchDevice) {
                        continue;
                    }
                    const QVariantMap &stateMap = it.value();
                    const QString stateVal = stateMap.value(QStringLiteral("rawState"), stateMap.value(QStringLiteral("state"))).toString().trimmed();
                    const QVariantMap attrs = stateMap.value(QStringLiteral("attributes")).toMap();

                    if (eid.contains(QStringLiteral("_running_status"))) {
                        runningStatus = stateVal;
                    } else if (eid.contains(QStringLiteral("_progress"))) {
                        progress = stateVal;
                    } else if (eid.contains(QStringLiteral("_power_consumption_once"))) {
                        // 必须优先匹配耗电量传感器，防止被下方 _power 错误截获
                        powerUsage = stateVal.toDouble();
                    } else if (eid.endsWith(QStringLiteral("_power")) || (eid.startsWith(QStringLiteral("switch.")) && eid.contains(QStringLiteral("_power")))) {
                        powerStatus = stateVal;
                    } else if (eid.contains(QStringLiteral("_control_status"))) {
                        controlStatus = stateVal;
                    } else if (eid.contains(QStringLiteral("_remain_time"))) {
                        if (stateVal != QStringLiteral("unknown") && stateVal != QStringLiteral("unavailable") && stateVal != QStringLiteral("0")) {
                            remainTime = stateVal;
                        }
                    } else if (eid.contains(QStringLiteral("_door_opened")) || (eid.contains(QStringLiteral("_door")) && !eid.contains(QStringLiteral("_down_light")))) {
                        hasDoorSensor = true;
                        doorOpened = (stateVal == QStringLiteral("on") || stateVal == QStringLiteral("open"));
                    } else if (eid.contains(QStringLiteral("_detergent_lack"))) {
                        detergentLack = (stateVal == QStringLiteral("on"));
                    } else if (eid.startsWith(QStringLiteral("lock.")) && eid.contains(QStringLiteral("_lock"))) {
                        childLocked = (stateVal == QStringLiteral("locked") || stateVal == QStringLiteral("on"));
                    } else if (eid.contains(QStringLiteral("_program"))) {
                        program = stateVal;
                        if (attrs.contains(QStringLiteral("options"))) {
                            programOptions = attrs.value(QStringLiteral("options")).toList();
                        }
                    } else if (eid.contains(QStringLiteral("_temperature"))) {
                        temperature = stateVal;
                        if (attrs.contains(QStringLiteral("options"))) {
                            tempOptions = attrs.value(QStringLiteral("options")).toList();
                        }
                    } else if (eid.contains(QStringLiteral("_dehydration_speed"))) {
                        spinSpeed = stateVal;
                        if (attrs.contains(QStringLiteral("options"))) {
                            speedOptions = attrs.value(QStringLiteral("options")).toList();
                        }
                    } else if (eid.contains(QStringLiteral("_soak_count"))) {
                        soakCount = stateVal;
                        if (attrs.contains(QStringLiteral("options"))) {
                            rinseOptions = attrs.value(QStringLiteral("options")).toList();
                        }
                    } else if (eid.contains(QStringLiteral("_water_level"))) {
                        waterLevel = stateVal;
                        if (attrs.contains(QStringLiteral("options"))) {
                            waterOptions = attrs.value(QStringLiteral("options")).toList();
                        }
                    } else if (eid.contains(QStringLiteral("_detergent")) && !eid.contains(QStringLiteral("lack"))) {
                        detergent = stateVal;
                        if (attrs.contains(QStringLiteral("options"))) {
                            detergentOptions = attrs.value(QStringLiteral("options")).toList();
                        }
                    } else if (eid.contains(QStringLiteral("_nightly"))) {
                        nightly = (stateVal == QStringLiteral("on"));
                    } else if (eid.contains(QStringLiteral("_wind_dispel"))) {
                        windDispel = (stateVal == QStringLiteral("on"));
                    } else if (eid.contains(QStringLiteral("_water_consumption_once"))) {
                        waterUsage = stateVal.toDouble();
                    } else if (eid.contains(QStringLiteral("_lan_ip"))) {
                        lanIp = stateVal;
                    }

                    // 补充：检查聚合设备状态（包含 binary_sensor.midea_<id> 等）
                    const bool isAggregateDevice = (!washerDeviceId.isEmpty() && eid == QStringLiteral("binary_sensor.midea_%1").arg(washerDeviceId))
                                                || eid.contains(QStringLiteral("_device_status"))
                                                || (eid.startsWith(QStringLiteral("binary_sensor.")) && eid.contains(QStringLiteral("washer")));
                    if (isAggregateDevice) {
                        if (attrs.contains(QStringLiteral("power")) && powerStatus == QStringLiteral("off")) {
                            const QString attrPower = attrs.value(QStringLiteral("power")).toString().trimmed().toLower();
                            if (attrPower == QStringLiteral("on")) {
                                powerStatus = QStringLiteral("on");
                            }
                        }
                        if (attrs.contains(QStringLiteral("program")) && (program.isEmpty() || program == QStringLiteral("unknown"))) {
                            program = attrs.value(QStringLiteral("program")).toString().trimmed();
                        }
                        if (attrs.contains(QStringLiteral("running_status")) && (runningStatus.isEmpty() || runningStatus == QStringLiteral("unknown"))) {
                            runningStatus = attrs.value(QStringLiteral("running_status")).toString().trimmed();
                        }
                        if (attrs.contains(QStringLiteral("progress")) && (progress.isEmpty() || progress == QStringLiteral("unknown"))) {
                            progress = attrs.value(QStringLiteral("progress")).toString().trimmed();
                        }
                        if (attrs.contains(QStringLiteral("remain_time")) && remainTime.isEmpty()) {
                            const QString rt = attrs.value(QStringLiteral("remain_time")).toString().trimmed();
                            if (!rt.isEmpty() && rt != QStringLiteral("0")) {
                                remainTime = rt;
                            }
                        }
                    }
                }

                // 综合判定开机状态：显式开关为 on，或者正在洗涤/漂洗/脱水/运行/暂停等，或者启停开关为 on
                bool isPoweredOn = (powerStatus == QStringLiteral("on"));
                const QString lowerRunningStatus = runningStatus.toLower();
                const QString lowerProgress = progress.toLower();
                if (!isPoweredOn) {
                    if (controlStatus == QStringLiteral("on")) {
                        isPoweredOn = true;
                        powerStatus = QStringLiteral("on");
                    } else if (!lowerRunningStatus.isEmpty()
                               && lowerRunningStatus != QStringLiteral("off")
                               && lowerRunningStatus != QStringLiteral("idle")
                               && lowerRunningStatus != QStringLiteral("standby")
                               && lowerRunningStatus != QStringLiteral("unknown")
                               && lowerRunningStatus != QStringLiteral("unavailable")) {
                        isPoweredOn = true;
                        powerStatus = QStringLiteral("on");
                    }
                }

                item.insert(QStringLiteral("washerRunningStatus"), runningStatus);
                item.insert(QStringLiteral("washerProgress"), progress);
                item.insert(QStringLiteral("washerPower"), powerStatus);
                item.insert(QStringLiteral("washerControlStatus"), controlStatus);
                item.insert(QStringLiteral("washerRemainTime"), remainTime);
                item.insert(QStringLiteral("washerHasDoorSensor"), hasDoorSensor);
                item.insert(QStringLiteral("washerDoorOpened"), doorOpened);
                item.insert(QStringLiteral("washerChildLock"), childLocked);
                item.insert(QStringLiteral("washerDetergentLack"), detergentLack);
                item.insert(QStringLiteral("washerProgram"), program);
                if (!programOptions.isEmpty()) item.insert(QStringLiteral("washerProgramOptions"), programOptions);
                item.insert(QStringLiteral("washerTemp"), temperature);
                if (!tempOptions.isEmpty()) item.insert(QStringLiteral("washerTempOptions"), tempOptions);
                item.insert(QStringLiteral("washerSpeed"), spinSpeed);
                item.insert(QStringLiteral("washerSpeedOptions"), speedOptions);
                item.insert(QStringLiteral("washerRinseCount"), soakCount);
                if (!rinseOptions.isEmpty()) item.insert(QStringLiteral("washerRinseOptions"), rinseOptions);
                item.insert(QStringLiteral("washerWaterLevel"), waterLevel);
                if (!waterOptions.isEmpty()) item.insert(QStringLiteral("washerWaterOptions"), waterOptions);
                item.insert(QStringLiteral("washerDetergent"), detergent);
                if (!detergentOptions.isEmpty()) item.insert(QStringLiteral("washerDetergentOptions"), detergentOptions);
                item.insert(QStringLiteral("washerNightly"), nightly);
                item.insert(QStringLiteral("washerWindDispel"), windDispel);
                item.insert(QStringLiteral("washerWaterUsage"), waterUsage);
                item.insert(QStringLiteral("washerPowerUsage"), powerUsage);
                if (!lanIp.isEmpty()) item.insert(QStringLiteral("washerLanIp"), lanIp);

                // 综合判定正在运行（Active）
                const bool isWorkStatus = (lowerRunningStatus == QStringLiteral("start")
                                        || lowerRunningStatus == QStringLiteral("run")
                                        || lowerRunningStatus == QStringLiteral("running")
                                        || lowerRunningStatus == QStringLiteral("wash")
                                        || lowerRunningStatus == QStringLiteral("rinse")
                                        || lowerRunningStatus == QStringLiteral("spin")
                                        || lowerRunningStatus == QStringLiteral("dehydration")
                                        || lowerRunningStatus == QStringLiteral("drying")
                                        || lowerRunningStatus == QStringLiteral("dry")
                                        || lowerRunningStatus == QStringLiteral("soak")
                                        || lowerRunningStatus == QStringLiteral("pause"));
                const bool isWorkProgress = (lowerProgress == QStringLiteral("wash")
                                          || lowerProgress == QStringLiteral("rinse")
                                          || lowerProgress == QStringLiteral("spin")
                                          || lowerProgress == QStringLiteral("dehydration")
                                          || lowerProgress == QStringLiteral("drying")
                                          || lowerProgress == QStringLiteral("dry")
                                          || lowerProgress == QStringLiteral("soak"));
                const bool isRunning = isPoweredOn && (isWorkStatus || isWorkProgress || (controlStatus == QStringLiteral("on") && lowerRunningStatus != QStringLiteral("standby") && lowerRunningStatus != QStringLiteral("idle") && lowerRunningStatus != QStringLiteral("end")));
                item.insert(QStringLiteral("active"), isRunning);
                item.insert(QStringLiteral("available"), true);

                QString statusText;
                if (!isPoweredOn) {
                    statusText = tr("已关机");
                } else if (lowerRunningStatus == QStringLiteral("pause")) {
                    statusText = tr("已暂停");
                } else if (lowerRunningStatus == QStringLiteral("end") || lowerRunningStatus == QStringLiteral("finish") || lowerProgress == QStringLiteral("end")) {
                    statusText = tr("洗涤完成");
                } else if (lowerProgress == QStringLiteral("wash") || lowerRunningStatus == QStringLiteral("wash")) {
                    statusText = tr("洗涤中");
                } else if (lowerProgress == QStringLiteral("rinse") || lowerRunningStatus == QStringLiteral("rinse")) {
                    statusText = tr("漂洗中");
                } else if (lowerProgress == QStringLiteral("spin") || lowerProgress == QStringLiteral("dehydration") || lowerRunningStatus == QStringLiteral("spin") || lowerRunningStatus == QStringLiteral("dehydration")) {
                    statusText = tr("脱水中");
                } else if (lowerProgress == QStringLiteral("drying") || lowerProgress == QStringLiteral("dry") || lowerRunningStatus == QStringLiteral("drying") || lowerRunningStatus == QStringLiteral("dry")) {
                    statusText = tr("烘干中");
                } else if (lowerProgress == QStringLiteral("soak") || lowerRunningStatus == QStringLiteral("soak")) {
                    statusText = tr("浸泡中");
                } else if (lowerRunningStatus == QStringLiteral("start") || lowerRunningStatus == QStringLiteral("run") || lowerRunningStatus == QStringLiteral("running")) {
                    statusText = tr("运行中");
                } else if (lowerRunningStatus == QStringLiteral("idle") || lowerRunningStatus == QStringLiteral("standby")) {
                    statusText = tr("待机中");
                } else {
                    statusText = tr("待机中");
                }
                item.insert(QStringLiteral("state"), isPoweredOn ? (runningStatus.isEmpty() ? QStringLiteral("standby") : runningStatus) : QStringLiteral("off"));
                item.insert(QStringLiteral("stateText"), statusText);
            }
        }

        result.append(item);
    }
    return result;
}

HomeAssistantModule::HomeAssistantModule(GlobalState *globalState, QObject *parent)
    : IModule(globalState, parent)
    , m_worker(new HomeAssistantWorker)
{
    // 工作对象放到独立线程，和视频模块保持一致的结构。
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &HomeAssistantWorker::statusChanged, globalState, &GlobalState::setHaStatus);
    connect(m_worker, &HomeAssistantWorker::actionStatesChanged, globalState, &GlobalState::setHaActionStates);
    m_workerThread.start();
}

HomeAssistantModule::~HomeAssistantModule()
{
    stop();
    m_workerThread.quit();
    m_workerThread.wait();
}

QString HomeAssistantModule::name() const
{
    return QStringLiteral("homeassistant");
}

void HomeAssistantModule::applyConfig(const AppConfig &config)
{
    m_config = config.homeAssistant;
}

void HomeAssistantModule::start()
{
    QMetaObject::invokeMethod(m_worker, "initialize", Qt::QueuedConnection,
                              Q_ARG(HomeAssistantConfig, m_config));
}

void HomeAssistantModule::stop()
{
    QMetaObject::invokeMethod(m_worker, "shutdown", Qt::QueuedConnection);
}

void HomeAssistantModule::triggerAction(const QString &actionName)
{
    QMetaObject::invokeMethod(m_worker, "triggerAction", Qt::QueuedConnection,
                              Q_ARG(QString, actionName));
}

void HomeAssistantModule::callActionService(const QString &actionName, const QString &service)
{
    QMetaObject::invokeMethod(m_worker, "callActionService", Qt::QueuedConnection,
                              Q_ARG(QString, actionName),
                              Q_ARG(QString, service));
}

void HomeAssistantModule::callCustomService(const QString &domain, const QString &service, const QVariantMap &data)
{
    QMetaObject::invokeMethod(m_worker, "callCustomService", Qt::QueuedConnection,
                              Q_ARG(QString, domain),
                              Q_ARG(QString, service),
                              Q_ARG(QVariantMap, data));
}

void HomeAssistantModule::setLightBrightness(const QString &actionName, qreal brightness)
{
    QMetaObject::invokeMethod(m_worker, "setLightBrightness", Qt::QueuedConnection,
                              Q_ARG(QString, actionName),
                              Q_ARG(qreal, brightness));
}

void HomeAssistantModule::setCoverPosition(const QString &actionName, qreal position)
{
    QMetaObject::invokeMethod(m_worker, "setCoverPosition", Qt::QueuedConnection,
                              Q_ARG(QString, actionName),
                              Q_ARG(qreal, position));
}

void HomeAssistantModule::setMediaPlayerVolume(const QString &actionName, qreal volume)
{
    QMetaObject::invokeMethod(m_worker, "setMediaPlayerVolume", Qt::QueuedConnection,
                              Q_ARG(QString, actionName),
                              Q_ARG(qreal, volume));
}
