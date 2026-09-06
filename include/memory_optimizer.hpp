#pragma once

#include "disping_types.hpp"
#include <cstdint>
#include <windows.h>

namespace disping {

class MemoryOptimizer {
public:
    MemoryOptimizer();

    // Purge Windows Standby List and file cache
    OperationResult PurgeStandbyList();

    // Empty working sets of processes
    OperationResult EmptyAllWorkingSets();

    // Combined memory cleanup for gaming
    OperationResult CleanGamingMemory();

    // Get current memory status (used, total, free in MB)
    void GetMemoryStats(uint64_t& totalMb, uint64_t& availableMb, uint32_t& loadPercent);

private:
    typedef enum _SYSTEM_MEMORY_LIST_COMMAND {
        MemoryCaptureAccessedBits,
        MemoryCaptureAndResetAccessedBits,
        MemoryEmptyWorkingSets,
        MemoryFlushModifiedList,
        MemoryPurgeStandbyList,
        MemoryPurgeLowPriorityStandbyList,
        MemoryCommandMax
    } SYSTEM_MEMORY_LIST_COMMAND;

    typedef LONG(NTAPI* pfnNtSetSystemInformation)(INT SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength);
    pfnNtSetSystemInformation m_pfnNtSetSystemInformation = nullptr;
    
    bool EnablePrivilege(const wchar_t* privilegeName);
};

} // namespace disping
