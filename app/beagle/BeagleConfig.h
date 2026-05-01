#pragma once

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

}
