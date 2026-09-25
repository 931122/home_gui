#ifndef HOMEASSISTANTMODULE_H
#define HOMEASSISTANTMODULE_H

#include <QNetworkAccessManager>
#include <QHash>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QVariantList>

#include "core/imodule.h"

class QWebSocket;

class HomeAssistantWorker : public QObject
{
    Q_OBJECT

public slots:
    // 初始化 Home Assistant 连接状态。
    void initialize(const HomeAssistantConfig &config);
    // 关闭连接并回写停止状态。
    void shutdown();
    // 按动作名称调用 Home Assistant 服务。
    void triggerAction(const QString &actionName);
    void callActionService(const QString &actionName, const QString &service);
    void callCustomService(const QString &domain, const QString &service, const QVariantMap &data = QVariantMap());
    void setLightBrightness(const QString &actionName, qreal brightness);
    void setCoverPosition(const QString &actionName, qreal position);
    void setMediaPlayerVolume(const QString &actionName, qreal volume);
    void refreshActionStates();

signals:
    void statusChanged(const QString &status);
    void actionStatesChanged(const QVariantList &actionStates);

private:
    bool findAction(const QString &actionName, HomeAssistantActionConfig &matchedAction) const;
    void sendServiceRequest(const HomeAssistantActionConfig &action,
                            const QString &service,
                            const QJsonObject &payload,
                            const QString &successStatus);
    void abortCurrentReply();
    void connectWebSocket();
    void disconnectWebSocket();
    void handleWebSocketMessage(const QString &message);
    QVariantList buildActionStates() const;
    void scheduleActionStatesUpdate();
    bool isEntityRelevant(const QString &entityId) const;
    void updateRelevantEntitiesCache();
    void startFetchingActionStates();
    void fetchNextActionState(int index, QVariantList result, int generation, bool hasError = false);
    void trackReply(QNetworkReply *reply, int timeoutMs = 3000);
    void startReconnectTimer();

    HomeAssistantConfig m_config;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QWebSocket *m_webSocket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QTimer *m_actionStatesDebounceTimer = nullptr;
    int m_reconnectDelayMs = 5000;
    QHash<QString, QVariantMap> m_entityStates;
    QSet<QString> m_relevantEntityIds;
    bool m_hasCooker = false;
    bool m_hasWasher = false;
    bool m_hasSteamer = false;
    QSet<QNetworkReply *> m_activeReplies;
    int m_requestGeneration = 0;
    qint64 m_nextCommandId = 1;
    qint64 m_getStatesCommandId = 0;
    qint64 m_subscribeStatesCommandId = 0;
};

class HomeAssistantModule : public IModule
{
    Q_OBJECT

public:
    // Home Assistant 模块封装为独立线程，避免后续 MQTT 阻塞主线程。
    explicit HomeAssistantModule(GlobalState *globalState, QObject *parent = nullptr);
    ~HomeAssistantModule() override;

    QString name() const override;
    void applyConfig(const AppConfig &config) override;
    void start() override;
    void stop() override;
    void triggerAction(const QString &actionName);
    void callActionService(const QString &actionName, const QString &service);
    void callCustomService(const QString &domain, const QString &service, const QVariantMap &data = QVariantMap());
    void setLightBrightness(const QString &actionName, qreal brightness);
    void setCoverPosition(const QString &actionName, qreal position);
    void setMediaPlayerVolume(const QString &actionName, qreal volume);

private:
    HomeAssistantConfig m_config;
    QThread m_workerThread;
    HomeAssistantWorker *m_worker;
};

#endif
