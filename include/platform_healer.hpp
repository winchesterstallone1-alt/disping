#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

class PlatformHealer {
public:
    PlatformHealer() = default;

    // Comprehensive healing: repairs stale registry PIDs, deletes .crash locks,
    // verifies loginusers.vdf, resets corrupted CEF cache, and restores AutoLogin flags.
    // terminateZombies is FALSE by default to avoid killing Steam during startup!
    static OperationResult HealSteamAndGames(bool terminateZombies = false);

    // Check if HKCU\Software\Valve\Steam\ActiveProcess\pid points to a dead or zombie process
    static bool RepairActiveProcessRegistry();

    // Remove any .crash or error-lock files in Steam directory
    static bool ClearCrashLocks(const std::string& customSteamPath = "");

    // Detect and terminate headless hung Steam processes that block the GUI from launching
    // Safely verifies process age > 60s and lack of UI before terminating.
    static int TerminateHungSteamZombies();

    // Fix registry flags that cause Steam to prompt for offline/error recovery mode
    static bool RepairSteamLoginSettings();

    // Ensure loginusers.vdf has MostRecent=1, RememberPassword=1 and WantsOfflineMode=0
    static bool RepairLoginUsersVdf(const std::string& customSteamPath = "");

    // Clean corrupted Chromium CEF GPU / shader caches if crash streak detected
    static bool CleanCorruptedHtmlCache();

    // Resolve Steam installation directory from Registry or standard locations
    static std::string GetSteamPath();
};

} // namespace disping
