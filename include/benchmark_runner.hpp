#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

struct BenchmarkMetrics {
    // OS Timer
    double timerResolutionMs = 15.625;
    double sleepAccuracy1MsAvg = 0.0;
    double sleepAccuracy1MsMax = 0.0;

    // Memory
    uint64_t totalRamMb = 0;
    uint64_t availableRamMb = 0;
    uint32_t memoryLoadPercent = 0;

    // DNS Resolution
    double dnsResolutionMs = 0.0;

    // Ping & Jitter
    double pingMinMs = 0.0;
    double pingAvgMs = 0.0;
    double pingMaxMs = 0.0;
    double jitterMs = 0.0;
    double packetLossRate = 0.0;

    // Assembly performance
    double scalarChecksumThroughputMBs = 0.0;
    double asmChecksumThroughputMBs = 0.0;
    double asmSpeedupFactor = 0.0;
    double crc32ThroughputMBs = 0.0;
    double memzeroThroughputGBs = 0.0;
    double simdStatsSpeedup = 0.0;
    uint64_t qpcCycles = 0;
    uint64_t rdtscCycles = 0;

    // Registry state
    bool nagleDisabled = false;
    bool ackFrequencyOptimized = false;
    bool networkThrottlingDisabled = false;
    bool systemResponsivenessZero = false;
    bool mmcssBoosted = false;
};

class BenchmarkRunner {
public:
    BenchmarkRunner() = default;

    // Measure system metrics in current state
    BenchmarkMetrics MeasureMetrics(const std::string& pingTarget = "192.168.31.1");

    // Run full Before vs After comparison test
    void RunComparisonBenchmark(const std::string& pingTarget = "192.168.31.1");

    // Print side-by-side comparison table
    static void PrintComparisonTable(const BenchmarkMetrics& before, const BenchmarkMetrics& after);

private:
    double BenchmarkSleepPrecision(double& maxSleep, int iterations = 15);
    double BenchmarkDnsResolution(const std::string& domain = "valve.net");
    void BenchmarkAssemblyRoutines(
        double& scalarMBs, 
        double& asmMBs, 
        double& speedup,
        double& crcMBs,
        double& memzeroGBs,
        double& statsSpeedup,
        uint64_t& qpcCyc, 
        uint64_t& rdtscCyc
    );
};

} // namespace disping
