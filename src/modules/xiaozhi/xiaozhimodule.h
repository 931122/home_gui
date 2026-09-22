#ifndef XIAOZHIMODULE_H
#define XIAOZHIMODULE_H

#include <QAbstractSocket>
#include <QFile>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QUdpSocket>
#include <QWebSocket>

#include "core/imodule.h"

class XiaozhiWorker : public QObject
{
    Q_OBJECT

public slots:
    void initialize(const XiaozhiConfig &config);
    void shutdown();
    void wake();
    void hideOverlay();

signals:
    void availableChanged(bool available);
    void overlayVisibleChanged(bool visible);
    void stateChanged(const QString &state);
    void statusChanged(const QString &status);
    void textChanged(const QString &text);

private slots:
    void handleAudioDatagrams();
    void handleUiDatagrams();
    void handleActivationReply();
    void connectWebSocket();
    void handleTextMessage(const QString &message);
    void handleBinaryMessage(const QByteArray &message);
    void handleGpioPoll();

private:
    enum DeviceState {
        StateUnknown = 0,
        StateStarting,
        StateWifiConfiguring,
        StateIdle,
        StateConnecting,
        StateListening,
        StateSpeaking,
        StateUpgrading,
        StateActivating,
        StateFatalError
    };

    bool configureSockets();
    void closeSockets();
    void startAudioProcess();
    void stopAudioProcess();
    void ensureUuid();
    QString readUuid() const;
    bool writeUuid(const QString &uuid) const;
    QString generateUuid() const;
    QString deviceId() const;
    void activateDevice();
    void sendHello();
    void sendStartListening(const QString &mode);
    void sendIotDescriptors();
    void sendIotState();
    void sendWebSocketText(const QJsonObject &payload);
    void sendUiState(DeviceState state);
    void sendUiText(const QString &text);
    void setOverlayVisible(bool visible);
    void setState(DeviceState state);
    QString stateLabel(DeviceState state) const;
    void processServerJson(const QJsonObject &object);
    bool prepareGpio();
    void releaseGpio();
    bool writeTextFile(const QString &path, const QString &value) const;

    XiaozhiConfig m_config;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    QNetworkReply *m_activationReply = nullptr;
    QWebSocket *m_webSocket = nullptr;
    QProcess *m_audioProcess = nullptr;
    QUdpSocket *m_audioSocket = nullptr;
    QUdpSocket *m_uiSocket = nullptr;
    QTimer *m_activationRetryTimer = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QTimer *m_overlayHideTimer = nullptr;
    QTimer *m_gpioPollTimer = nullptr;
    QString m_uuid;
    QString m_sessionId;
    bool m_audioUploadEnabled = true;
    bool m_overlayVisible = false;
    bool m_shuttingDown = false;
    bool m_pendingWake = false;
    int m_reconnectDelayMs = 3000;
    int m_lastGpioValue = -1;
};

class XiaozhiModule : public IModule
{
    Q_OBJECT

public:
    explicit XiaozhiModule(GlobalState *globalState, QObject *parent = nullptr);
    ~XiaozhiModule() override;

    QString name() const override;
    void applyConfig(const AppConfig &config) override;
    void start() override;
    void stop() override;
    void wake();
    void hideOverlay();

private:
    XiaozhiConfig m_config;
    QThread m_workerThread;
    XiaozhiWorker *m_worker;
};

#endif
