#pragma once

#include "BeagleConfig.h"

#include <QNetworkAccessManager>
#include <QObject>

namespace Beagle {

struct WgPeer {
    QString public_key;
    QString endpoint;
    QString allowed_ips;
    bool valid = false;
};

struct AllocateResult {
    bool success = false;
    QString error;
    QString host_ip;
    int port = 47984;
    QString token;
    WgPeer wg_peer;
};

class BeagleBroker : public QObject
{
    Q_OBJECT

public:
    explicit BeagleBroker(QObject* parent = nullptr);

    void allocate(const QString& poolId = QString());

signals:
    void allocated(Beagle::AllocateResult result);

private:
    EnrollmentConfig m_cfg;
    QNetworkAccessManager m_nam;
};

}

Q_DECLARE_METATYPE(Beagle::AllocateResult)
