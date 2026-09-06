#pragma once

#include "disping_types.hpp"
#include <string>

namespace disping {

class BackupManager {
public:
    BackupManager() = default;

    // Create full system snapshot backup to file
    OperationResult CreateBackup(const std::string& backupPath = "disping_backup.json");

    // Generate standalone batch rollback script
    OperationResult GenerateRollbackScript(const std::string& scriptPath = "disping_rollback.bat");

    // Restore Windows default network & system settings
    OperationResult RestoreFactoryDefaults();

    // Check if a backup file exists
    bool BackupExists(const std::string& backupPath = "disping_backup.json") const;
};

} // namespace disping
