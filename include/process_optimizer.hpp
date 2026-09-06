#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>
#include <windows.h>

namespace disping {

struct RunningProcessInfo {
    DWORD pid;
    std::string name;
    DWORD priorityClass;
    DWORD_PTR affinityMask;
};

class ProcessOptimizer {
public:
    ProcessOptimizer() = default;

    // Scan running processes and find active game executables
    std::vector<RunningProcessInfo> FindActiveGames();

    // Optimize process priority and core affinity
    OperationResult BoostProcess(DWORD pid, bool pinToPerformanceCores = true);

    // Boost process by name (e.g. "cs2.exe")
    OperationResult BoostProcessByName(const std::string& processName, bool pinToPerformanceCores = true);

    // Automatically boost all currently running games
    OperationResult AutoBoostAllActiveGames();

    // Calculate optimal affinity mask for physical cores
    DWORD_PTR GetPhysicalCoresAffinityMask() const;

private:
    std::vector<std::string> GetKnownGameExecutables() const;
};

} // namespace disping
