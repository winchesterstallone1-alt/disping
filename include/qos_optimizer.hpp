#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

class QoSOptimizer {
public:
    QoSOptimizer() = default;

    // Enable DSCP/TOS packet tagging in Windows TCP/IP stack
    OperationResult EnableUserTOS();

    // Configure Policy-Based QoS DSCP 46 (Expedited Forwarding) for gaming
    OperationResult SetupGamingQoSPolicies(const std::vector<std::string>& customGameExecutables = {});

    // Remove QoS Policies
    OperationResult RemoveGamingQoSPolicies();

    // Check if TOS/QoS is enabled
    bool IsQoSEnabled() const;

private:
    std::vector<std::string> GetDefaultGamingExecutables() const;
};

} // namespace disping
