#pragma once

#include "BeagleBroker.h"
#include "backend/nvapp.h"
#include "backend/nvcomputer.h"

#include <QString>

namespace Beagle {

struct BootstrapResult {
    bool success = false;
    QString error;
    WgPeer wg_peer;
    bool vpn_activated = false;
    NvApp app;
    bool app_found = false;
};

class BeagleBootstrap
{
public:
    static bool isEnabled();
    static BootstrapResult prepareComputer(NvComputer& computer, const QString& appName = QString());
    static void cleanup(const BootstrapResult& result);
};

}
