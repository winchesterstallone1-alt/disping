#include "dns_optimizer.hpp"
#include "adapter_optimizer.hpp"
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace disping {

double DnsOptimizer::MeasureDnsLatency(const std::string& dnsServerIp, const std::string& testDomain) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        return 9999.0;
    }

    // Set timeout to 1200ms
    DWORD timeout = 1200;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(53); // DNS port
    inet_pton(AF_INET, dnsServerIp.c_str(), &serverAddr.sin_addr);

    // Build DNS wire packet
    std::vector<uint8_t> packet;
    // Header
    packet.push_back(0x12); packet.push_back(0x34); // Transaction ID
    packet.push_back(0x01); packet.push_back(0x00); // Standard query, recursion desired
    packet.push_back(0x00); packet.push_back(0x01); // 1 Question
    packet.push_back(0x00); packet.push_back(0x00); // 0 Answer RRs
    packet.push_back(0x00); packet.push_back(0x00); // 0 Authority RRs
    packet.push_back(0x00); packet.push_back(0x00); // 0 Additional RRs

    // Encode QNAME: valve.net -> 5 valve 3 net 0
    std::string domain = testDomain;
    size_t start = 0;
    size_t dot = domain.find('.');
    while (dot != std::string::npos) {
        std::string part = domain.substr(start, dot - start);
        packet.push_back(static_cast<uint8_t>(part.length()));
        for (char c : part) packet.push_back(c);
        start = dot + 1;
        dot = domain.find('.', start);
    }
    std::string last = domain.substr(start);
    packet.push_back(static_cast<uint8_t>(last.length()));
    for (char c : last) packet.push_back(c);
    packet.push_back(0x00); // Root label

    // QTYPE: A (0x0001)
    packet.push_back(0x00); packet.push_back(0x01);
    // QCLASS: IN (0x0001)
    packet.push_back(0x00); packet.push_back(0x01);

    auto t1 = std::chrono::high_resolution_clock::now();
    int sent = sendto(s, (const char*)packet.data(), (int)packet.size(), 0, 
                      (sockaddr*)&serverAddr, sizeof(serverAddr));
    
    if (sent <= 0) {
        closesocket(s);
        return 9999.0;
    }

    uint8_t recvBuf[512];
    sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);
    int recvd = recvfrom(s, (char*)recvBuf, sizeof(recvBuf), 0, (sockaddr*)&fromAddr, &fromLen);
    auto t2 = std::chrono::high_resolution_clock::now();

    closesocket(s);

    if (recvd <= 0) {
        return 9999.0; // timeout or error
    }

    double latencyMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    return latencyMs;
}

std::vector<DnsBenchmarkResult> DnsOptimizer::BenchmarkDnsProviders(int iterations) {
    struct DnsEntry {
        std::string name;
        std::string primary;
        std::string secondary;
    };

    std::vector<DnsEntry> providers = {
        {"Cloudflare Gaming", "1.1.1.1", "1.0.0.1"},
        {"Google Public DNS", "8.8.8.8", "8.8.4.4"},
        {"Quad9 Low-Latency", "9.9.9.9", "149.112.112.112"},
        {"OpenDNS Fast",      "208.67.222.222", "208.67.220.220"},
        {"AdGuard Gaming",    "94.140.14.14", "94.140.15.15"}
    };

    std::vector<DnsBenchmarkResult> results;

    for (const auto& prov : providers) {
        DnsBenchmarkResult item;
        item.name = prov.name;
        item.primaryIp = prov.primary;
        item.secondaryIp = prov.secondary;

        double sum = 0.0;
        int validCount = 0;

        for (int i = 0; i < iterations; ++i) {
            double lat = MeasureDnsLatency(prov.primary);
            if (lat < 9000.0) {
                sum += lat;
                validCount++;
            }
            Sleep(20);
        }

        if (validCount > 0) {
            item.avgLatencyMs = sum / validCount;
            item.reachable = true;
        } else {
            item.avgLatencyMs = 999.0;
            item.reachable = false;
        }

        results.push_back(item);
    }

    std::sort(results.begin(), results.end(), [](const DnsBenchmarkResult& a, const DnsBenchmarkResult& b) {
        return a.avgLatencyMs < b.avgLatencyMs;
    });

    return results;
}

OperationResult DnsOptimizer::SetDnsServers(const std::string& interfaceName, const std::string& primary, const std::string& secondary) {
    OperationResult res;
    
    std::stringstream ss1;
    ss1 << "cmd.exe /c \"netsh interface ipv4 set dnsservers name=\\\"" << interfaceName << "\\\" static " << primary << " primary\" > nul 2>&1";
    int ret1 = system(ss1.str().c_str());

    std::stringstream ss2;
    ss2 << "cmd.exe /c \"netsh interface ipv4 add dnsservers name=\\\"" << interfaceName << "\\\" " << secondary << " index=2\" > nul 2>&1";
    int ret2 = system(ss2.str().c_str());
    (void)ret2;

    FlushDnsCache();

    if (ret1 == 0) {
        res.success = true;
        res.message = "Configured DNS: Primary=" + primary + ", Secondary=" + secondary + " on [" + interfaceName + "].";
    } else {
        res.success = false;
        res.message = "Failed to set DNS servers (ensure running as Administrator).";
    }
    return res;
}

OperationResult DnsOptimizer::FlushDnsCache() {
    OperationResult res;
    int ret = system("cmd.exe /c \"ipconfig /flushdns\" > nul 2>&1");
    res.success = (ret == 0);
    res.message = res.success ? "Flushed Windows DNS resolver cache." : "Failed to flush DNS cache.";
    return res;
}

OperationResult DnsOptimizer::AutoSelectFastestDns() {
    auto results = BenchmarkDnsProviders(3);
    if (results.empty() || !results[0].reachable) {
        OperationResult r;
        r.success = false;
        r.message = "No reachable DNS providers found.";
        return r;
    }

    const auto& best = results[0];

    AdapterOptimizer adapterOpt;
    auto adapters = adapterOpt.GetActiveAdapters();
    if (adapters.empty()) {
        OperationResult r;
        r.success = false;
        r.message = "No active network adapters found to configure DNS.";
        return r;
    }

    int applied = 0;
    for (const auto& nic : adapters) {
        if (nic.isPhysical) {
            auto r = SetDnsServers(nic.name, best.primaryIp, best.secondaryIp);
            if (r.success) applied++;
        }
    }

    OperationResult overall;
    overall.success = (applied > 0);
    std::stringstream ss;
    ss << "Fastest DNS selected: " << best.name << " (" << best.primaryIp << ") with avg latency "
       << std::fixed << std::setprecision(2) << best.avgLatencyMs << " ms!";
    overall.message = ss.str();
    return overall;
}

} // namespace disping
