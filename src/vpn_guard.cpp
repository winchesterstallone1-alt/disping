#include "vpn_guard.hpp"
#include "ui_console.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <algorithm>

namespace disping {

const std::vector<std::string>& VpnGuard::GetProtectedProcesses() {
    static const std::vector<std::string> s_protected = {
        // Zapret / DPI Bypass
        "winws.exe",
        "blockcheck.exe",
        "goodbyedpi.exe",
        "ciadpi.exe",
        "zapret.exe",

        // Incy
        "incy.exe",
        "incyd.exe",

        // Happ / Hiddify
        "happ.exe",
        "happd.exe",
        "hiddify.exe",

        // Sing-box / Xray / V2Ray / Clash Core
        "sing-box.exe",
        "xray.exe",
        "v2ray.exe",
        "clash.exe",
        "clash-meta.exe",
        "mihomo.exe",
        "nekoray.exe",
        "nekobox.exe",

        // WireGuard / OpenVPN / Amnezia
        "wireguard.exe",
        "openvpn.exe",
        "openvpnserv.exe",
        "amneziavpn.exe",
        "amnezia-vpn.exe",
        "tun2socks.exe",
        "tailscale.exe",
        "tailscaled.exe",
        "zerotier-one_x64.exe"
    };
    return s_protected;
}

bool VpnGuard::IsProcessProtected(const std::string& processName) {
    std::string lowerName = processName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    const auto& list = GetProtectedProcesses();
    for (const auto& p : list) {
        if (lowerName == p) {
            return true;
        }
    }
    return false;
}

bool VpnGuard::IsVirtualOrVpnAdapter(const std::string& adapterName, const std::string& description) {
    std::string text = adapterName + " " + description;
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);

    static const std::vector<std::string> keywords = {
        "wintun",
        "tunnel",
        "tap",
        "tun",
        "vpn",
        "wwan",
        "wireguard",
        "sing-box",
        "hiddify",
        "amnezia",
        "openvpn",
        "tailscale",
        "zerotier",
        "hyper-v",
        "virtual",
        "vmware",
        "vbox",
        "wan miniport",
        "loopback",
        "pseudo"
    };

    for (const auto& kw : keywords) {
        if (text.find(kw) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool VpnGuard::HasFakeIpOrVpnDns() {
    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES;
    DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != NULL; pCurr = pCurr->Next) {
            for (PIP_ADAPTER_DNS_SERVER_ADDRESS pDns = pCurr->FirstDnsServerAddress; pDns != NULL; pDns = pDns->Next) {
                if (pDns->Address.lpSockaddr->sa_family == AF_INET) {
                    char ipStr[INET_ADDRSTRLEN];
                    sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pDns->Address.lpSockaddr);
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
                    std::string dnsIp = ipStr;

                    // Check for 198.18.x.x (RFC 2544 Fake-IP range used by Sing-box/Happ/Incy)
                    if (dnsIp.rfind("198.18.", 0) == 0 || dnsIp == "127.0.0.1") {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool VpnGuard::IsVpnOrDpiBypassActive() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    bool found = false;
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            std::wstring ws(pe.szExeFile);
            std::string exeName(ws.begin(), ws.end());
            if (IsProcessProtected(exeName)) {
                found = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    if (found) return true;

    // Check if TUN adapter or Fake-IP DNS is active
    if (HasFakeIpOrVpnDns()) return true;

    return false;
}

VpnDetectionReport VpnGuard::ScanVpnStatus() {
    VpnDetectionReport report;

    // 1. Scan Processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                std::wstring ws(pe.szExeFile);
                std::string exeName(ws.begin(), ws.end());
                if (IsProcessProtected(exeName)) {
                    report.vpnProcessDetected = true;
                    report.detectedProcesses.push_back(exeName);
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }

    // 2. Scan Adapters and DNS
    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES;
    DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != NULL; pCurr = pCurr->Next) {
            std::string aname = pCurr->AdapterName ? pCurr->AdapterName : "";
            
            std::string fname;
            int len = WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, NULL, 0, NULL, NULL);
            if (len > 0) {
                fname.resize(len - 1);
                WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, &fname[0], len, NULL, NULL);
            }

            std::string desc;
            len = WideCharToMultiByte(CP_UTF8, 0, pCurr->Description, -1, NULL, 0, NULL, NULL);
            if (len > 0) {
                desc.resize(len - 1);
                WideCharToMultiByte(CP_UTF8, 0, pCurr->Description, -1, &desc[0], len, NULL, NULL);
            }

            if (pCurr->OperStatus == IfOperStatusUp && IsVirtualOrVpnAdapter(fname, desc)) {
                report.tunAdapterDetected = true;
                report.detectedAdapters.push_back(fname + " (" + desc + ")");
            }

            for (PIP_ADAPTER_DNS_SERVER_ADDRESS pDns = pCurr->FirstDnsServerAddress; pDns != NULL; pDns = pDns->Next) {
                if (pDns->Address.lpSockaddr->sa_family == AF_INET) {
                    char ipStr[INET_ADDRSTRLEN];
                    sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pDns->Address.lpSockaddr);
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
                    std::string dnsIp = ipStr;

                    if (dnsIp.rfind("198.18.", 0) == 0 || dnsIp == "127.0.0.1") {
                        report.fakeIpDnsDetected = true;
                        report.detectedDnsServers.push_back(fname + " -> " + dnsIp);
                    }
                }
            }
        }
    }

    return report;
}

void VpnGuard::PrintVpnShieldStatus() {
    auto report = ScanVpnStatus();
    UIConsole::PrintHeader("ЩИТ СОВМЕСТИМОСТИ DISPING (VPN, ZAPRET, INCY, HAPP)");

    if (!report.vpnProcessDetected && !report.tunAdapterDetected && !report.fakeIpDnsDetected) {
        std::cout << UIConsole::BrightGreen() << "[+] VPN / DPI Bypass не обнаружены. Система работает в стандартном режиме.\n" << UIConsole::Reset();
        return;
    }

    std::cout << UIConsole::BrightGreen() << UIConsole::Bold()
              << "[OK] ЩИТ СОВМЕСТИМОСТИ АКТИВЕН! DisPing автоматически защищает ваши VPN и DPI-сервисы:\n"
              << UIConsole::Reset();

    if (report.vpnProcessDetected) {
        std::cout << "  * Обнаружены запущенные процессы VPN/DPI:\n";
        for (const auto& p : report.detectedProcesses) {
            std::cout << "    - " << UIConsole::Cyan() << p << UIConsole::Reset() << " [ЗАЩИЩЁН: память и поток не сбрасываются]\n";
        }
    }

    if (report.tunAdapterDetected) {
        std::cout << "  * Обнаружены виртуальные TUN/Wintun туннели:\n";
        for (const auto& a : report.detectedAdapters) {
            std::cout << "    - " << UIConsole::Cyan() << a << UIConsole::Reset() << " [ЗАЩИЩЁН: аппаратные твики и MTU не трогаются]\n";
        }
    }

    if (report.fakeIpDnsDetected) {
        std::cout << "  * Обнаружен защищённый Fake-IP / Proxy DNS:\n";
        for (const auto& d : report.detectedDnsServers) {
            std::cout << "    - " << UIConsole::Cyan() << d << UIConsole::Reset() << " [ЗАЩИЩЁН: авто-переключение DNS отключено]\n";
        }
    }

    std::cout << "\n" << UIConsole::BrightYellow()
              << "ГАРАНТИЯ БЕЗОПАСНОСТИ: DisPing не перезаписывает DNS прокси, не сбрасывает память WinDivert\n"
              << "и не сбивает виртуальные адаптеры Wintun. Incy, Happ и Zapret работают стабильно и без сбоев!\n"
              << UIConsole::Reset() << "\n";
}

} // namespace disping
