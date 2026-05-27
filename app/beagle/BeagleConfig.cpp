#include "BeagleConfig.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QTextStream>

namespace Beagle {
namespace {

constexpr auto kEnrollmentConfigPath = "/etc/beagle/enrollment.conf";

QString stripQuotes(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2 &&
        ((value.startsWith('"') && value.endsWith('"')) ||
         (value.startsWith('\'') && value.endsWith('\'')))) {
        value = value.mid(1, value.size() - 2);
    }
    return value.trimmed();
}

}

EnrollmentConfig loadEnrollmentConfig()
{
    EnrollmentConfig cfg;
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.value(QStringLiteral("BEAGLE_STREAM_CLIENT_DISABLE_INTERNAL_BROKER")).trimmed() == QStringLiteral("1")) {
        return cfg;
    }

    QFile file(QString::fromUtf8(kEnrollmentConfigPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return cfg;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        int separator = line.indexOf('=');
        if (separator <= 0) {
            continue;
        }

        const QString key = line.left(separator).trimmed();
        const QString value = stripQuotes(line.mid(separator + 1));

        if (key == "control_plane") {
            cfg.control_plane = value;
        }
        else if (key == "device_id") {
            cfg.device_id = value;
        }
        else if (key == "pool_id") {
            cfg.pool_id = value;
        }
        else if (key == "enrollment_token") {
            cfg.enrollment_token = value;
        }
    }

    cfg.valid = !cfg.control_plane.isEmpty() &&
                !cfg.device_id.isEmpty() &&
                !cfg.pool_id.isEmpty() &&
                !cfg.enrollment_token.isEmpty();
    return cfg;
}

bool isManagedMode()
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.value(QStringLiteral("BEAGLE_STREAM_CLIENT_MANAGED")).trimmed() == QStringLiteral("1")) {
        return true;
    }
    return loadEnrollmentConfig().valid;
}

void logStreamEvent(const QString& event, QJsonObject fields)
{
    if (!isManagedMode()) {
        return;
    }
    fields.insert(QStringLiteral("event"), event);
    fields.insert(QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    const QByteArray payload = QJsonDocument(fields).toJson(QJsonDocument::Compact);
    qInfo().noquote() << QStringLiteral("BEAGLE_STREAM_EVENT") << QString::fromUtf8(payload);
}

}
