#pragma once

#include <QString>

namespace Beagle {

struct WgPeer;

class BeagleVPN
{
public:
    static bool activatePeer(const WgPeer& peer);
    static void deactivatePeer(const QString& publicKey);
    static bool isActive();
};

}
