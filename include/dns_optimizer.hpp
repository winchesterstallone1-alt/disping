#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

class DnsOptimizer {
public:
    DnsOptimizer() = default;

    // Run benchmark across known high-speed gaming DNS providers
    std::vector<DnsBenchmarkResult> BenchmarkDnsProviders(int iterations = 3);

    // Apply the fastest DNS automatically
    OperationResult AutoSelectFastestDns();

    // Set custom DNS on an interface
    OperationResult SetDnsServers(const std::string& interfaceName, const std::string& primary, const std::string& secondary);

    // Flush Windows DNS resolver cache
    OperationResult FlushDnsCache();

private:
    double MeasureDnsLatency(const std::string& dnsServerIp, const std::string& testDomain = "valve.net");
};

} // namespace disping
