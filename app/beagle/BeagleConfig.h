#pragma once

#include <QJsonObject>
#include <QString>

namespace Beagle {

struct EnrollmentConfig {
    QString control_plane;
    QString device_id;
    QString pool_id;
    QString enrollment_token;
    bool valid = false;
};

EnrollmentConfig loadEnrollmentConfig();
bool isManagedMode();
void logStreamEvent(const QString& event, QJsonObject fields = QJsonObject());

}
