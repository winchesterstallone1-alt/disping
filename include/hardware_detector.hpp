#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>

namespace disping {

enum class CpuVendor {
    Intel,
    Amd,
    Other
};

enum class NicVendor {
    Intel,
    Realtek,
    Killer,
    Qualcomm,
    MediaTek,
    Broadcom,
    Other
};

struct HardwareProfile {
    // CPU
    CpuVendor cpuVendor = CpuVendor::Other;
    std::string cpuVendorStr = "Unknown";
    std::string cpuBrand = "Unknown CPU";
    uint32_t logicalCores = 1;
    uint32_t physicalCores = 1;
    uint32_t pCoreCount = 1;
    uint32_t eCoreCount = 0;
    bool isHybrid = false;           // Intel Alder/Raptor/Arrow Lake (P-cores + E-cores)
    bool isAmdX3D = false;           // AMD Ryzen 3D V-Cache processor (e.g. 7800X3D, 5800X3D)
    DWORD_PTR optimalGameAffinityMask = 0;

    // ISA capabilities
    bool hasAvx512 = false;
    bool hasAvx2 = false;
    bool hasAvx = false;
    bool hasFma3 = false;
    bool hasSse42 = false;
    bool hasSse41 = false;
    bool hasSse2 = false;
    bool hasBmi1 = false;
    bool hasBmi2 = false;
    bool hasRdtscp = false;
    bool hasCrc32 = false;

    // Memory (RAM)
    uint64_t totalRamBytes = 0;
    uint32_t totalRamGb = 0;
    bool isLowRam = false;           // <= 8 GB
    bool isHighRam = false;          // >= 16 GB

    // Network adapters
    std::vector<std::string> physicalNicNames;
    std::vector<NicVendor> detectedNicVendors;
};

class HardwareDetector {
public:
    static HardwareProfile DetectHardware();
    static void PrintHardwareReport(const HardwareProfile& profile);
    static DWORD_PTR CalculateOptimalGamingAffinity(const HardwareProfile& profile);
    static std::string GetIsaTierDescription(const HardwareProfile& profile);
    static NicVendor IdentifyNicVendor(const std::string& driverDesc);
};

} // namespace disping
