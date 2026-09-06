#include "mtu_optimizer.hpp"
#include "adapter_optimizer.hpp"
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <sstream>

#ifndef IP_FLAG_DF
#define IP_FLAG_DF 0x02
#endif

namespace disping {

bool MtuOptimizer::ProbePacketSize(const std::string& targetHost, uint16_t payloadSize) {
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    unsigned long ipaddr = inet_addr(targetHost.c_str());
    if (ipaddr == INADDR_NONE) {
        struct hostent* remoteHost = gethostbyname(targetHost.c_str());
        if (remoteHost && remoteHost->h_addr_list[0]) {
            ipaddr = *(u_long*)remoteHost->h_addr_list[0];
        } else {
            IcmpCloseHandle(hIcmpFile);
            return false;
        }
    }

    std::vector<char> sendData(payloadSize, 'D');
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + payloadSize + 32;
    std::vector<char> replyBuffer(replySize, 0);

    IP_OPTION_INFORMATION ipOptions;
    ZeroMemory(&ipOptions, sizeof(ipOptions));
    ipOptions.Ttl = 64;
    ipOptions.Flags = IP_FLAG_DF; // Don't Fragment flag!

    DWORD dwRetVal = IcmpSendEcho2(
        hIcmpFile,
        NULL,
        NULL,
        NULL,
        ipaddr,
        sendData.data(),
        payloadSize,
        &ipOptions,
        replyBuffer.data(),
        replySize,
        800 // 800ms timeout
    );

    bool ok = false;
    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer.data());
        if (pEchoReply->Status == IP_SUCCESS) {
            ok = true;
        }
    }

    IcmpCloseHandle(hIcmpFile);
    return ok;
}

MtuResult MtuOptimizer::DiscoverOptimalMtu(const std::string& targetHost) {
    MtuResult result;

    int low = 1200;
    int high = 1472;
    int optimalPayload = -1;

    // Quick sanity check on low bound
    if (!ProbePacketSize(targetHost, 1200)) {
        result.optimalMtu = 1500;
        result.optimalMss = 1460;
        result.found = false;
        result.notes = "Target host did not reply to ICMP ping or path is unreachable.";
        return result;
    }

    // Check if 1472 passes directly (standard 1500 MTU)
    if (ProbePacketSize(targetHost, 1472)) {
        result.optimalMtu = 1500;
        result.optimalMss = 1460;
        result.found = true;
        result.notes = "Standard 1500 byte MTU confirmed with zero fragmentation.";
        return result;
    }

    // Binary search between low and high
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (ProbePacketSize(targetHost, static_cast<uint16_t>(mid))) {
            optimalPayload = mid;
            low = mid + 1; // Try higher
        } else {
            high = mid - 1; // Try lower
        }
    }

    if (optimalPayload > 0) {
        result.optimalMtu = optimalPayload + 28; // 20 IP + 8 ICMP
        result.optimalMss = result.optimalMtu - 40; // 20 IP + 20 TCP
        result.found = true;
        result.notes = "Optimal MTU discovered via DF Binary Search: " + std::to_string(result.optimalMtu);
    } else {
        result.optimalMtu = 1500;
        result.optimalMss = 1460;
        result.found = false;
        result.notes = "Could not converge; using standard 1500 MTU.";
    }

    return result;
}

OperationResult MtuOptimizer::ApplyMtuToInterface(const std::string& interfaceName, uint32_t mtu) {
    OperationResult res;
    std::stringstream ss;
    // For cmd.exe system(), enclose entire command line in quotes if quotes are used internally
    ss << "cmd.exe /c \"netsh interface ipv4 set subinterface \\\"" << interfaceName << "\\\" mtu=" << mtu << " store=persistent\" > nul 2>&1";
    
    std::string cmd = ss.str();
    int ret = system(cmd.c_str());
    if (ret == 0) {
        res.success = true;
        res.message = "Set MTU=" + std::to_string(mtu) + " on interface [" + interfaceName + "].";
    } else {
        res.success = false;
        res.message = "Failed to set MTU via netsh (check admin privileges).";
    }
    return res;
}

OperationResult MtuOptimizer::AutoDetectAndApplyMtu(const std::string& targetHost) {
    MtuResult probe = DiscoverOptimalMtu(targetHost);
    AdapterOptimizer adapterOpt;
    auto adapters = adapterOpt.GetActiveAdapters();

    if (adapters.empty()) {
        OperationResult r;
        r.success = false;
        r.message = "No active network adapters found.";
        return r;
    }

    int applied = 0;
    for (const auto& nic : adapters) {
        if (nic.isPhysical) {
            auto r = ApplyMtuToInterface(nic.name, probe.optimalMtu);
            if (r.success) applied++;
        }
    }

    OperationResult overall;
    overall.success = (applied > 0);
    overall.message = "Configured MTU=" + std::to_string(probe.optimalMtu) + 
                      " (MSS=" + std::to_string(probe.optimalMss) + ") on " + std::to_string(applied) + " interface(s).";
    overall.details = probe.notes;
    return overall;
}

} // namespace disping
