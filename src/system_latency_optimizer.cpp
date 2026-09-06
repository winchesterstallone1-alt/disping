#include "system_latency_optimizer.hpp"
#include "registry_util.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mmsystem.h>
#include <avrt.h>

namespace disping {

SystemLatencyOptimizer::SystemLatencyOptimizer() {
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll) {
        m_pfnNtSetTimerResolution = reinterpret_cast<pfnNtSetTimerResolution>(
            reinterpret_cast<void*>(GetProcAddress(hNtDll, "NtSetTimerResolution")));
        m_pfnNtQueryTimerResolution = reinterpret_cast<pfnNtQueryTimerResolution>(
            reinterpret_cast<void*>(GetProcAddress(hNtDll, "NtQueryTimerResolution")));
    }
}

SystemLatencyOptimizer::~SystemLatencyOptimizer() {
    StopTimerDaemon();
    RestoreTimerResolution();
}

bool SystemLatencyOptimizer::QueryTimerResolution(double& minResMs, double& maxResMs, double& currentResMs) {
    if (!m_pfnNtQueryTimerResolution) {
        return false;
    }
    ULONG minUnits = 0, maxUnits = 0, currUnits = 0;
    NTSTATUS status = m_pfnNtQueryTimerResolution(&minUnits, &maxUnits, &currUnits);
    if (status == 0) { // STATUS_SUCCESS
        // 100ns units -> convert to ms: units / 10000.0
        minResMs = minUnits / 10000.0;
        maxResMs = maxUnits / 10000.0;
        currentResMs = currUnits / 10000.0;
        return true;
    }
    return false;
}

OperationResult SystemLatencyOptimizer::SetHighResolutionTimer(double targetMs) {
    OperationResult res;

    // Convert targetMs to 100ns units (e.g. 0.5ms = 5000 units, 1.0ms = 10000 units)
    ULONG desiredUnits = static_cast<ULONG>(targetMs * 10000.0);
    if (desiredUnits < 5000) desiredUnits = 5000; // minimum supported by Windows is 0.5ms

    if (m_pfnNtSetTimerResolution) {
        ULONG actualUnits = 0;
        NTSTATUS status = m_pfnNtSetTimerResolution(desiredUnits, TRUE, &actualUnits);
        if (status == 0) {
            m_timerSet = true;
            m_actualResolutionUnits = actualUnits;
            m_activeResolutionMs = actualUnits / 10000.0;

            std::stringstream ss;
            ss << "OS Timer Resolution locked to ultra-low " 
               << std::fixed << std::setprecision(3) << m_activeResolutionMs << " ms (NtSetTimerResolution).";
            res.success = true;
            res.message = ss.str();
            return res;
        }
    }

    // Fallback to WinMM timeBeginPeriod
    UINT period = static_cast<UINT>(targetMs < 1.0 ? 1 : targetMs);
    if (timeBeginPeriod(period) == TIMERR_NOERROR) {
        m_timerSet = true;
        m_activeResolutionMs = period;
        res.success = true;
        res.message = "OS Timer Resolution set to " + std::to_string(period) + ".0 ms via timeBeginPeriod.";
        return res;
    }

    res.success = false;
    res.message = "Failed to set high-resolution timer.";
    return res;
}

OperationResult SystemLatencyOptimizer::RestoreTimerResolution() {
    OperationResult res;
    if (m_timerSet) {
        if (m_pfnNtSetTimerResolution) {
            ULONG dummy = 0;
            m_pfnNtSetTimerResolution(m_actualResolutionUnits, FALSE, &dummy);
        }
        timeEndPeriod(1);
        m_timerSet = false;
        res.success = true;
        res.message = "Restored default OS timer resolution.";
    } else {
        res.success = true;
        res.message = "Timer resolution was not overridden.";
    }
    return res;
}

OperationResult SystemLatencyOptimizer::OptimizeMMCSSGamesProfile() {
    OperationResult res;
    if (!RegistryUtil::IsRunningAsAdmin()) {
        res.success = false;
        res.message = "Administrator privileges required to optimize MMCSS.";
        return res;
    }

    std::string taskGamesKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games";

    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, taskGamesKey, "Affinity", 0);
    RegistryUtil::SetString(HKEY_LOCAL_MACHINE, taskGamesKey, "Background Only", "False");
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, taskGamesKey, "Clock Rate", 10000);
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, taskGamesKey, "GPU Priority", 8);
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, taskGamesKey, "Priority", 6);
    RegistryUtil::SetString(HKEY_LOCAL_MACHINE, taskGamesKey, "Scheduling Category", "High");
    RegistryUtil::SetString(HKEY_LOCAL_MACHINE, taskGamesKey, "SFIO Priority", "High");

    res.success = true;
    res.message = "Configured MMCSS Games Profile: GPU Priority 8, Priority 6, High Scheduling Category.";
    return res;
}

void SystemLatencyOptimizer::StartTimerDaemon(double targetMs) {
    if (m_daemonRunning.load()) {
        return;
    }

    m_daemonRunning.store(true);
    m_daemonThread = std::thread([this, targetMs]() {
        SetHighResolutionTimer(targetMs);
        while (m_daemonRunning.load()) {
            Sleep(100);
        }
        RestoreTimerResolution();
    });
}

void SystemLatencyOptimizer::StopTimerDaemon() {
    if (m_daemonRunning.load()) {
        m_daemonRunning.store(false);
        if (m_daemonThread.joinable()) {
            m_daemonThread.join();
        }
    }
}

} // namespace disping
