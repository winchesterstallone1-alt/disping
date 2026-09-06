#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace disping {

class PingMonitor {
public:
    PingMonitor() = default;

    // Single probe ping in milliseconds (sub-millisecond / microsecond precision)
    double PingSingle(const std::string& targetHost, uint32_t timeoutMs = 1000);

    // Run continuous monitoring session
    PingStats RunContinuousMonitor(
        const std::string& targetHost, 
        int count = 20, 
        int intervalMs = 100, 
        std::function<void(const PingStats&, double currentPing)> onPingCallback = nullptr
    );

    // Multi-target benchmark
    std::vector<PingStats> BenchmarkMultipleTargets(
        const std::vector<std::string>& targets,
        int pingsPerTarget = 5
    );

    // Helper: generate ASCII sparkline string from history
    static std::string GenerateSparkline(const std::vector<double>& history, int width = 30);

    // Stop ongoing monitoring
    void Stop() { m_stopRequested.store(true); }

private:
    std::atomic<bool> m_stopRequested{false};
};

} // namespace disping
