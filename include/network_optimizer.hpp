#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

class NetworkOptimizer {
public:
    NetworkOptimizer() = default;

    // Apply all core network TCP/IP optimizations
    OperationResult ApplyAllNetworkTweaks();

    // Specific sub-tweaks
    OperationResult OptimizeTcpNoDelayAndAckFrequency();
    OperationResult OptimizeNetworkThrottlingAndResponsiveness();
    OperationResult OptimizeTcpGlobalNetsh();
    OperationResult OptimizeTcpParameters();

    // Inspection
    bool AreTweaksApplied() const;
    std::vector<std::string> GetInterfaceGuids() const;

private:
    std::string RunCommand(const std::string& cmd) const;
};

} // namespace disping
