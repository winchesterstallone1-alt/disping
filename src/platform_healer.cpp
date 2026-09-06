#include "platform_healer.hpp"
#include "registry_util.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <algorithm>

namespace disping {

std::string PlatformHealer::GetSteamPath() {
    std::string path;
    // 1. Try HKCU
    if (RegistryUtil::GetString(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", path)) {
        std::replace(path.begin(), path.end(), '/', '\\');
        return path;
    }
    // 2. Try HKLM 64-bit
    if (RegistryUtil::GetString(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", "InstallPath", path)) {
        return path;
    }
    // 3. Standard fallback
    return "C:\\Program Files (x86)\\Steam";
}

bool PlatformHealer::ClearCrashLocks(const std::string& customSteamPath) {
    std::string sPath = customSteamPath.empty() ? GetSteamPath() : customSteamPath;
    if (sPath.empty()) return false;

    std::string crashFile = sPath + "\\.crash";
    DWORD attr = GetFileAttributesA(crashFile.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) {
        DeleteFileA(crashFile.c_str());
        return true;
    }
    return false;
}

bool PlatformHealer::RepairActiveProcessRegistry() {
    DWORD activePid = 0;
    if (!RegistryUtil::GetDword(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", "pid", activePid)) {
        return false;
    }

    if (activePid == 0) {
        return false; // Already clean
    }

    // Check if the process exists
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, activePid);
    bool shouldReset = false;

    if (!hProc) {
        // PID does not exist in the system (stale dead PID)
        shouldReset = true;
    } else {
        // PID exists: verify if it's actually steam.exe
        char exePath[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameA(hProc, 0, exePath, &size)) {
            std::string sName = exePath;
            std::string lower = sName;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find("steam.exe") == std::string::npos) {
                // Not steam.exe, stale PID assigned to another process
                shouldReset = true;
            }
        }
        CloseHandle(hProc);
    }

    if (shouldReset) {
        RegistryUtil::SetDword(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", "pid", 0);
        return true;
    }

    return false;
}

int PlatformHealer::TerminateHungSteamZombies() {
    // If ActiveProcess/pid is 0 or invalid, but steam.exe processes are running headlessly,
    // they are hung background zombies blocking user GUI launches
    DWORD activePid = 0;
    RegistryUtil::GetDword(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", "pid", activePid);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    std::vector<DWORD> pidsToKill;

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            std::wstring ws(pe.szExeFile);
            std::string name(ws.begin(), ws.end());
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            if (name == "steam.exe" || name == "steamwebhelper.exe") {
                if (activePid == 0) {
                    // Stale zombie process left behind with no active session
                    pidsToKill.push_back(pe.th32ProcessID);
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);

    int killed = 0;
    for (DWORD pid : pidsToKill) {
        HANDLE hP = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hP) {
            TerminateProcess(hP, 0);
            CloseHandle(hP);
            killed++;
        }
    }

    return killed;
}

bool PlatformHealer::RepairSteamLoginSettings() {
    bool b1 = RegistryUtil::SetDword(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "RememberPassword", 1);
    bool b2 = RegistryUtil::SetDword(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "AlreadyRetriedOfflineMode", 0);
    bool b3 = RegistryUtil::SetDword(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "StartupModeTmp", 0);
    bool b4 = RegistryUtil::SetDword(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "StartupModeTmpIsValid", 0);
    return (b1 && b2 && b3 && b4);
}

OperationResult PlatformHealer::HealSteamAndGames() {
    OperationResult res;
    std::string sPath = GetSteamPath();

    bool crashCleared = ClearCrashLocks(sPath);
    bool regRepaired = RepairActiveProcessRegistry();
    int zombiesKilled = TerminateHungSteamZombies();
    bool loginRepaired = RepairSteamLoginSettings();

    res.success = true;
    std::string details = "";
    if (crashCleared) details += "Removed stale Steam .crash lock file.\n";
    if (regRepaired) details += "Reset stale ActiveProcess PID in registry.\n";
    if (zombiesKilled > 0) details += "Terminated " + std::to_string(zombiesKilled) + " orphaned Steam zombie process(es).\n";
    if (loginRepaired) details += "Restored Steam AutoLogin & RememberPassword flags.\n";

    if (details.empty()) {
        res.message = "Steam & Game Launchers verified: All platform locks healthy and clean.";
    } else {
        res.message = "Steam & Game Launchers auto-healed and unlocked!";
        res.details = details;
    }
    return res;
}

} // namespace disping
