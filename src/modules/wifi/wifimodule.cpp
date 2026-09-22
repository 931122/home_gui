#include "wifimodule.h"

#include <QDir>
#include <QFileInfo>
#include <QNetworkInterface>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>

#include "core/globalstate.h"

namespace {

QString firstLine(const QString &value)
{
    const int newlineIndex = value.indexOf(QLatin1Char('\n'));
    return newlineIndex >= 0 ? value.left(newlineIndex).trimmed() : value.trimmed();
}

QString quotedWpaValue(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}

QStringList nonEmptyLines(const QString &value)
{
    QStringList lines = value.split(QLatin1Char('\n'));
    lines.removeAll(QString());
    return lines;
}

} // namespace

void WifiWorker::initialize(const WifiConfig &config)
{
    m_config = config;
    // Wi-Fi 模块是“轮询 + 命令执行”模型：
    // 定时刷新状态，用户动作再触发 scan/connect。
    if (m_refreshTimer == nullptr) {
        m_refreshTimer = new QTimer(this);
        connect(m_refreshTimer, &QTimer::timeout, this, &WifiWorker::refreshState);
    }
    if (m_connectionTimer == nullptr) {
        m_connectionTimer = new QTimer(this);
        m_connectionTimer->setSingleShot(true);
        m_connectionTimer->setInterval(30000); // 只覆盖认证/关联阶段，DHCP 超时单独处理。
        connect(m_connectionTimer, &QTimer::timeout, this, [this]() {
            if (!m_pendingConnectionSsid.isEmpty()) {
                emit statusChanged(tr("Connection to %1 timed out").arg(m_pendingConnectionSsid));
                m_pendingConnectionSsid.clear();
                refreshState();
            }
        });
    }
    m_refreshTimer->start(15000);
    refreshState();
    QTimer::singleShot(500, this, &WifiWorker::connectUsingStartupPolicy);
}

void WifiWorker::shutdown()
{
    if (m_refreshTimer != nullptr) {
        m_refreshTimer->stop();
    }
    if (m_connectionTimer != nullptr) {
        m_connectionTimer->stop();
    }
    emit scanningChanged(false);
    emit statusChanged(tr("Wi-Fi stopped"));
}

void WifiWorker::scanNetworks()
{
    refreshState();
    if (!m_wifiAvailable || m_wifiInterface.isEmpty() || !ensureWifiControlReady()) {
        emit statusChanged(tr("Wi-Fi unavailable"));
        emit scanningChanged(false);
        return;
    }

    QString stdoutText;
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("scan"),
                        &stdoutText)) {
        emit statusChanged(tr("Wi-Fi scan failed"));
        emit scanningChanged(false);
        return;
    }

    m_wifiScanning = true;
    emit scanningChanged(true);
    emit statusChanged(tr("Scanning Wi-Fi..."));
    QTimer::singleShot(2500, this, &WifiWorker::refreshScanResults);
}

void WifiWorker::connectToNetwork(const QString &ssid, const QString &password)
{
    refreshState();
    if (!m_wifiAvailable || m_wifiInterface.isEmpty() || !ensureWifiControlReady()) {
        emit statusChanged(tr("Wi-Fi unavailable"));
        return;
    }
    if (m_connectionTimer != nullptr) {
        m_connectionTimer->stop();
    }

    QString stdoutText;
    QString networkId;
    bool createdNetwork = false;
    const QStringList existingNetworkIds = findConfiguredNetworkIds(ssid);
    if (!existingNetworkIds.isEmpty()) {
        networkId = existingNetworkIds.first();
        for (int index = 1; index < existingNetworkIds.size(); ++index) {
            removeNetwork(existingNetworkIds.at(index));
        }
    } else {
        if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("add_network"),
                            &stdoutText)) {
            emit statusChanged(tr("Wi-Fi connect failed: add_network error"));
            return;
        }

        networkId = firstLine(stdoutText);
        // add_network 应该返回一个数字。
        bool isIdOk = false;
        networkId.toInt(&isIdOk);
        if (!isIdOk) {
            emit statusChanged(tr("Wi-Fi connect failed: invalid network ID"));
            return;
        }
        createdNetwork = true;
    }

    if (networkId.isEmpty()) {
        emit statusChanged(tr("Wi-Fi connect failed"));
        return;
    }

    // 这里沿用 wpa_cli 的网络配置流，保证连过一次后能被 save_config 持久化。
    const QStringList baseArgs = QStringList() << QStringLiteral("-i") << m_wifiInterface
                                               << QStringLiteral("set_network") << networkId;
    if (!runWifiCommand(baseArgs + QStringList{QStringLiteral("ssid"), quotedWpaValue(ssid)}, &stdoutText)) {
        if (createdNetwork) {
            removeNetwork(networkId);
        }
        emit statusChanged(tr("Wi-Fi set SSID failed"));
        return;
    }

    if (!password.isEmpty()) {
        if (password.length() < 8) {
            if (createdNetwork) {
                removeNetwork(networkId);
            }
            emit statusChanged(tr("Password too short (min 8 chars)"));
            return;
        }
        if (!runWifiCommand(baseArgs + QStringList{QStringLiteral("psk"), quotedWpaValue(password)}, &stdoutText)) {
            if (createdNetwork) {
                removeNetwork(networkId);
            }
            emit statusChanged(tr("Wi-Fi password rejected"));
            return;
        }
    } else {
        if (!runWifiCommand(baseArgs + QStringList{QStringLiteral("key_mgmt"), QStringLiteral("NONE")}, &stdoutText)) {
            if (createdNetwork) {
                removeNetwork(networkId);
            }
            emit statusChanged(tr("Wi-Fi security config failed"));
            return;
        }
    }

    // 允许扫描隐藏 SSID
    if (!runWifiCommand(baseArgs + QStringList{QStringLiteral("scan_ssid"), QStringLiteral("1")}, &stdoutText)) {
        if (createdNetwork) {
            removeNetwork(networkId);
        }
        emit statusChanged(tr("Wi-Fi hidden scan config failed"));
        return;
    }

    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("enable_network") << networkId, &stdoutText)) {
        emit statusChanged(tr("Wi-Fi enable failed"));
        return;
    }

    // 确保开启了主动扫描并强制重连
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("ap_scan") << QStringLiteral("1"), &stdoutText)) {
        emit statusChanged(tr("Wi-Fi scan mode config failed"));
        return;
    }
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("select_network") << networkId, &stdoutText)) {
        emit statusChanged(tr("Wi-Fi select network failed"));
        return;
    }
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("reassociate"), &stdoutText)) {
        emit statusChanged(tr("Wi-Fi reassociate failed"));
        return;
    }
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("save_config"), &stdoutText)) {
        emit statusChanged(tr("Wi-Fi save config failed"));
        return;
    }

    emit statusChanged(tr("Connecting to %1...").arg(ssid));
    m_pendingConnectionSsid = ssid.trimmed();
    if (m_connectionTimer != nullptr) {
        m_connectionTimer->start();
    }
    m_dhcpRetryCount = 0;
    // 触发立即刷新状态，以便捕获初始连接过程。
    QTimer::singleShot(500, this, &WifiWorker::refreshState);
}

void WifiWorker::forgetNetwork(const QString &ssid)
{
    refreshState();
    if (!m_wifiAvailable || m_wifiInterface.isEmpty() || !ensureWifiControlReady()) {
        emit statusChanged(tr("Wi-Fi unavailable"));
        return;
    }

    const QStringList networkIds = findConfiguredNetworkIds(ssid);
    bool removed = false;
    for (const QString &networkId : networkIds) {
        removed = removeNetwork(networkId) || removed;
    }

    if (!removed) {
        emit statusChanged(tr("Wi-Fi not saved: %1").arg(ssid));
        return;
    }

    QString stdoutText;
    runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("save_config"), &stdoutText);
    if (m_pendingConnectionSsid == ssid.trimmed()) {
        m_pendingConnectionSsid.clear();
        m_dhcpRetryCount = 0;
        if (m_connectionTimer != nullptr) {
            m_connectionTimer->stop();
        }
    }
    emit statusChanged(tr("Forgot Wi-Fi: %1").arg(ssid));
    refreshState();
    refreshScanResults();
}

void WifiWorker::refreshState()
{
    const QString nextInterface = detectWifiInterface();
    if (m_wifiInterface != nextInterface) {
        m_wifiInterface = nextInterface;
        emit interfaceChanged(m_wifiInterface);
    }

    const bool nextAvailable = !m_wifiInterface.isEmpty()
            && !QStandardPaths::findExecutable(QStringLiteral("wpa_cli")).isEmpty()
            && !QStandardPaths::findExecutable(QStringLiteral("wpa_supplicant")).isEmpty();
    if (m_wifiAvailable != nextAvailable) {
        m_wifiAvailable = nextAvailable;
        emit availableChanged(m_wifiAvailable);
    }

    if (!m_wifiAvailable) {
        m_wifiCtrlPath.clear();
        emit scanningChanged(false);
        emit networksChanged(QVariantList());
        emit statusChanged(tr("Wi-Fi unavailable"));
        return;
    }

    if (!ensureWifiControlReady()) {
        emit scanningChanged(false);
        emit networksChanged(QVariantList());
        emit statusChanged(tr("Wi-Fi service unavailable"));
        return;
    }

    QString stdoutText;
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("status"),
                        &stdoutText)) {
        emit statusChanged(tr("Wi-Fi idle"));
        return;
    }

    QString nextStatus = tr("Wi-Fi idle");
    const QStringList lines = nonEmptyLines(stdoutText);
    QString ssid;
    QString ipAddress;
    QString wpaState;
    for (const QString &line : lines) {
        const int separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }
        const QString key = line.left(separator).trimmed();
        const QString value = line.mid(separator + 1).trimmed();
        if (key == QStringLiteral("ssid")) {
            ssid = value;
        } else if (key == QStringLiteral("ip_address")) {
            ipAddress = value;
        } else if (key == QStringLiteral("wpa_state")) {
            wpaState = value;
        }
    }

    if (ipAddress.isEmpty()) {
        ipAddress = detectInterfaceIpAddress();
    }

    if (wpaState == QStringLiteral("COMPLETED") && !ssid.isEmpty()) {
        if (ipAddress.isEmpty()) {
            if (m_connectionTimer != nullptr) {
                m_connectionTimer->stop();
            }
            if (!m_isRequestingDhcp) {
                requestDhcpLease();
            }
            nextStatus = tr("Connected to %1, requesting IP...").arg(ssid);
        } else {
            nextStatus = tr("Connected: %1 (%2)").arg(ssid, ipAddress);
            m_pendingConnectionSsid.clear();
            m_dhcpRetryCount = 0;
            m_isRequestingDhcp = false;
            if (m_connectionTimer != nullptr) {
                m_connectionTimer->stop();
            }
        }
    } else if (wpaState == QStringLiteral("AUTHENTICATING") || wpaState == QStringLiteral("ASSOCIATING")) {
        nextStatus = tr("Authenticating with %1...").arg(ssid.isEmpty() ? m_pendingConnectionSsid : ssid);
    } else if (wpaState == QStringLiteral("4WAY_HANDSHAKE") || wpaState == QStringLiteral("GROUP_HANDSHAKE")) {
        nextStatus = tr("Handshaking with %1...").arg(ssid.isEmpty() ? m_pendingConnectionSsid : ssid);
    } else if (wpaState == QStringLiteral("SCANNING")) {
        nextStatus = m_pendingConnectionSsid.isEmpty() ? tr("Scanning...") : tr("Searching for %1...").arg(m_pendingConnectionSsid);
    } else if (wpaState == QStringLiteral("INACTIVE") || wpaState == QStringLiteral("DISCONNECTED")) {
        nextStatus = m_pendingConnectionSsid.isEmpty() ? tr("Wi-Fi idle") : tr("Waiting for %1...").arg(m_pendingConnectionSsid);
    } else if (!ssid.isEmpty()) {
        nextStatus = tr("Connecting to %1...").arg(ssid);
    } else if (!m_pendingConnectionSsid.isEmpty()) {
        nextStatus = tr("Connecting to %1...").arg(m_pendingConnectionSsid);
    }

    emit statusChanged(nextStatus);
}

void WifiWorker::refreshScanResults()
{
    m_wifiScanning = false;
    emit scanningChanged(false);

    if (!m_wifiAvailable || m_wifiInterface.isEmpty() || !ensureWifiControlReady()) {
        emit networksChanged(QVariantList());
        emit statusChanged(tr("Wi-Fi unavailable"));
        return;
    }

    QString stdoutText;
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("scan_results"),
                        &stdoutText)) {
        emit statusChanged(tr("Wi-Fi scan failed"));
        return;
    }

    const QVariantMap configuredNetworks = configuredNetworksBySsid();
    QVariantList nextNetworks;
    const QStringList lines = nonEmptyLines(stdoutText);
    for (int index = 1; index < lines.size(); ++index) {
        const QString line = lines.at(index);
        const QStringList columns = line.split(QLatin1Char('\t'));
        if (columns.size() < 5) {
            continue;
        }

        const QString ssid = columns.mid(4).join(QStringLiteral("\t")).trimmed();
        if (ssid.isEmpty()) {
            continue;
        }

        QVariantMap item;
        item.insert(QStringLiteral("ssid"), ssid);
        item.insert(QStringLiteral("signal"), columns.value(2));
        item.insert(QStringLiteral("flags"), columns.value(3));
        item.insert(QStringLiteral("connected"), columns.value(3).contains(QStringLiteral("[CURRENT]")));
        item.insert(QStringLiteral("saved"), configuredNetworks.contains(ssid));
        nextNetworks.append(item);
    }

    emit networksChanged(nextNetworks);
    // 只有在空闲时才更新状态文本，避免覆盖连接中的状态。
    if (m_pendingConnectionSsid.isEmpty()) {
        emit statusChanged(nextNetworks.isEmpty()
                           ? tr("No Wi-Fi networks found")
                           : tr("Found %1 Wi-Fi networks").arg(nextNetworks.size()));
    }
}

void WifiWorker::connectUsingStartupPolicy()
{
    refreshState();
    if (!m_config.enabled || !m_wifiAvailable || m_wifiInterface.isEmpty() || !ensureWifiControlReady()) {
        return;
    }

    QString stdoutText;
    if (!m_config.ssid.trimmed().isEmpty()) {
        connectToNetwork(m_config.ssid.trimmed(), m_config.password);
        return;
    }

    runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("reconnect"),
                   &stdoutText);
    emit statusChanged(tr("Restoring previous Wi-Fi..."));
    m_pendingConnectionSsid.clear();
    m_dhcpRetryCount = 0;
    QTimer::singleShot(2500, this, [this]() {
        requestDhcpLease();
        ++m_dhcpRetryCount;
        refreshState();
    });
}

bool WifiWorker::ensureWifiControlReady()
{
    if (m_wifiInterface.isEmpty()) {
        return false;
    }

    // 先复用已存在的 wpa_supplicant 控制口；没有的话再尝试自启动。
    const QString nextCtrlPath = detectWifiCtrlPath();
    if (!nextCtrlPath.isEmpty()) {
        m_wifiCtrlPath = nextCtrlPath;
        return true;
    }

    const QString defaultCtrlPath = QStringLiteral("/var/run/wpa_supplicant");
    if (!startWifiSupplicant(defaultCtrlPath)) {
        return false;
    }

    m_wifiCtrlPath = detectWifiCtrlPath();
    return !m_wifiCtrlPath.isEmpty();
}

QString WifiWorker::detectWifiInterface() const
{
    if (!m_config.interfaceName.trimmed().isEmpty()) {
        return m_config.interfaceName.trimmed();
    }

    const QDir netRoot(QStringLiteral("/sys/class/net"));
    const QFileInfoList entries = netRoot.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &entry : entries) {
        const QString name = entry.fileName();
        if (QFileInfo(entry.absoluteFilePath() + QStringLiteral("/wireless")).exists()
            || name.startsWith(QStringLiteral("wlan"))
            || name.startsWith(QStringLiteral("wl"))) {
            return name;
        }
    }
    return QString();
}

QString WifiWorker::detectWifiCtrlPath() const
{
    if (m_wifiInterface.isEmpty()) {
        return QString();
    }

    const QStringList candidatePaths = {
        QStringLiteral("/var/run/wpa_supplicant"),
        QStringLiteral("/run/wpa_supplicant")
    };

    for (const QString &path : candidatePaths) {
        if (QFileInfo(path + QLatin1Char('/') + m_wifiInterface).exists()) {
            return path;
        }
    }

    return QString();
}

QString WifiWorker::detectWifiConfigPath() const
{
    const QStringList candidatePaths = {
        QStringLiteral("/etc/wpa_supplicant.conf"),
        QStringLiteral("/etc/wpa_supplicant/wpa_supplicant.conf"),
        QStringLiteral("/data/cfg/wpa_supplicant.conf")
    };

    for (const QString &path : candidatePaths) {
        if (QFileInfo(path).exists()) {
            return path;
        }
    }

    return QString();
}

QString WifiWorker::detectInterfaceIpAddress() const
{
    if (m_wifiInterface.isEmpty()) {
        return QString();
    }

    const QNetworkInterface iface = QNetworkInterface::interfaceFromName(m_wifiInterface);
    const QList<QNetworkAddressEntry> entries = iface.addressEntries();
    for (const QNetworkAddressEntry &entry : entries) {
        const QHostAddress address = entry.ip();
        if (address.protocol() == QAbstractSocket::IPv4Protocol
            && !address.isNull()
            && !address.isLoopback()) {
            return address.toString();
        }
    }

    return QString();
}

QVariantMap WifiWorker::configuredNetworksBySsid() const
{
    QString stdoutText;
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("list_networks"),
                        &stdoutText)) {
        return {};
    }

    QVariantMap result;
    const QStringList lines = nonEmptyLines(stdoutText);
    for (int index = 1; index < lines.size(); ++index) {
        const QStringList columns = lines.at(index).split(QLatin1Char('\t'));
        if (columns.size() < 2) {
            continue;
        }

        const QString networkId = columns.at(0).trimmed();
        const QString ssid = columns.at(1).trimmed();
        if (!networkId.isEmpty() && !ssid.isEmpty()) {
            result.insert(ssid, networkId);
        }
    }
    return result;
}

QStringList WifiWorker::findConfiguredNetworkIds(const QString &ssid) const
{
    if (ssid.trimmed().isEmpty()) {
        return {};
    }

    QString stdoutText;
    if (!runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface << QStringLiteral("list_networks"),
                        &stdoutText)) {
        return {};
    }

    QStringList networkIds;
    const QStringList lines = nonEmptyLines(stdoutText);
    for (int index = 1; index < lines.size(); ++index) {
        const QStringList columns = lines.at(index).split(QLatin1Char('\t'));
        if (columns.size() < 2) {
            continue;
        }
        if (columns.at(1).trimmed() == ssid.trimmed()) {
            networkIds.append(columns.at(0).trimmed());
        }
    }
    return networkIds;
}

bool WifiWorker::removeNetwork(const QString &networkId) const
{
    if (networkId.trimmed().isEmpty()) {
        return false;
    }

    QString stdoutText;
    return runWifiCommand(QStringList() << QStringLiteral("-i") << m_wifiInterface
                          << QStringLiteral("remove_network") << networkId.trimmed(),
                          &stdoutText);
}

void WifiWorker::requestDhcpLease()
{
    if (m_wifiInterface.isEmpty() || m_isRequestingDhcp) {
        return;
    }

    const QString udhcpc = QStandardPaths::findExecutable(QStringLiteral("udhcpc"));
    if (udhcpc.isEmpty()) {
        return;
    }

    m_isRequestingDhcp = true;
    QProcess *process = new QProcess(this);
    // -n: 立即失败不等待；-q: 获取到 IP 后退出。
    process->start(udhcpc, QStringList() << QStringLiteral("-n") << QStringLiteral("-q")
                                         << QStringLiteral("-i") << m_wifiInterface);
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        m_isRequestingDhcp = false;
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            refreshState();
        } else {
            m_dhcpRetryCount++;
        }
        process->deleteLater();
    });
}

bool WifiWorker::startWifiSupplicant(const QString &ctrlPath)
{
    if (m_wifiInterface.isEmpty()) {
        return false;
    }

    const QString wpaSupplicantExecutable = QStandardPaths::findExecutable(QStringLiteral("wpa_supplicant"));
    const QString wifiConfigPath = detectWifiConfigPath();
    if (wpaSupplicantExecutable.isEmpty() || wifiConfigPath.isEmpty()) {
        return false;
    }

    // 关键：在 Luckfox 上必须确保目录存在。
    QDir().mkpath(ctrlPath);

    QProcess process;
    process.start(wpaSupplicantExecutable,
                  QStringList()
                  << QStringLiteral("-B")
                  << QStringLiteral("-i") << m_wifiInterface
                  << QStringLiteral("-c") << wifiConfigPath
                  << QStringLiteral("-C") << ctrlPath);
    if (!process.waitForFinished(5000)) {
        return false;
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}
bool WifiWorker::runWifiCommand(const QStringList &arguments, QString *stdoutText, QString *stderrText) const
{
    CommandResult result = runWifiCommandSync(arguments, 3000);
    if (stdoutText) {
        *stdoutText = result.stdoutText;
    }
    if (stderrText) {
        *stderrText = result.stderrText;
    }

    // wpa_cli 在命令失败时通常返回 0 但输出 "FAIL"。
    if (!result.success || result.stdoutText.trimmed() == QStringLiteral("FAIL")) {
        return false;
    }

    return true;
}

WifiWorker::CommandResult WifiWorker::runWifiCommandSync(const QStringList &arguments, int timeoutMs) const
{
    QProcess process;
    QStringList effectiveArguments;
    if (!m_wifiCtrlPath.isEmpty()) {
        effectiveArguments << QStringLiteral("-p") << m_wifiCtrlPath;
    }
    effectiveArguments << arguments;

    process.start(QStringLiteral("wpa_cli"), effectiveArguments);
    if (!process.waitForStarted(1000)) {
        return { false, QString(), process.errorString() };
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.terminate();
        if (!process.waitForFinished(1000)) {
            process.kill();
        }
        return { false, QString(), QStringLiteral("timeout") };
    }

    return {
        process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0,
        QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed(),
        QString::fromLocal8Bit(process.readAllStandardError()).trimmed()
    };
}

WifiModule::WifiModule(GlobalState *globalState, QObject *parent)
    : IModule(globalState, parent)
    , m_worker(new WifiWorker)
{
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &WifiWorker::availableChanged, globalState, &GlobalState::setWifiAvailable);
    connect(m_worker, &WifiWorker::scanningChanged, globalState, &GlobalState::setWifiScanning);
    connect(m_worker, &WifiWorker::interfaceChanged, globalState, &GlobalState::setWifiInterface);
    connect(m_worker, &WifiWorker::statusChanged, globalState, &GlobalState::setWifiStatus);
    connect(m_worker, &WifiWorker::networksChanged, globalState, &GlobalState::setWifiNetworks);
    m_workerThread.start();
}

WifiModule::~WifiModule()
{
    stop();
    m_workerThread.quit();
    m_workerThread.wait();
}

QString WifiModule::name() const
{
    return QStringLiteral("wifi");
}

void WifiModule::applyConfig(const AppConfig &config)
{
    m_config = config.wifi;
}

void WifiModule::start()
{
    QMetaObject::invokeMethod(m_worker, "initialize", Qt::QueuedConnection,
                              Q_ARG(WifiConfig, m_config));
}

void WifiModule::stop()
{
    QMetaObject::invokeMethod(m_worker, "shutdown", Qt::QueuedConnection);
}

void WifiModule::scanNetworks()
{
    QMetaObject::invokeMethod(m_worker, "scanNetworks", Qt::QueuedConnection);
}

void WifiModule::connectToNetwork(const QString &ssid, const QString &password)
{
    QMetaObject::invokeMethod(m_worker, "connectToNetwork", Qt::QueuedConnection,
                              Q_ARG(QString, ssid),
                              Q_ARG(QString, password));
}

void WifiModule::forgetNetwork(const QString &ssid)
{
    QMetaObject::invokeMethod(m_worker, "forgetNetwork", Qt::QueuedConnection,
                              Q_ARG(QString, ssid));
}
