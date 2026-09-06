#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>

namespace disping {

class RegistryUtil {
public:
    static bool SetDword(HKEY hRootKey, const std::string& subKey, const std::string& valueName, DWORD value);
    static bool GetDword(HKEY hRootKey, const std::string& subKey, const std::string& valueName, DWORD& outValue);
    
    static bool SetString(HKEY hRootKey, const std::string& subKey, const std::string& valueName, const std::string& value);
    static bool GetString(HKEY hRootKey, const std::string& subKey, const std::string& valueName, std::string& outValue);
    
    static bool DeleteValue(HKEY hRootKey, const std::string& subKey, const std::string& valueName);
    static bool ValueExists(HKEY hRootKey, const std::string& subKey, const std::string& valueName);
    static bool KeyExists(HKEY hRootKey, const std::string& subKey);

    static std::vector<std::string> EnumerateSubKeys(HKEY hRootKey, const std::string& subKey);
    static bool IsRunningAsAdmin();
};

} // namespace disping
