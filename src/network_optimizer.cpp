#include "network_optimizer.hpp"
#include "registry_util.hpp"
#include <iostream>
#include <sstream>
#include <array>
#include <memory>
#include <cstdio>
#include <windows.h>

namespace disping {

std::string NetworkOptimizer::RunCommand(const std::string& cmd) const {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::vector<std::string> NetworkOptimizer::GetInterfaceGuids() const {
    return RegistryUtil::EnumerateSubKeys(
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"
    );
}

OperationResult NetworkOptimizer::OptimizeTcpNoDelayAndAckFrequency() {
    OperationResult res;
    if (!RegistryUtil::IsRunningAsAdmin()) {
        res.success = false;
        res.message = "Administrator privileges required to modify TCP/IP registry settings.";
        return res;
    }

    auto guids = GetInterfaceGuids();
    if (guids.empty()) {
        res.success = false;
        res.message = "No network interface GUIDs found in registry.";
        return res;
    }

    int successCount = 0;
    for (const auto& guid : guids) {
        std::string subKey = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + guid;
        
        bool b1 = RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, subKey, "TcpAckFrequency", 1);
        bool b2 = RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, subKey, "TCPNoDelay", 1);
        bool b3 = RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, subKey, "TcpDelAckTicks", 0);

        if (b1 && b2 && b3) {
            successCount++;
        }
    }

    res.success = (successCount > 0);
    res.message = "Optimized TCPNoDelay & TcpAckFrequency across " + std::to_string(successCount) + " interfaces.";
    return res;
}

OperationResult NetworkOptimizer::OptimizeNetworkThrottlingAndResponsiveness() {
    OperationResult res;
    if (!RegistryUtil::IsRunningAsAdmin()) {
        res.success = false;
        res.message = "Administrator privileges required.";
        return res;
    }

    std::string profileKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile";
    
    bool b1 = RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, profileKey, "NetworkThrottlingIndex", 0xFFFFFFFF);
    bool b2 = RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, profileKey, "SystemResponsiveness", 0);
    bool b3 = RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, profileKey, "NoLazyMode", 1);
    (void)b3;

    res.success = (b1 && b2);
    res.message = res.success ? "Network Throttling disabled and System Responsiveness set to 100% gaming priority."
                              : "Failed to configure SystemProfile registry values.";
    return res;
}

OperationResult NetworkOptimizer::OptimizeTcpParameters() {
    OperationResult res;
    if (!RegistryUtil::IsRunningAsAdmin()) {
        res.success = false;
        res.message = "Administrator privileges required.";
        return res;
    }

    std::string tcpKey = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";

    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, tcpKey, "DefaultTTL", 64);
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, tcpKey, "MaxUserPort", 65534);
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, tcpKey, "TcpTimedWaitDelay", 30);
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, tcpKey, "StrictTimeWaitSeqCheck", 1);
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, tcpKey, "DisableTaskOffload", 0);
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, tcpKey, "EnableWsd", 0);

    res.success = true;
    res.message = "Configured optimal TCP/IP global parameters (TTL, MaxUserPort, TimedWaitDelay).";
    return res;
}

OperationResult NetworkOptimizer::OptimizeTcpGlobalNetsh() {
    OperationResult res;
    
    std::vector<std::string> commands = {
        "netsh int tcp set global autotuninglevel=normal",
        "netsh int tcp set global ecncapability=disabled",
        "netsh int tcp set global timestamps=disabled",
        "netsh int tcp set global rss=enabled",
        "netsh int tcp set global rsc=disabled",
        "netsh int tcp set heuristics disabled",
        "netsh int ip set global taskoffload=enabled"
    };

    std::stringstream outputLog;
    for (const auto& cmd : commands) {
        std::string out = RunCommand(cmd);
        outputLog << "[" << cmd << "] -> " << out;
    }

    std::string bbrOut = RunCommand("netsh int tcp set supplemental template=internet congestionprovider=bbr");
    if (bbrOut.find("The parameter is incorrect") != std::string::npos || bbrOut.find("Error") != std::string::npos) {
        std::string cubicOut = RunCommand("netsh int tcp set supplemental template=internet congestionprovider=cubic");
        if (cubicOut.find("The parameter is incorrect") != std::string::npos) {
            RunCommand("netsh int tcp set supplemental template=internet congestionprovider=ctcp");
        }
    }

    res.success = true;
    res.message = "Executed Netsh TCP/IP stack optimization rules (RSS, autotuning, RSC disabled, timestamps disabled).";
    res.details = outputLog.str();
    return res;
}

OperationResult NetworkOptimizer::ApplyAllNetworkTweaks() {
    OperationResult r1 = OptimizeTcpNoDelayAndAckFrequency();
    OperationResult r2 = OptimizeNetworkThrottlingAndResponsiveness();
    OperationResult r3 = OptimizeTcpParameters();
    OperationResult r4 = OptimizeTcpGlobalNetsh();

    OperationResult overall;
    overall.success = r1.success && r2.success && r3.success;
    overall.message = overall.success 
        ? "All Network TCP/IP tweaks successfully applied!" 
        : "Some network tweaks failed (ensure running as Administrator).";
    overall.details = r1.message + "\n" + r2.message + "\n" + r3.message + "\n" + r4.message;
    return overall;
}

bool NetworkOptimizer::AreTweaksApplied() const {
    DWORD throttle = 0;
    if (RegistryUtil::GetDword(HKEY_LOCAL_MACHINE, 
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile", 
        "NetworkThrottlingIndex", throttle)) {
        return (throttle == 0xFFFFFFFF);
    }
    return false;
}

} // namespace disping
