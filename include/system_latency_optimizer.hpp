#pragma once

#include "disping_types.hpp"
#include <atomic>
#include <thread>
#include <windows.h>

namespace disping {

class SystemLatencyOptimizer {
public:
    SystemLatencyOptimizer();
    ~SystemLatencyOptimizer();

    // Query current, min, and max timer resolution (in milliseconds)
    bool QueryTimerResolution(double& minResMs, double& maxResMs, double& currentResMs);

    // Set high-resolution timer (e.g. 0.5 ms / 5000 in 100ns units)
    OperationResult SetHighResolutionTimer(double targetMs = 0.5);

    // Restore standard timer resolution
    OperationResult RestoreTimerResolution();

    // Configure MMCSS Games profile in registry
    OperationResult OptimizeMMCSSGamesProfile();

    // Hardware-adapted memory management (DisablePagingExecutive for >= 16GB RAM)
    OperationResult OptimizeMemorySubsystem(bool isHighRam);

    // Wi-Fi Anti-Spike: Disable Windows Location Service (lfsvc BSSID periodic scan)
    OperationResult DisableLocationServices();
    OperationResult RestoreLocationServices();

    // Keep the timer locked in background until StopTimerDaemon()
    void StartTimerDaemon(double targetMs = 0.5);
    void StopTimerDaemon();

    bool IsDaemonRunning() const { return m_daemonRunning.load(); }
    double GetActiveResolutionMs() const { return m_activeResolutionMs; }

private:
    std::atomic<bool> m_daemonRunning{false};
    std::thread m_daemonThread;
    double m_activeResolutionMs = 15.625;
    bool m_timerSet = false;
    ULONG m_actualResolutionUnits = 0;

    typedef NTSTATUS(NTAPI* pfnNtSetTimerResolution)(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);
    typedef NTSTATUS(NTAPI* pfnNtQueryTimerResolution)(PULONG MaximumResolution, PULONG MinimumResolution, PULONG CurrentResolution);
    
    pfnNtSetTimerResolution m_pfnNtSetTimerResolution = nullptr;
    pfnNtQueryTimerResolution m_pfnNtQueryTimerResolution = nullptr;
};

} // namespace disping
