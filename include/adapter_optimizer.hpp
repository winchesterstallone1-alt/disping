#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

class AdapterOptimizer {
public:
    AdapterOptimizer() = default;

    // Get list of active physical adapters
    std::vector<NetworkAdapterInfo> GetActiveAdapters();

    // Optimize NIC hardware properties in registry
    OperationResult OptimizeAllNetworkAdapters();

    // Specific optimizations
    OperationResult DisableInterruptModeration();
    OperationResult DisableLargeSendOffload();
    OperationResult DisableEnergySaving();
    OperationResult MaximizeAdapterBuffers();
    OperationResult DisableFlowControl();

    // Wi-Fi Zero-Jitter Anti-Spike Engine
    OperationResult OptimizeWifiAdapters();
    OperationResult SetWifiBackgroundScan(bool enabled);

private:
    std::vector<std::string> GetAdapterClassKeys() const;
    bool IsWifiAdapter(const std::string& classKey) const;
};

} // namespace disping
