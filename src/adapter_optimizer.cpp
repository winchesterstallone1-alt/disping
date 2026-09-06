#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include "adapter_optimizer.hpp"
#include "registry_util.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace disping {

std::vector<NetworkAdapterInfo> AdapterOptimizer::GetActiveAdapters() {
    std::vector<NetworkAdapterInfo> list;

    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != NULL; pCurr = pCurr->Next) {
            if (pCurr->OperStatus != IfOperStatusUp) {
                continue; // only active connected interfaces
            }

            // Skip loopback and tunnels
            if (pCurr->IfType == IF_TYPE_SOFTWARE_LOOPBACK || pCurr->IfType == IF_TYPE_TUNNEL) {
                continue;
            }

            NetworkAdapterInfo info;
            info.id = pCurr->AdapterName ? pCurr->AdapterName : "";
            
            // Name
            int len = WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, NULL, 0, NULL, NULL);
            if (len > 0) {
                std::string fname(len - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, &fname[0], len, NULL, NULL);
                info.name = fname;
            }

            // Description
            len = WideCharToMultiByte(CP_UTF8, 0, pCurr->Description, -1, NULL, 0, NULL, NULL);
            if (len > 0) {
                std::string desc(len - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, pCurr->Description, -1, &desc[0], len, NULL, NULL);
                info.description = desc;
            }

            // MAC Address
            std::stringstream macStream;
            for (ULONG i = 0; i < pCurr->PhysicalAddressLength; i++) {
                if (i > 0) macStream << "-";
                macStream << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                          << static_cast<int>(pCurr->PhysicalAddress[i]);
            }
            info.macAddress = macStream.str();

            // IPv4 Address
            for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress; pUnicast != NULL; pUnicast = pUnicast->Next) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                    char ipStr[INET_ADDRSTRLEN];
                    sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
                    info.ipv4Address = ipStr;
                    break;
                }
            }

            info.mtu = pCurr->Mtu;
            info.speedMbps = static_cast<uint32_t>(pCurr->TransmitLinkSpeed / 1000000ULL);
            info.isPhysical = (pCurr->IfType == IF_TYPE_ETHERNET_CSMACD || pCurr->IfType == IF_TYPE_IEEE80211);
            info.isConnected = true;

            list.push_back(info);
        }
    }

    return list;
}

std::vector<std::string> AdapterOptimizer::GetAdapterClassKeys() const {
    std::string rootClass = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}";
    auto subkeys = RegistryUtil::EnumerateSubKeys(HKEY_LOCAL_MACHINE, rootClass);
    
    std::vector<std::string> validAdapters;
    for (const auto& sk : subkeys) {
        std::string fullKey = rootClass + "\\" + sk;
        std::string driverDesc;
        if (RegistryUtil::GetString(HKEY_LOCAL_MACHINE, fullKey, "DriverDesc", driverDesc)) {
            validAdapters.push_back(fullKey);
        }
    }
    return validAdapters;
}

OperationResult AdapterOptimizer::DisableInterruptModeration() {
    OperationResult res;
    if (!RegistryUtil::IsRunningAsAdmin()) {
        res.success = false;
        res.message = "Administrator privileges required to optimize Network Adapters.";
        return res;
    }

    auto keys = GetAdapterClassKeys();
    int count = 0;
    for (const auto& key : keys) {
        bool b1 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*InterruptModeration", "0");
        bool b2 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*InterruptModerationRate", "0");
        bool b3 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "ITR", "0"); // Intel specific
        if (b1 || b2 || b3) {
            count++;
        }
    }

    res.success = (count > 0);
    res.message = "Disabled Interrupt Moderation (Instant CPU IRQ) on " + std::to_string(count) + " adapter profiles.";
    return res;
}

OperationResult AdapterOptimizer::DisableLargeSendOffload() {
    OperationResult res;
    auto keys = GetAdapterClassKeys();
    int count = 0;
    for (const auto& key : keys) {
        bool b1 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*LsoV2IPv4", "0");
        bool b2 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*LsoV2IPv6", "0");
        bool b3 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*LSOv4IPv4", "0");
        if (b1 || b2 || b3) {
            count++;
        }
    }

    res.success = (count > 0);
    res.message = "Disabled Large Send Offload (LSOv2) to eliminate packet burst serialization latency.";
    return res;
}

OperationResult AdapterOptimizer::DisableEnergySaving() {
    OperationResult res;
    auto keys = GetAdapterClassKeys();
    int count = 0;
    for (const auto& key : keys) {
        bool b1 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*EEE", "0");
        bool b2 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*EnergyEfficientEthernet", "0");
        bool b3 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "GreenEthernet", "0");
        bool b4 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "ReduceSpeedOnPowerDown", "0");
        bool b5 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "AutoPowerSaveModeEnabled", "0");
        bool b6 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "PnPCapabilities", "24"); // Prevent sleep
        if (b1 || b2 || b3 || b4 || b5 || b6) {
            count++;
        }
    }

    res.success = (count > 0);
    res.message = "Disabled Green Ethernet / PHY Power Saving to keep hardware links actively awake.";
    return res;
}

OperationResult AdapterOptimizer::MaximizeAdapterBuffers() {
    OperationResult res;
    auto keys = GetAdapterClassKeys();
    int count = 0;
    for (const auto& key : keys) {
        bool b1 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*ReceiveBuffers", "2048");
        bool b2 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*TransmitBuffers", "2048");
        bool b3 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "RxDescriptors", "2048");
        bool b4 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "TxDescriptors", "2048");
        if (b1 || b2 || b3 || b4) {
            count++;
        }
    }

    res.success = (count > 0);
    res.message = "Maximized Network Adapter Ring Buffers (2048 Descriptors).";
    return res;
}

OperationResult AdapterOptimizer::DisableFlowControl() {
    OperationResult res;
    auto keys = GetAdapterClassKeys();
    int count = 0;
    for (const auto& key : keys) {
        bool b1 = RegistryUtil::SetString(HKEY_LOCAL_MACHINE, key, "*FlowControl", "0");
        if (b1) {
            count++;
        }
    }

    res.success = (count > 0);
    res.message = "Disabled Flow Control (prevents ethernet pause frame lag spikes).";
    return res;
}

OperationResult AdapterOptimizer::OptimizeAllNetworkAdapters() {
    OperationResult r1 = DisableInterruptModeration();
    OperationResult r2 = DisableLargeSendOffload();
    OperationResult r3 = DisableEnergySaving();
    OperationResult r4 = MaximizeAdapterBuffers();
    OperationResult r5 = DisableFlowControl();

    OperationResult overall;
    overall.success = r1.success || r2.success || r3.success || r4.success || r5.success;
    overall.message = overall.success 
        ? "Network Adapter Hardware properties tuned for minimum latency!" 
        : "Failed to modify adapter properties (run as Administrator).";
    overall.details = r1.message + "\n" + r2.message + "\n" + r3.message + "\n" + r4.message + "\n" + r5.message;
    return overall;
}

} // namespace disping
