#include "memory_optimizer.hpp"
#include "vpn_guard.hpp"
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <iostream>

namespace disping {

MemoryOptimizer::MemoryOptimizer() {
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll) {
        m_pfnNtSetSystemInformation = reinterpret_cast<pfnNtSetSystemInformation>(
            reinterpret_cast<void*>(GetProcAddress(hNtDll, "NtSetSystemInformation")));
    }
}

bool MemoryOptimizer::EnablePrivilege(const wchar_t* privilegeName) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!LookupPrivilegeValueW(NULL, privilegeName, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    CloseHandle(hToken);
    return (ok == TRUE && GetLastError() == ERROR_SUCCESS);
}

void MemoryOptimizer::GetMemoryStats(uint64_t& totalMb, uint64_t& availableMb, uint32_t& loadPercent) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        totalMb = memInfo.ullTotalPhys / (1024 * 1024);
        availableMb = memInfo.ullAvailPhys / (1024 * 1024);
        loadPercent = memInfo.dwMemoryLoad;
    } else {
        totalMb = 0;
        availableMb = 0;
        loadPercent = 0;
    }
}

OperationResult MemoryOptimizer::PurgeStandbyList() {
    OperationResult res;
    EnablePrivilege(L"SeProfileSingleProcessPrivilege");
    EnablePrivilege(L"SeIncreaseQuotaPrivilege");

    if (!m_pfnNtSetSystemInformation) {
        res.success = false;
        res.message = "NtSetSystemInformation API not found in ntdll.";
        return res;
    }

    // SystemMemoryListInformation = 0x50 (80)
    SYSTEM_MEMORY_LIST_COMMAND command = MemoryPurgeStandbyList;
    LONG status = m_pfnNtSetSystemInformation(80, &command, sizeof(SYSTEM_MEMORY_LIST_COMMAND));

    if (status >= 0) {
        res.success = true;
        res.message = "Purged Windows Standby Memory List (eliminated pagefile stutter).";
    } else {
        res.success = false;
        res.message = "Failed to purge standby list (requires Administrator privileges).";
    }
    return res;
}

OperationResult MemoryOptimizer::EmptyAllWorkingSets() {
    OperationResult res;
    EnablePrivilege(L"SeIncreaseQuotaPrivilege");

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        // At least empty current process
        EmptyWorkingSet(GetCurrentProcess());
        res.success = true;
        res.message = "Emptied current process working set.";
        return res;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    int clearedCount = 0;
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4) continue; // Skip System
            
            std::wstring ws(pe.szExeFile);
            std::string exeName(ws.begin(), ws.end());
            std::string lowerExe = exeName;
            std::transform(lowerExe.begin(), lowerExe.end(), lowerExe.begin(), ::tolower);

            // PROTECT VPN & DPI BYPASS: Never flush working sets of Zapret (winws), Incy, Happ, etc.
            if (VpnGuard::IsProcessProtected(lowerExe)) {
                continue;
            }

            // PROTECT STEAM, DISCORD, GPU DRIVERS & GAMING PROCESSES:
            // Flushing working sets on Chromium/CEF (steamwebhelper, discord) or games (cs2.exe)
            // evicts GPU textures and causes STATUS_DRIVER_CANCELLED crashes!
            static const std::vector<std::string> s_gamingProtected = {
                "steam.exe", "steamwebhelper.exe", "steamservice.exe", "gameoverlayui.exe",
                "cs2.exe", "csgo.exe", "valorant.exe", "dota2.exe", "r5apex.exe",
                "discord.exe", "epicgameslauncher.exe", "riotclientservices.exe",
                "nvcontainer.exe", "amdrsserv.exe", "amdfendrs.exe", "dwmp.exe", "dwm.exe"
            };
            bool skipGaming = false;
            for (const auto& gp : s_gamingProtected) {
                if (lowerExe == gp) {
                    skipGaming = true;
                    break;
                }
            }
            if (skipGaming) {
                continue;
            }

            HANDLE hProc = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
            if (hProc) {
                if (EmptyWorkingSet(hProc)) {
                    clearedCount++;
                }
                CloseHandle(hProc);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);

    res.success = true;
    res.message = "Emptied working sets across " + std::to_string(clearedCount) + " running processes.";
    return res;
}

OperationResult MemoryOptimizer::CleanGamingMemory() {
    uint64_t beforeTotal, beforeAvail;
    uint32_t beforeLoad;
    GetMemoryStats(beforeTotal, beforeAvail, beforeLoad);

    auto r1 = PurgeStandbyList();
    auto r2 = EmptyAllWorkingSets();

    uint64_t afterTotal, afterAvail;
    uint32_t afterLoad;
    GetMemoryStats(afterTotal, afterAvail, afterLoad);

    OperationResult overall;
    overall.success = r1.success || r2.success;
    int64_t freedMb = static_cast<int64_t>(afterAvail) - static_cast<int64_t>(beforeAvail);
    if (freedMb < 0) freedMb = 0;

    overall.message = "Memory Optimization Complete: Freed ~" + std::to_string(freedMb) + 
                      " MB of RAM (Memory Load: " + std::to_string(beforeLoad) + "% -> " + std::to_string(afterLoad) + "%).";
    overall.details = r1.message + "\n" + r2.message;
    return overall;
}

} // namespace disping
