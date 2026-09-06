#include "backup_manager.hpp"
#include "registry_util.hpp"
#include "qos_optimizer.hpp"
#include "system_latency_optimizer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace disping {

bool BackupManager::BackupExists(const std::string& backupPath) const {
    std::ifstream f(backupPath);
    return f.good();
}

OperationResult BackupManager::CreateBackup(const std::string& backupPath) {
    OperationResult res;
    std::ofstream out(backupPath);
    if (!out.is_open()) {
        res.success = false;
        res.message = "Failed to create backup file: " + backupPath;
        return res;
    }

    out << "{\n";
    out << "  \"timestamp\": \"" << __DATE__ << " " << __TIME__ << "\",\n";
    out << "  \"entries\": [\n";

    // Read NetworkThrottlingIndex & SystemResponsiveness
    DWORD throttle = 10, resp = 20;
    std::string profileKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile";
    RegistryUtil::GetDword(HKEY_LOCAL_MACHINE, profileKey, "NetworkThrottlingIndex", throttle);
    RegistryUtil::GetDword(HKEY_LOCAL_MACHINE, profileKey, "SystemResponsiveness", resp);

    out << "    {\"key\": \"" << profileKey << "\", \"name\": \"NetworkThrottlingIndex\", \"value\": " << throttle << "},\n";
    out << "    {\"key\": \"" << profileKey << "\", \"name\": \"SystemResponsiveness\", \"value\": " << resp << "}\n";
    out << "  ]\n";
    out << "}\n";

    out.close();

    GenerateRollbackScript("disping_rollback.bat");

    res.success = true;
    res.message = "Created system backup in [" + backupPath + "] and [disping_rollback.bat].";
    return res;
}

OperationResult BackupManager::GenerateRollbackScript(const std::string& scriptPath) {
    OperationResult res;
    std::ofstream out(scriptPath);
    if (!out.is_open()) {
        res.success = false;
        res.message = "Failed to write rollback script: " + scriptPath;
        return res;
    }

    out << "@echo off\n";
    out << ":: =====================================================\n";
    out << ":: DisPing Automatic Rollback & Restore Script\n";
    out << ":: Restores standard Windows network and system defaults\n";
    out << ":: =====================================================\n\n";
    out << "net session >nul 2>&1\n";
    out << "if %errorlevel% neq 0 (\n";
    out << "    echo [!] Administrator privileges required. Please right-click and Run as Administrator.\n";
    out << "    pause\n";
    out << "    exit /b 1\n";
    out << ")\n\n";

    out << "echo [*] Restoring Windows Network Throttling Index to 10...\n";
    out << "reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\" /v NetworkThrottlingIndex /t REG_DWORD /d 10 /f >nul\n";
    out << "echo [*] Restoring Windows System Responsiveness to 20...\n";
    out << "reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\" /v SystemResponsiveness /t REG_DWORD /d 20 /f >nul\n\n";

    out << "echo [*] Restoring Netsh TCP defaults...\n";
    out << "netsh int tcp set global autotuninglevel=normal >nul\n";
    out << "netsh int tcp set global congestionprovider=default >nul\n";
    out << "netsh int tcp set global ecncapability=default >nul\n";
    out << "netsh int tcp set global timestamps=allowed >nul\n";
    out << "netsh int tcp set global rss=enabled >nul\n";
    out << "netsh int tcp set global rsc=enabled >nul\n";
    out << "netsh int tcp set heuristics enabled >nul\n\n";

    out << "echo [*] Flushing DNS cache...\n";
    out << "ipconfig /flushdns >nul\n\n";

    out << "echo [OK] All settings successfully restored to Windows defaults!\n";
    out << "pause\n";
    out.close();

    res.success = true;
    res.message = "Generated standalone rollback script: " + scriptPath;
    return res;
}

OperationResult BackupManager::RestoreFactoryDefaults() {
    OperationResult res;
    if (!RegistryUtil::IsRunningAsAdmin()) {
        res.success = false;
        res.message = "Administrator privileges required to restore defaults.";
        return res;
    }

    // 1. Reset Network Throttling and System Responsiveness
    std::string profileKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile";
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, profileKey, "NetworkThrottlingIndex", 10);
    RegistryUtil::SetDword(HKEY_LOCAL_MACHINE, profileKey, "SystemResponsiveness", 20);

    // 2. Remove TCP interface customizations
    auto guids = RegistryUtil::EnumerateSubKeys(
        HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"
    );
    for (const auto& guid : guids) {
        std::string subKey = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + guid;
        RegistryUtil::DeleteValue(HKEY_LOCAL_MACHINE, subKey, "TcpAckFrequency");
        RegistryUtil::DeleteValue(HKEY_LOCAL_MACHINE, subKey, "TCPNoDelay");
        RegistryUtil::DeleteValue(HKEY_LOCAL_MACHINE, subKey, "TcpDelAckTicks");
    }

    // 3. Remove QoS policies
    QoSOptimizer qosOpt;
    qosOpt.RemoveGamingQoSPolicies();

    // 4. Netsh default TCP stack restore
    system("netsh int tcp set global autotuninglevel=normal > nul 2>&1");
    system("netsh int tcp set global congestionprovider=default > nul 2>&1");
    system("netsh int tcp set global ecncapability=default > nul 2>&1");
    system("netsh int tcp set global timestamps=allowed > nul 2>&1");
    system("netsh int tcp set global rss=enabled > nul 2>&1");
    system("netsh int tcp set global rsc=enabled > nul 2>&1");
    system("netsh int tcp set heuristics enabled > nul 2>&1");

    // 5. Restore timer resolution
    SystemLatencyOptimizer timerOpt;
    timerOpt.RestoreTimerResolution();

    res.success = true;
    res.message = "Factory Windows defaults restored for TCP/IP, Throttling, MMCSS, and QoS.";
    return res;
}

} // namespace disping
