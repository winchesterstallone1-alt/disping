#pragma once

#include "disping_types.hpp"
#include <string>
#include <atomic>
#include <thread>
#include <cstdint>

namespace disping {

struct UdpProxyStats {
    uint64_t packetsForwarded = 0;
    uint64_t bytesForwarded = 0;
    double avgForwardLatencyUs = 0.0;
};

class UdpProxy {
public:
    UdpProxy() = default;
    ~UdpProxy();

    // Start local zero-loss high-priority UDP proxy / relay
    OperationResult Start(
        uint16_t listenPort,
        const std::string& remoteHost,
        uint16_t remotePort
    );

    // Stop proxy
    void Stop();

    bool IsRunning() const { return m_running.load(); }
    UdpProxyStats GetStats() const;

private:
    std::atomic<bool> m_running{false};
    std::thread m_workerThread;
    UdpProxyStats m_stats;
};

} // namespace disping
