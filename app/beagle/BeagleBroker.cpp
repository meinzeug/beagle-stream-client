#include "BeagleBroker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QUrl>

namespace Beagle {

BeagleBroker::BeagleBroker(QObject *parent)
    : QObject(parent),
      m_Cfg(loadEnrollmentConfig()),
      m_Nam(new QNetworkAccessManager(this))
{
    connect(m_Nam, &QNetworkAccessManager::sslErrors, this, [](QNetworkReply *reply, const QList<QSslError> &errors) {
        reply->ignoreSslErrors(errors);
    });
}

void BeagleBroker::allocate(const QString &pool_id)
{
    AllocateResult result;
    if (!m_Cfg.valid) {
        result.error = tr("Beagle enrollment config is missing or incomplete");
        emit allocated(result);
        return;
    }

    QUrl url(m_Cfg.control_plane);
    url.setPath("/api/v1/streams/allocate");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-Beagle-Token", m_Cfg.enrollment_token.toUtf8());

    QJsonObject body;
    body["pool_id"] = pool_id.isEmpty() ? m_Cfg.pool_id : pool_id;
    body["device_id"] = m_Cfg.device_id;
    body["user_id"] = "";

    QNetworkReply *reply = m_Nam->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        AllocateResult result;
        const QByteArray payload = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            result.error = reply->errorString();
            reply->deleteLater();
            emit allocated(result);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            result.error = tr("Invalid Beagle broker response");
            reply->deleteLater();
            emit allocated(result);
            return;
        }

        const QJsonObject obj = doc.object();
        result.host_ip = obj.value("host_ip").toString();
        result.port = obj.value("port").toInt(result.port);
        result.token = obj.value("token").toString();

        const QJsonObject wg = obj.value("wg_peer_config").toObject();
        result.wg_peer.public_key = wg.value("public_key").toString();
        result.wg_peer.endpoint = wg.value("endpoint").toString();
        result.wg_peer.allowed_ips = wg.value("allowed_ips").toString();
        result.wg_peer.valid = !result.wg_peer.public_key.isEmpty() &&
                               !result.wg_peer.endpoint.isEmpty() &&
                               !result.wg_peer.allowed_ips.isEmpty();

        result.success = !result.host_ip.isEmpty() && !result.token.isEmpty();
        if (!result.success) {
            result.error = tr("Beagle broker response is missing host or token");
        }

        reply->deleteLater();
        emit allocated(result);
    });
}

}
