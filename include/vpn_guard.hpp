#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

struct VpnDetectionReport {
    bool vpnProcessDetected = false;
    bool tunAdapterDetected = false;
    bool fakeIpDnsDetected = false;
    std::vector<std::string> detectedProcesses;
    std::vector<std::string> detectedAdapters;
    std::vector<std::string> detectedDnsServers;
};

class VpnGuard {
public:
    // Check if any VPN, proxy, or DPI bypass tool (Zapret, Incy, Happ, GoodbyeDPI, etc.) is active
    static bool IsVpnOrDpiBypassActive();

    // Check if system currently uses Fake-IP DNS (198.18.x.x) or local proxy DNS (127.0.0.1)
    static bool HasFakeIpOrVpnDns();

    // Check if an adapter name or description belongs to a virtual / TUN / TAP / VPN adapter
    static bool IsVirtualOrVpnAdapter(const std::string& adapterName, const std::string& description);

    // Check if a process is a protected VPN / DPI bypass / network driver process
    static bool IsProcessProtected(const std::string& processName);

    // Full system scan and status report
    static VpnDetectionReport ScanVpnStatus();

    // Print human-readable report
    static void PrintVpnShieldStatus();

    // List of protected process names
    static const std::vector<std::string>& GetProtectedProcesses();
};

} // namespace disping
