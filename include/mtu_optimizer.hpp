#pragma once

#include "disping_types.hpp"
#include <string>

namespace disping {

class MtuOptimizer {
public:
    MtuOptimizer() = default;

    // Discover maximum non-fragmented MTU to a target host
    MtuResult DiscoverOptimalMtu(const std::string& targetHost = "1.1.1.1");

    // Apply discovered MTU to the primary network interface
    OperationResult ApplyMtuToInterface(const std::string& interfaceName, uint32_t mtu);

    // Apply optimal MTU automatically to all active physical interfaces
    OperationResult AutoDetectAndApplyMtu(const std::string& targetHost = "1.1.1.1");

private:
    bool ProbePacketSize(const std::string& targetHost, uint16_t payloadSize);
};

} // namespace disping
