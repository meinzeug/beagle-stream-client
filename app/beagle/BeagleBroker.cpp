#include "BeagleBroker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QUrl>

namespace Beagle {
namespace {

constexpr int kAllocateTimeoutMs = 15000;

AllocateResult makeErrorResult(const QString& error)
{
    AllocateResult result;
    result.error = error;
    return result;
}

QString joinUrlPath(const QString& base, const QString& path)
{
    QString normalized = base;
    while (normalized.endsWith('/')) {
        normalized.chop(1);
    }
    return normalized + path;
}

}

BeagleBroker::BeagleBroker(QObject* parent)
    : QObject(parent),
      m_cfg(loadEnrollmentConfig())
{
}

void BeagleBroker::allocate(const QString& poolId)
{
    if (!m_cfg.valid) {
        emit allocated(makeErrorResult(tr("Beagle enrollment is not configured.")));
        return;
    }

    QUrl url(joinUrlPath(m_cfg.control_plane, "/api/v1/streams/allocate"));
    if (!url.isValid()) {
        emit allocated(makeErrorResult(tr("Invalid Beagle control plane URL.")));
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    request.setTransferTimeout(kAllocateTimeoutMs);
#endif
    request.setRawHeader("X-Beagle-Token", m_cfg.enrollment_token.toUtf8());

    QJsonObject payload{
        {"pool_id", poolId.isEmpty() ? m_cfg.pool_id : poolId},
        {"device_id", m_cfg.device_id},
        {"user_id", QString()}
    };

    QNetworkReply* reply = m_nam.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::sslErrors, this, [reply](const QList<QSslError>& errors) {
        Q_UNUSED(errors);
        reply->ignoreSslErrors();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit allocated(makeErrorResult(reply->errorString()));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit allocated(makeErrorResult(tr("Invalid allocate response from Beagle control plane.")));
            return;
        }

        const QJsonObject object = document.object();
        AllocateResult result;
        result.success = true;
        result.host_ip = object.value("host_ip").toString();
        result.port = object.value("port").toInt(47984);
        result.token = object.value("token").toString();

        const QJsonObject wg = object.value("wg_peer_config").toObject();
        if (!wg.isEmpty()) {
            result.wg_peer.public_key = wg.value("public_key").toString();
            result.wg_peer.endpoint = wg.value("endpoint").toString();
            result.wg_peer.allowed_ips = wg.value("allowed_ips").toString();
            result.wg_peer.valid = !result.wg_peer.public_key.isEmpty() &&
                                   !result.wg_peer.endpoint.isEmpty() &&
                                   !result.wg_peer.allowed_ips.isEmpty();
        }

        if (result.host_ip.isEmpty()) {
            emit allocated(makeErrorResult(tr("Allocate response did not include a host address.")));
            return;
        }

        if (result.token.isEmpty()) {
            emit allocated(makeErrorResult(tr("Allocate response did not include a stream token.")));
            return;
        }

        emit allocated(result);
    });
}

}
