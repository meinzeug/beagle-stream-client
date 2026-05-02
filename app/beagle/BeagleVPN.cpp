#include "BeagleVPN.h"

#include "BeagleBroker.h"

#include <QDebug>
#include <QDir>
#include <QProcess>

namespace Beagle {
namespace {

constexpr auto kWireGuardInterfacePath = "/sys/class/net/wg-beagle";
constexpr auto kWireGuardTool = "wg";
constexpr auto kWireGuardInterface = "wg-beagle";

bool runWireGuardCommand(const QStringList& arguments)
{
    QProcess process;
    process.start(QString::fromUtf8(kWireGuardTool), arguments);
    if (!process.waitForStarted() || !process.waitForFinished(5000) || process.exitCode() != 0) {
        qWarning() << "Beagle VPN command failed:" << arguments
                   << process.readAllStandardError();
        return false;
    }
    return true;
}

}

bool BeagleVPN::activatePeer(const WgPeer& peer)
{
    if (!peer.valid) {
        return true;
    }

    return runWireGuardCommand({
        "set",
        QString::fromUtf8(kWireGuardInterface),
        "peer",
        peer.public_key,
        "endpoint",
        peer.endpoint,
        "allowed-ips",
        peer.allowed_ips
    });
}

void BeagleVPN::deactivatePeer(const QString& publicKey)
{
    if (publicKey.isEmpty()) {
        return;
    }

    runWireGuardCommand({
        "set",
        QString::fromUtf8(kWireGuardInterface),
        "peer",
        publicKey,
        "remove"
    });
}

bool BeagleVPN::isActive()
{
    return QDir(QString::fromUtf8(kWireGuardInterfacePath)).exists();
}

}
