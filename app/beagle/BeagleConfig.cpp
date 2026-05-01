#include "BeagleConfig.h"

#include <QFile>
#include <QTextStream>

namespace Beagle {
namespace {

constexpr auto kEnrollmentPath = "/etc/beagle/enrollment.conf";

QString stripQuotes(QString value)
{
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
        return value.mid(1, value.size() - 2);
    }
    return value;
}

}

EnrollmentConfig loadEnrollmentConfig()
{
    EnrollmentConfig cfg;
    QFile file(kEnrollmentPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return cfg;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const int pos = line.indexOf('=');
        if (pos < 0) {
            continue;
        }

        const QString key = line.left(pos).trimmed();
        const QString value = stripQuotes(line.mid(pos + 1).trimmed());
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
                !cfg.enrollment_token.isEmpty();
    return cfg;
}

}
