#ifndef WIFIMODULE_H
#define WIFIMODULE_H

#include <QThread>
#include <QTimer>

#include "core/imodule.h"

class WifiWorker : public QObject
{
    Q_OBJECT

public slots:
    void initialize(const WifiConfig &config);
    void shutdown();
    void scanNetworks();
    void connectToNetwork(const QString &ssid, const QString &password);
    void forgetNetwork(const QString &ssid);

signals:
    void availableChanged(bool available);
    void scanningChanged(bool scanning);
    void interfaceChanged(const QString &interfaceName);
    void statusChanged(const QString &status);
    void networksChanged(const QVariantList &networks);

private slots:
    void refreshState();
    void refreshScanResults();
    void connectUsingStartupPolicy();

private:
    bool ensureWifiControlReady();
    QString detectWifiInterface() const;
    QString detectWifiCtrlPath() const;
    QString detectWifiConfigPath() const;
    QString detectInterfaceIpAddress() const;
    QVariantMap configuredNetworksBySsid() const;
    QStringList findConfiguredNetworkIds(const QString &ssid) const;
    bool removeNetwork(const QString &networkId) const;
    void requestDhcpLease();
    bool startWifiSupplicant(const QString &ctrlPath);
    bool runWifiCommand(const QStringList &arguments, QString *stdoutText, QString *stderrText = nullptr) const;

    // 辅助异步工具，后续可扩展为真正的任务队列。
    struct CommandResult {
        bool success;
        QString stdoutText;
        QString stderrText;
    };
    CommandResult runWifiCommandSync(const QStringList &arguments, int timeoutMs = 5000) const;

    QTimer *m_refreshTimer = nullptr;
    QTimer *m_connectionTimer = nullptr;
    WifiConfig m_config;
    QString m_wifiInterface;
    QString m_wifiCtrlPath;
    bool m_wifiAvailable = false;
    bool m_wifiScanning = false;
    bool m_isRequestingDhcp = false;
    QString m_pendingConnectionSsid;
    int m_dhcpRetryCount = 0;
};

class WifiModule : public IModule
{
    Q_OBJECT

public:
    explicit WifiModule(GlobalState *globalState, QObject *parent = nullptr);
    ~WifiModule() override;

    QString name() const override;
    void applyConfig(const AppConfig &config) override;
    void start() override;
    void stop() override;

    void scanNetworks();
    void connectToNetwork(const QString &ssid, const QString &password);
    void forgetNetwork(const QString &ssid);

private:
    WifiConfig m_config;
    QThread m_workerThread;
    WifiWorker *m_worker;
};

#endif
