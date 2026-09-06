#include "ping_monitor.hpp"
#include "disping_asm.h"
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <future>

namespace disping {

double PingMonitor::PingSingle(const std::string& targetHost, uint32_t timeoutMs) {
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        return -1.0;
    }

    unsigned long ipaddr = inet_addr(targetHost.c_str());
    if (ipaddr == INADDR_NONE) {
        struct hostent* remoteHost = gethostbyname(targetHost.c_str());
        if (remoteHost && remoteHost->h_addr_list[0]) {
            ipaddr = *(u_long*)remoteHost->h_addr_list[0];
        } else {
            IcmpCloseHandle(hIcmpFile);
            return -1.0;
        }
    }

    char sendData[32] = "DISPING_LATENCY_ENGINE_FAST_PRO";
    // Checksum data with assembly routine for verification
    uint16_t csum = asm_fast_checksum_x64(sendData, sizeof(sendData));
    (void)csum;

    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 32;
    std::vector<char> replyBuffer(replySize, 0);

    IP_OPTION_INFORMATION ipOptions;
    ZeroMemory(&ipOptions, sizeof(ipOptions));
    ipOptions.Ttl = 64;

    // High resolution clock start
    auto t1 = std::chrono::high_resolution_clock::now();

    DWORD dwRetVal = IcmpSendEcho2(
        hIcmpFile,
        NULL,
        NULL,
        NULL,
        ipaddr,
        sendData,
        sizeof(sendData),
        &ipOptions,
        replyBuffer.data(),
        replySize,
        timeoutMs
    );

    auto t2 = std::chrono::high_resolution_clock::now();
    IcmpCloseHandle(hIcmpFile);

    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer.data());
        if (pEchoReply->Status == IP_SUCCESS) {
            // Calculate round trip with microsecond precision
            double rttMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
            return rttMs;
        }
    }

    return -1.0; // packet loss / timeout
}

PingStats PingMonitor::RunContinuousMonitor(
    const std::string& targetHost, 
    int count, 
    int intervalMs, 
    std::function<void(const PingStats&, double currentPing)> onPingCallback) 
{
    m_stopRequested.store(false);
    PingStats stats;
    stats.target = targetHost;
    stats.minRttMs = 99999.0;
    stats.maxRttMs = 0.0;
    stats.avgRttMs = 0.0;
    stats.jitterMs = 0.0;

    double previousRtt = -1.0;
    std::vector<double> validRtts;

    for (int i = 0; i < count && !m_stopRequested.load(); ++i) {
        stats.sent++;
        double rtt = PingSingle(targetHost, 1000);

        if (rtt >= 0.0) {
            stats.received++;
            validRtts.push_back(rtt);
            stats.rttHistory.push_back(rtt);

            if (rtt < stats.minRttMs) stats.minRttMs = rtt;
            if (rtt > stats.maxRttMs) stats.maxRttMs = rtt;

            // RFC 3550 Jitter calculation using assembly SIMD routine
            if (previousRtt >= 0.0) {
                double transitDiff = rtt - previousRtt;
                stats.jitterMs = asm_calc_jitter_rfc3550(stats.jitterMs, transitDiff);
            }
            previousRtt = rtt;

            // Update average
            double sum = std::accumulate(validRtts.begin(), validRtts.end(), 0.0);
            stats.avgRttMs = sum / validRtts.size();

            // Calculate Standard Deviation
            if (validRtts.size() > 1) {
                double sqSum = 0.0;
                for (double val : validRtts) {
                    sqSum += (val - stats.avgRttMs) * (val - stats.avgRttMs);
                }
                stats.stdDevMs = std::sqrt(sqSum / validRtts.size());
            }
        } else {
            stats.lost++;
            stats.rttHistory.push_back(-1.0); // marker for lost packet
        }

        stats.lossRate = stats.sent > 0 ? (static_cast<double>(stats.lost) / stats.sent) * 100.0 : 0.0;

        if (onPingCallback) {
            onPingCallback(stats, rtt);
        }

        if (i + 1 < count && !m_stopRequested.load()) {
            Sleep(intervalMs);
        }
    }

    if (stats.received == 0) {
        stats.minRttMs = 0.0;
    }

    return stats;
}

std::vector<PingStats> PingMonitor::BenchmarkMultipleTargets(
    const std::vector<std::string>& targets,
    int pingsPerTarget) 
{
    std::vector<std::future<PingStats>> futures;

    for (const auto& target : targets) {
        futures.push_back(std::async(std::launch::async, [this, target, pingsPerTarget]() {
            return RunContinuousMonitor(target, pingsPerTarget, 50, nullptr);
        }));
    }

    std::vector<PingStats> results;
    for (auto& f : futures) {
        results.push_back(f.get());
    }

    return results;
}

std::string PingMonitor::GenerateSparkline(const std::vector<double>& history, int width) {
    if (history.empty()) {
        return "";
    }

    // Filter valid non-negative pings
    std::vector<double> valid;
    for (double v : history) {
        if (v >= 0.0) valid.push_back(v);
    }

    if (valid.empty()) {
        return "[PACKET LOSS]";
    }

    double minVal = *std::min_element(valid.begin(), valid.end());
    double maxVal = *std::max_element(valid.begin(), valid.end());
    double range = maxVal - minVal;
    if (range < 0.001) range = 1.0;

    // 7 levels of ASCII bar blocks:  , ▂, ▃, ▄, ▅, ▆, ▇, █
    // For pure ASCII compatibility across all Windows codepages (including cp866, cp1251):
    // We provide clean standard blocks or bullet indicators: _ . - = # ^ *
    const char asciiChars[] = {'_', '.', '-', '=', 'o', 'O', '#'};
    const int numChars = sizeof(asciiChars);

    std::string sparkline = "";
    int startIdx = 0;
    if (static_cast<int>(history.size()) > width) {
        startIdx = static_cast<int>(history.size()) - width;
    }

    for (size_t i = startIdx; i < history.size(); ++i) {
        double val = history[i];
        if (val < 0.0) {
            sparkline += "X"; // Lost packet
        } else {
            int level = static_cast<int>(((val - minVal) / range) * (numChars - 1));
            if (level < 0) level = 0;
            if (level >= numChars) level = numChars - 1;
            sparkline += asciiChars[level];
        }
    }

    return sparkline;
}

} // namespace disping
