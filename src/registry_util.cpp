#include "registry_util.hpp"
#include <iostream>

namespace disping {

bool RegistryUtil::SetDword(HKEY hRootKey, const std::string& subKey, const std::string& valueName, DWORD value) {
    HKEY hKey;
    LONG result = RegCreateKeyExA(
        hRootKey,
        subKey.c_str(),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE | KEY_WOW64_64KEY,
        NULL,
        &hKey,
        NULL
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegSetValueExA(
        hKey,
        valueName.c_str(),
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(DWORD)
    );

    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

bool RegistryUtil::GetDword(HKEY hRootKey, const std::string& subKey, const std::string& valueName, DWORD& outValue) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(
        hRootKey,
        subKey.c_str(),
        0,
        KEY_QUERY_VALUE | KEY_WOW64_64KEY,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwData = 0;

    result = RegQueryValueExA(
        hKey,
        valueName.c_str(),
        NULL,
        &dwType,
        reinterpret_cast<BYTE*>(&dwData),
        &dwSize
    );

    RegCloseKey(hKey);
    if (result == ERROR_SUCCESS && dwType == REG_DWORD) {
        outValue = dwData;
        return true;
    }
    return false;
}

bool RegistryUtil::SetString(HKEY hRootKey, const std::string& subKey, const std::string& valueName, const std::string& value) {
    HKEY hKey;
    LONG result = RegCreateKeyExA(
        hRootKey,
        subKey.c_str(),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE | KEY_WOW64_64KEY,
        NULL,
        &hKey,
        NULL
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegSetValueExA(
        hKey,
        valueName.c_str(),
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        static_cast<DWORD>(value.length() + 1)
    );

    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

bool RegistryUtil::GetString(HKEY hRootKey, const std::string& subKey, const std::string& valueName, std::string& outValue) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(
        hRootKey,
        subKey.c_str(),
        0,
        KEY_QUERY_VALUE | KEY_WOW64_64KEY,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    char buffer[1024];
    DWORD dwSize = sizeof(buffer);
    DWORD dwType = REG_SZ;

    result = RegQueryValueExA(
        hKey,
        valueName.c_str(),
        NULL,
        &dwType,
        reinterpret_cast<BYTE*>(buffer),
        &dwSize
    );

    RegCloseKey(hKey);
    if (result == ERROR_SUCCESS && (dwType == REG_SZ || dwType == REG_EXPAND_SZ)) {
        outValue = std::string(buffer);
        return true;
    }
    return false;
}

bool RegistryUtil::DeleteValue(HKEY hRootKey, const std::string& subKey, const std::string& valueName) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(
        hRootKey,
        subKey.c_str(),
        0,
        KEY_SET_VALUE | KEY_WOW64_64KEY,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegDeleteValueA(hKey, valueName.c_str());
    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

bool RegistryUtil::ValueExists(HKEY hRootKey, const std::string& subKey, const std::string& valueName) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(
        hRootKey,
        subKey.c_str(),
        0,
        KEY_QUERY_VALUE | KEY_WOW64_64KEY,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegQueryValueExA(
        hKey,
        valueName.c_str(),
        NULL,
        NULL,
        NULL,
        NULL
    );

    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

bool RegistryUtil::KeyExists(HKEY hRootKey, const std::string& subKey) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(
        hRootKey,
        subKey.c_str(),
        0,
        KEY_READ | KEY_WOW64_64KEY,
        &hKey
    );

    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

std::vector<std::string> RegistryUtil::EnumerateSubKeys(HKEY hRootKey, const std::string& subKey) {
    std::vector<std::string> subKeys;
    HKEY hKey;
    LONG result = RegOpenKeyExA(
        hRootKey,
        subKey.c_str(),
        0,
        KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        return subKeys;
    }

    char nameBuffer[256];
    DWORD dwIndex = 0;
    DWORD dwSize = sizeof(nameBuffer);

    while (RegEnumKeyExA(hKey, dwIndex, nameBuffer, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        subKeys.push_back(std::string(nameBuffer));
        dwIndex++;
        dwSize = sizeof(nameBuffer);
    }

    RegCloseKey(hKey);
    return subKeys;
}

bool RegistryUtil::IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(
        &ntAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup))
    {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return (isAdmin == TRUE);
}

} // namespace disping
