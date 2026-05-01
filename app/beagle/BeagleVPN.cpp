#include "BeagleVPN.h"

#include "BeagleBroker.h"

#include <QDir>
#include <QProcess>
#include <QDebug>

namespace Beagle {

bool BeagleVPN::activatePeer(const WgPeer &peer)
{
    if (!peer.valid) {
        return true;
    }

#ifdef Q_OS_LINUX
    QProcess wg;
    wg.start("wg", {"set", "wg-beagle", "peer", peer.public_key, "endpoint", peer.endpoint, "allowed-ips", peer.allowed_ips});
    if (!wg.waitForFinished(5000) || wg.exitStatus() != QProcess::NormalExit || wg.exitCode() != 0) {
        qWarning() << "Beagle WireGuard peer activation failed:" << wg.errorString() << wg.readAllStandardError();
        return false;
    }
    return true;
#else
    qWarning() << "Beagle WireGuard peer activation is only supported on Linux";
    return false;
#endif
}

void BeagleVPN::deactivatePeer(const QString &public_key)
{
    if (public_key.isEmpty()) {
        return;
    }

#ifdef Q_OS_LINUX
    QProcess wg;
    wg.start("wg", {"set", "wg-beagle", "peer", public_key, "remove"});
    if (!wg.waitForFinished(5000) || wg.exitStatus() != QProcess::NormalExit || wg.exitCode() != 0) {
        qWarning() << "Beagle WireGuard peer removal failed:" << wg.errorString() << wg.readAllStandardError();
    }
#endif
}

bool BeagleVPN::isActive()
{
#ifdef Q_OS_LINUX
    return QDir("/sys/class/net/wg-beagle").exists();
#else
    return false;
#endif
}

}
