#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace disping {

// Optimization flags
enum class OptimizationModule : uint32_t {
    None               = 0,
    TcpStack           = 1 << 0,
    NetworkThrottling  = 1 << 1,
    AdapterTuning      = 1 << 2,
    QoSPriority        = 1 << 3,
    TimerResolution    = 1 << 4,
    MMCSSProfile       = 1 << 5,
    ProcessAffinity    = 1 << 6,
    MemoryClean        = 1 << 7,
    DnsOptimization    = 1 << 8,
    All                = 0xFFFFFFFF
};

inline OptimizationModule operator|(OptimizationModule a, OptimizationModule b) {
    return static_cast<OptimizationModule>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool operator&(OptimizationModule a, OptimizationModule b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

// Result status for operations
struct OperationResult {
    bool success = false;
    std::string message;
    std::string details;
    uint32_t errorCode = 0;
};

// Ping statistics
struct PingStats {
    std::string target;
    std::string ipAddress;
    uint32_t sent = 0;
    uint32_t received = 0;
    uint32_t lost = 0;
    double lossRate = 0.0;     // in %
    double minRttMs = 0.0;
    double maxRttMs = 0.0;
    double avgRttMs = 0.0;
    double jitterMs = 0.0;     // RFC 3550 jitter
    double stdDevMs = 0.0;
    std::vector<double> rttHistory;
};

// Network Adapter Info
struct NetworkAdapterInfo {
    std::string id;            // GUID or Interface name
    std::string name;          // e.g. "Ethernet"
    std::string description;   // e.g. "Intel(R) Ethernet Controller I225-V"
    std::string macAddress;
    std::string ipv4Address;
    uint32_t mtu = 1500;
    uint32_t speedMbps = 1000;
    bool isPhysical = true;
    bool isConnected = true;
};

// MTU probe result
struct MtuResult {
    uint32_t optimalMtu = 1500;
    uint32_t optimalMss = 1460;
    bool found = false;
    std::string notes;
};

// DNS Benchmark item
struct DnsBenchmarkResult {
    std::string name;          // e.g. "Cloudflare"
    std::string primaryIp;
    std::string secondaryIp;
    double avgLatencyMs = 0.0;
    bool reachable = false;
};

// System latency info
struct LatencyProfile {
    double timerResolutionMs = 15.625; // Default Windows timer
    double requestedResolutionMs = 0.5;
    bool mmcssEnabled = false;
    std::string mmcssTask = "Games";
    bool responsivenessMaximized = false;
};

} // namespace disping
