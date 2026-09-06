#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

class PlatformHealer {
public:
    PlatformHealer() = default;

    // Comprehensive healing: repairs stale registry PIDs, deletes .crash locks,
    // cleans up hung zombie processes, and restores AutoLogin / RememberPassword settings
    static OperationResult HealSteamAndGames();

    // Check if HKCU\Software\Valve\Steam\ActiveProcess\pid points to a dead or zombie process
    static bool RepairActiveProcessRegistry();

    // Remove any .crash or error-lock files in Steam directory
    static bool ClearCrashLocks(const std::string& customSteamPath = "");

    // Detect and terminate headless hung Steam processes that block the GUI from launching
    static int TerminateHungSteamZombies();

    // Fix registry flags that cause Steam to prompt for offline/error recovery mode
    static bool RepairSteamLoginSettings();

    // Resolve Steam installation directory from Registry or standard locations
    static std::string GetSteamPath();
};

} // namespace disping
