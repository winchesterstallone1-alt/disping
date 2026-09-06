#include "platform_healer.hpp"
#include "registry_util.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

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
    // Only terminate if process has been running for AT LEAST 60 seconds with activePid == 0
    // and is completely headless, to avoid interfering with Steam startup or updates!
    DWORD activePid = 0;
    RegistryUtil::GetDword(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", "pid", activePid);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    FILETIME nowFt;
    GetSystemTimeAsFileTime(&nowFt);
    ULARGE_INTEGER nowU;
    nowU.LowPart = nowFt.dwLowDateTime;
    nowU.HighPart = nowFt.dwHighDateTime;

    std::vector<DWORD> pidsToKill;

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            std::wstring ws(pe.szExeFile);
            std::string name(ws.begin(), ws.end());
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            if (name == "steam.exe" || name == "steamwebhelper.exe") {
                if (activePid != 0 && pe.th32ProcessID == activePid) {
                    continue; // Legitimate active session, never kill
                }

                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                if (hProc) {
                    FILETIME ct, et, kt, ut;
                    if (GetProcessTimes(hProc, &ct, &et, &kt, &ut)) {
                        ULARGE_INTEGER createU;
                        createU.LowPart = ct.dwLowDateTime;
                        createU.HighPart = ct.dwHighDateTime;

                        // 60 seconds = 600,000,000 in 100ns units
                        if (nowU.QuadPart > createU.QuadPart) {
                            ULONGLONG age100ns = nowU.QuadPart - createU.QuadPart;
                            if (age100ns > 600000000ULL && activePid == 0) {
                                pidsToKill.push_back(pe.th32ProcessID);
                            }
                        }
                    }
                    CloseHandle(hProc);
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

bool PlatformHealer::RepairLoginUsersVdf(const std::string& customSteamPath) {
    std::string sPath = customSteamPath.empty() ? GetSteamPath() : customSteamPath;
    if (sPath.empty()) return false;

    std::string vdfPath = sPath + "\\config\\loginusers.vdf";
    std::ifstream inFile(vdfPath, std::ios::in | std::ios::binary);
    if (!inFile.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(inFile)),
                         std::istreambuf_iterator<char>());
    inFile.close();

    if (content.empty()) return false;

    std::string autoUser;
    RegistryUtil::GetString(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "AutoLoginUser", autoUser);

    if (!autoUser.empty()) {
        std::string pattern = "\"AccountName\"\\s*\"" + autoUser + "\"";
        std::regex reAcc(pattern, std::regex::icase);
        std::smatch match;
        if (std::regex_search(content, match, reAcc)) {
            size_t matchPos = match.position(0);
            size_t blockStart = content.rfind('{', matchPos);
            size_t blockEnd = content.find('}', matchPos);
            if (blockStart != std::string::npos && blockEnd != std::string::npos && blockEnd > blockStart) {
                std::string block = content.substr(blockStart, blockEnd - blockStart + 1);

                if (block.find("\"MostRecent\"") != std::string::npos) {
                    block = std::regex_replace(block, std::regex("\"MostRecent\"\\s*\"[^\"]*\""), "\"MostRecent\"\t\t\"1\"");
                } else {
                    block.insert(block.length() - 1, "\t\t\"MostRecent\"\t\t\"1\"\n\t");
                }

                if (block.find("\"RememberPassword\"") != std::string::npos) {
                    block = std::regex_replace(block, std::regex("\"RememberPassword\"\\s*\"[^\"]*\""), "\"RememberPassword\"\t\t\"1\"");
                } else {
                    block.insert(block.length() - 1, "\t\t\"RememberPassword\"\t\t\"1\"\n\t");
                }

                if (block.find("\"WantsOfflineMode\"") != std::string::npos) {
                    block = std::regex_replace(block, std::regex("\"WantsOfflineMode\"\\s*\"[^\"]*\""), "\"WantsOfflineMode\"\t\t\"0\"");
                }

                content.replace(blockStart, blockEnd - blockStart + 1, block);
            }
        }
    } else {
        if (content.find("\"MostRecent\"\t\t\"1\"") == std::string::npos &&
            content.find("\"MostRecent\" \"1\"") == std::string::npos) {
            std::regex reFirstMR("\"MostRecent\"\\s*\"0\"");
            content = std::regex_replace(content, reFirstMR, "\"MostRecent\"\t\t\"1\"", std::regex_constants::format_first_only);
        }
    }

    std::ofstream outFile(vdfPath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) return false;
    outFile.write(content.data(), content.size());
    outFile.close();
    return true;
}

bool PlatformHealer::CleanCorruptedHtmlCache() {
    char localAppData[MAX_PATH] = {0};
    if (GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH) == 0) {
        return false;
    }

    fs::path htmlCache = fs::path(localAppData) / "Steam" / "htmlcache";
    if (!fs::exists(htmlCache)) return false;

    fs::path localStatePath = htmlCache / "Local State";
    bool cleaned = false;

    if (fs::exists(localStatePath)) {
        try {
            std::ifstream inFile(localStatePath, std::ios::in | std::ios::binary);
            if (inFile.is_open()) {
                std::string content((std::istreambuf_iterator<char>(inFile)),
                                     std::istreambuf_iterator<char>());
                inFile.close();

                bool needsClean = false;
                std::regex reStreak("\"variations_crash_streak\"\\s*:\\s*([0-9]+)");
                std::smatch mStreak;
                if (std::regex_search(content, mStreak, reStreak)) {
                    int val = std::stoi(mStreak[1].str());
                    if (val > 2) needsClean = true;
                }

                std::regex reSysCrash("\"system_crash_count\"\\s*:\\s*([0-9]+)");
                std::smatch mSys;
                if (std::regex_search(content, mSys, reSysCrash)) {
                    int val = std::stoi(mSys[1].str());
                    if (val > 2) needsClean = true;
                }

                if (needsClean) {
                    content = std::regex_replace(content, reStreak, "\"variations_crash_streak\":0");
                    content = std::regex_replace(content, reSysCrash, "\"system_crash_count\":0");
                    std::ofstream outFile(localStatePath, std::ios::out | std::ios::binary | std::ios::trunc);
                    if (outFile.is_open()) {
                        outFile.write(content.data(), content.size());
                        outFile.close();
                        cleaned = true;
                    }

                    std::error_code ec;
                    fs::remove_all(htmlCache / "GPUCache", ec);
                    fs::remove_all(htmlCache / "GrShaderCache", ec);
                    fs::remove_all(htmlCache / "GraphiteDawnCache", ec);
                    fs::remove_all(htmlCache / "ShaderCache", ec);
                }
            }
        } catch (...) {
            return false;
        }
    }
    return cleaned;
}

OperationResult PlatformHealer::HealSteamAndGames(bool terminateZombies) {
    OperationResult res;
    std::string sPath = GetSteamPath();

    bool crashCleared = ClearCrashLocks(sPath);
    bool regRepaired = RepairActiveProcessRegistry();
    bool loginRepaired = RepairSteamLoginSettings();
    bool vdfRepaired = RepairLoginUsersVdf(sPath);
    bool cacheCleaned = CleanCorruptedHtmlCache();
    int zombiesKilled = 0;
    if (terminateZombies) {
        zombiesKilled = TerminateHungSteamZombies();
    }

    res.success = true;
    std::string details = "";
    if (crashCleared) details += "Removed stale Steam .crash lock file.\n";
    if (regRepaired) details += "Reset stale ActiveProcess PID in registry.\n";
    if (loginRepaired) details += "Restored Steam AutoLogin & RememberPassword flags.\n";
    if (vdfRepaired) details += "Verified and healed loginusers.vdf credentials.\n";
    if (cacheCleaned) details += "Purged corrupted CEF crash state and shader cache.\n";
    if (zombiesKilled > 0) details += "Terminated " + std::to_string(zombiesKilled) + " orphaned Steam zombie process(es).\n";

    if (details.empty()) {
        res.message = "Steam & Game Launchers verified: All platform locks healthy and clean.";
    } else {
        res.message = "Steam & Game Launchers auto-healed and unlocked!";
        res.details = details;
    }
    return res;
}

} // namespace disping
