#include "qos_optimizer.hpp"
#include "registry_util.hpp"
#include <iostream>

namespace disping {

std::vector<std::string> QoSOptimizer::GetDefaultGamingExecutables() const {
    return {
        "cs2.exe",
        "csgo.exe",
        "valorant.exe",
        "VALORANT-Win64-Shipping.exe",
        "dota2.exe",
        "r5apex.exe",
        "FortniteClient-Win64-Shipping.exe",
        "Overwatch.exe",
        "RustClient.exe",
        "EscapeFromTarkov.exe",
        "RainbowSix.exe",
        "Warzone.exe",
        "ModernWarfare.exe",
        "RocketLeague.exe",
        "LeagueClientUx.exe",
        "disping.exe"
    };
}

OperationResult QoSOptimizer::EnableUserTOS() {
    OperationResult res;
    if (!RegistryUtil::IsRunningAsAdmin()) {
        res.success = false;
        res.message = "Administrator privileges required to enable DSCP/QoS packet marking.";
        return res;
    }

    std::string tcpKey = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
    bool b1 = RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, tcpKey, "DisableUserTOSSetting", 0);

    std::string qosKey = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\QoS";
    bool b2 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, qosKey, "Do not use NLA", "1");

    res.success = (b1 && b2);
    res.message = res.success 
        ? "Enabled User TOS/DSCP marking in Windows network stack."
        : "Failed to configure TCP/IP TOS registry settings.";
    return res;
}

OperationResult QoSOptimizer::SetupGamingQoSPolicies(const std::vector<std::string>& customGameExecutables) {
    OperationResult res = EnableUserTOS();
    if (!res.success) {
        return res;
    }

    std::vector<std::string> exes = customGameExecutables.empty() 
        ? GetDefaultGamingExecutables() 
        : customGameExecutables;

    std::string qosRoot = "SOFTWARE\\Policies\\Microsoft\\Windows\\QoS";

    int count = 0;
    for (const auto& exe : exes) {
        std::string policyName = "DisPing_QoS_" + exe;
        std::string policyPath = qosRoot + "\\" + policyName;

        // Policy settings:
        // Version: "1.0"
        // Application Name: exe
        // Protocol: "*"
        // Local Port: "*"
        // Local IP: "*"
        // Local IP Prefix Length: "*"
        // Remote Port: "*"
        // Remote IP: "*"
        // Remote IP Prefix Length: "*"
        // DSCP Value: "46" (Expedited Forwarding - EF)
        // Throttle Rate: "-1" (Unlimited)

        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Version", "1.0");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Application Name", exe);
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Protocol", "*");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Local Port", "*");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Local IP", "*");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Local IP Prefix Length", "*");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Remote Port", "*");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Remote IP", "*");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Remote IP Prefix Length", "*");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "DSCP Value", "46");
        RegistryUtil::SetString(HKEY_LOCAL_MACHINE, policyPath, "Throttle Rate", "-1");

        count++;
    }

    res.success = (count > 0);
    res.message = "Configured Policy-Based QoS DSCP 46 (Expedited Forwarding) for " + std::to_string(count) + " game titles.";
    return res;
}

OperationResult QoSOptimizer::RemoveGamingQoSPolicies() {
    OperationResult res;
    std::string qosRoot = "SOFTWARE\\Policies\\Microsoft\\Windows\\QoS";
    auto subkeys = RegistryUtil::EnumerateSubKeys(HKEY_LOCAL_MACHINE, qosRoot);

    int count = 0;
    for (const auto& sk : subkeys) {
        if (sk.find("DisPing_QoS_") == 0) {
            RegDeleteKeyA(HKEY_LOCAL_MACHINE, (qosRoot + "\\" + sk).c_str());
            count++;
        }
    }

    res.success = true;
    res.message = "Removed " + std::to_string(count) + " DisPing QoS policies.";
    return res;
}

bool QoSOptimizer::IsQoSEnabled() const {
    DWORD val = 1;
    if (RegistryUtil::GetDword(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "DisableUserTOSSetting", val)) {
        return (val == 0);
    }
    return false;
}

} // namespace disping
