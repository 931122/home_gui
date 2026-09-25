#include "networkutils.h"

#include <QHostAddress>
#include <QNetworkInterface>

namespace {

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
bool isIgnoredNetworkInterface(const QString &name)
{
    static const QStringList prefixes = {
        QStringLiteral("lo"),
        QStringLiteral("docker"),
        QStringLiteral("br-"),
        QStringLiteral("veth"),
        QStringLiteral("utun"),
        QStringLiteral("tun"),
        QStringLiteral("tap"),
        QStringLiteral("wg"),
        QStringLiteral("tailscale"),
        QStringLiteral("zt")
    };

    const QString lowerName = name.trimmed().toLower();
    for (const QString &prefix : prefixes) {
        if (lowerName.startsWith(prefix)) {
            return true;
        }
    }
    return false;
}

bool isRoutableAddress(const QHostAddress &address)
{
    if (address.isNull()
        || address.isLoopback()
        || address.isMulticast()) {
        return false;
    }

    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        const quint32 ipv4 = address.toIPv4Address();
        if (ipv4 == 0) {
            return false;
        }
        return (ipv4 & 0xFFFF0000U) != 0xA9FE0000U;
    }

    if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        return !address.isLinkLocal()
                && !address.isSiteLocal()
                && !address.isUniqueLocalUnicast();
    }

    return false;
}
#endif

} // namespace

bool detectNetworkOnline()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // 在移动平台（Android/iOS）上，受沙盒权限与隐私策略限制，直接枚举网卡往往无法获取完整状态，
    // 默认保持在线状态，由具体网络请求及底层 TCP/SSL 自行处理连接与错误，避免全局误判为断网。
    return true;
#else
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const QNetworkInterface::InterfaceFlags flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp)
            || !flags.testFlag(QNetworkInterface::IsRunning)
            || flags.testFlag(QNetworkInterface::IsLoopBack)
            || isIgnoredNetworkInterface(iface.name())) {
            continue;
        }

        const QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (isRoutableAddress(entry.ip())) {
                return true;
            }
        }
    }
    return false;
#endif
}
