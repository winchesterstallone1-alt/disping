#include "process_optimizer.hpp"
#include <tlhelp32.h>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace disping {

std::vector<std::string> ProcessOptimizer::GetKnownGameExecutables() const {
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
        "League of Legends.exe",
        "GTA5.exe",
        "cyberpunk2077.exe"
    };
}

std::vector<RunningProcessInfo> ProcessOptimizer::FindActiveGames() {
    std::vector<RunningProcessInfo> games;
    auto knownGames = GetKnownGameExecutables();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return games;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            std::wstring ws(pe.szExeFile);
            std::string exeName(ws.begin(), ws.end());

            // Convert to lowercase for comparison
            std::string lowerExe = exeName;
            std::transform(lowerExe.begin(), lowerExe.end(), lowerExe.begin(), ::tolower);

            for (const auto& kg : knownGames) {
                std::string lowerKg = kg;
                std::transform(lowerKg.begin(), lowerKg.end(), lowerKg.begin(), ::tolower);

                if (lowerExe == lowerKg) {
                    RunningProcessInfo info;
                    info.pid = pe.th32ProcessID;
                    info.name = exeName;
                    
                    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
                    if (hProc) {
                        info.priorityClass = GetPriorityClass(hProc);
                        DWORD_PTR procMask = 0, sysMask = 0;
                        GetProcessAffinityMask(hProc, &procMask, &sysMask);
                        info.affinityMask = procMask;
                        CloseHandle(hProc);
                    }
                    games.push_back(info);
                    break;
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return games;
}

DWORD_PTR ProcessOptimizer::GetPhysicalCoresAffinityMask() const {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD numCores = sysInfo.dwNumberOfProcessors;

    if (numCores <= 2) {
        return (1ULL << numCores) - 1; // All cores if <= 2
    }

    // Isolate Core 0 (which handles Windows DPCs, IRQs, OS scheduling)
    // Dedicate Cores 1..N-1 exclusively to game threads
    DWORD_PTR mask = 0;
    for (DWORD i = 1; i < numCores; ++i) {
        mask |= (1ULL << i);
    }
    return mask;
}

OperationResult ProcessOptimizer::BoostProcess(DWORD pid, bool pinToPerformanceCores) {
    OperationResult res;

    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        res.success = false;
        res.message = "Failed to open process PID " + std::to_string(pid) + " (access denied or terminated).";
        return res;
    }

    // 1. Set High Priority Class
    BOOL prioSuccess = SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);

    // 2. Set Affinity Mask (dedicate physical gaming cores, bypass core 0 OS DPC jitter)
    BOOL affSuccess = TRUE;
    if (pinToPerformanceCores) {
        DWORD_PTR mask = GetPhysicalCoresAffinityMask();
        affSuccess = SetProcessAffinityMask(hProcess, mask);
    }

    CloseHandle(hProcess);

    if (prioSuccess) {
        res.success = true;
        res.message = "Boosted PID " + std::to_string(pid) + " to HIGH_PRIORITY_CLASS"
                      + (affSuccess && pinToPerformanceCores ? " and pinned to isolated gaming cores." : ".");
    } else {
        res.success = false;
        res.message = "Failed to elevate process priority.";
    }
    return res;
}

OperationResult ProcessOptimizer::BoostProcessByName(const std::string& processName, bool pinToPerformanceCores) {
    OperationResult res;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        res.success = false;
        res.message = "Failed to snapshot processes.";
        return res;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    int boosted = 0;
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            std::wstring ws(pe.szExeFile);
            std::string exeName(ws.begin(), ws.end());

            std::string a = exeName;
            std::string b = processName;
            std::transform(a.begin(), a.end(), a.begin(), ::tolower);
            std::transform(b.begin(), b.end(), b.begin(), ::tolower);

            if (a == b) {
                auto r = BoostProcess(pe.th32ProcessID, pinToPerformanceCores);
                if (r.success) {
                    boosted++;
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);

    if (boosted > 0) {
        res.success = true;
        res.message = "Boosted " + std::to_string(boosted) + " instance(s) of [" + processName + "].";
    } else {
        res.success = false;
        res.message = "Process [" + processName + "] was not found running.";
    }
    return res;
}

OperationResult ProcessOptimizer::AutoBoostAllActiveGames() {
    auto games = FindActiveGames();
    if (games.empty()) {
        OperationResult r;
        r.success = true;
        r.message = "No active known game processes detected at this moment.";
        return r;
    }

    int count = 0;
    for (const auto& g : games) {
        auto r = BoostProcess(g.pid, true);
        if (r.success) count++;
    }

    OperationResult res;
    res.success = (count > 0);
    res.message = "Auto-boosted " + std::to_string(count) + " active game process(es) (High Priority & Core Isolation).";
    return res;
}

} // namespace disping
