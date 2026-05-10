#include "BeagleBootstrap.h"

#include "BeagleConfig.h"
#include "BeagleVPN.h"
#include "backend/nvhttp.h"
#include "backend/nvpairingmanager.h"

#include <QEventLoop>
#include <QObject>
#include <QSettings>
#include <QThread>

namespace Beagle {
namespace {

NvApp findAppByName(const QVector<NvApp>& appList, const QString& appName)
{
    for (const NvApp& app : appList) {
        if (app.name.compare(appName, Qt::CaseInsensitive) == 0) {
            return app;
        }
    }

    return NvApp();
}

void seedComputerFromPersistedHostConfig(NvComputer& computer)
{
    if (!computer.uuid.isEmpty() && !computer.serverCert.isNull()) {
        return;
    }

    QSettings settings;
    const int hosts = settings.beginReadArray("hosts");
    for (int i = 0; i < hosts; ++i) {
        settings.setArrayIndex(i);
        NvComputer persisted(settings);
        if (persisted.uuid.isEmpty() || persisted.serverCert.isNull()) {
            continue;
        }

        if (computer.name.isEmpty()) {
            computer.name = persisted.name;
        }
        computer.uuid = persisted.uuid;
        computer.serverCert = persisted.serverCert;
        if (computer.manualAddress.isNull()) {
            computer.manualAddress = persisted.manualAddress;
        }
        if (computer.activeAddress.isNull()) {
            computer.activeAddress = persisted.activeAddress;
        }
        if (computer.remoteAddress.isNull()) {
            computer.remoteAddress = persisted.remoteAddress;
        }
        if (computer.localAddress.isNull()) {
            computer.localAddress = persisted.localAddress;
        }
        break;
    }
    settings.endArray();
}

}

bool BeagleBootstrap::isEnabled()
{
    const EnrollmentConfig cfg = loadEnrollmentConfig();
    return cfg.valid && !cfg.pool_id.isEmpty();
}

BootstrapResult BeagleBootstrap::prepareComputer(NvComputer& computer, const QString& appName)
{
    BootstrapResult bootstrap;
    const EnrollmentConfig cfg = loadEnrollmentConfig();
    if (!cfg.valid || cfg.pool_id.isEmpty()) {
        bootstrap.success = true;
        return bootstrap;
    }

    BeagleBroker broker;
    QEventLoop loop;
    AllocateResult result;
    QObject::connect(&broker, &BeagleBroker::allocated, &loop, [&](AllocateResult allocated) {
        result = allocated;
        loop.quit();
    });
    broker.allocate(cfg.pool_id);
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (!result.success) {
        bootstrap.error = QObject::tr("Beagle broker allocation failed: %1").arg(result.error);
        return bootstrap;
    }

    seedComputerFromPersistedHostConfig(computer);
    bootstrap.wg_peer = result.wg_peer;
    if (bootstrap.wg_peer.valid) {
        bootstrap.vpn_activated = BeagleVPN::activatePeer(bootstrap.wg_peer);
        if (!bootstrap.vpn_activated) {
            bootstrap.error = QObject::tr("Failed to activate the Beagle VPN session.");
            return bootstrap;
        }
    }

    computer.activeAddress = NvAddress(result.host_ip, result.port);
    computer.activeHttpsPort = 0;
    computer.remoteAddress = computer.activeAddress;
    computer.manualAddress = computer.activeAddress;

    try {
        NvHTTP http(&computer);
        QString serverInfo = http.getServerInfo(NvHTTP::NVLL_ERROR);
        NvComputer updated(http, serverInfo);
        computer.update(updated);

        if (computer.pairState != NvComputer::PS_PAIRED) {
            NvPairingManager pairingManager(&computer);
            NvPairingManager::PairState pairState = NvPairingManager::PairState::FAILED;
            constexpr int kPairAttempts = 5;
            for (int attempt = 1; attempt <= kPairAttempts; ++attempt) {
                pairState = pairingManager.pair(computer.appVersion, result.token, computer.serverCert);
                if (pairState == NvPairingManager::PairState::PAIRED) {
                    break;
                }
                if (pairState == NvPairingManager::PairState::PIN_WRONG) {
                    break;
                }
                if (attempt < kPairAttempts) {
                    QThread::msleep(700);
                }
            }

            if (pairState != NvPairingManager::PairState::PAIRED) {
                bootstrap.error = QObject::tr("Beagle token pairing failed.");
                cleanup(bootstrap);
                return bootstrap;
            }

            NvHTTP pairedHttp(&computer);
            serverInfo = pairedHttp.getServerInfo(NvHTTP::NVLL_ERROR);
            NvComputer paired(pairedHttp, serverInfo);
            computer.update(paired);
        }

        if (!appName.isEmpty()) {
            const QVector<NvApp> appList = NvHTTP(&computer).getAppList();
            bootstrap.app = findAppByName(appList, appName);
            bootstrap.app_found = bootstrap.app.isInitialized();
            if (!bootstrap.app_found) {
                bootstrap.error = QObject::tr("Failed to find application %1").arg(appName);
                cleanup(bootstrap);
                return bootstrap;
            }
        }
    }
    catch (const GfeHttpResponseException& e) {
        bootstrap.error = QObject::tr("Beagle stream host returned error: %1").arg(e.toQString());
        cleanup(bootstrap);
        return bootstrap;
    }
    catch (const QtNetworkReplyException& e) {
        bootstrap.error = e.toQString();
        cleanup(bootstrap);
        return bootstrap;
    }

    bootstrap.success = true;
    return bootstrap;
}

void BeagleBootstrap::cleanup(const BootstrapResult& result)
{
    if (result.vpn_activated && result.wg_peer.valid) {
        BeagleVPN::deactivatePeer(result.wg_peer.public_key);
    }
}

}
