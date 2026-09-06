#include "hardware_detector.hpp"
#include "ui_console.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace disping {

static void GetCpuId(uint32_t leaf, uint32_t subleaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
#if defined(_MSC_VER)
    int cpuInfo[4];
    __cpuidex(cpuInfo, static_cast<int>(leaf), static_cast<int>(subleaf));
    *eax = cpuInfo[0];
    *ebx = cpuInfo[1];
    *ecx = cpuInfo[2];
    *edx = cpuInfo[3];
#else
    __cpuid_count(leaf, subleaf, *eax, *ebx, *ecx, *edx);
#endif
}

NicVendor HardwareDetector::IdentifyNicVendor(const std::string& driverDesc) {
    std::string text = driverDesc;
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);

    if (text.find("intel") != std::string::npos || text.find("killer") != std::string::npos) {
        return (text.find("killer") != std::string::npos) ? NicVendor::Killer : NicVendor::Intel;
    }
    if (text.find("realtek") != std::string::npos) {
        return NicVendor::Realtek;
    }
    if (text.find("qualcomm") != std::string::npos || text.find("atheros") != std::string::npos) {
        return NicVendor::Qualcomm;
    }
    if (text.find("mediatek") != std::string::npos || text.find("ralink") != std::string::npos) {
        return NicVendor::MediaTek;
    }
    if (text.find("broadcom") != std::string::npos) {
        return NicVendor::Broadcom;
    }
    return NicVendor::Other;
}

HardwareProfile HardwareDetector::DetectHardware() {
    HardwareProfile prof;

    // 1. CPU Vendor
    uint32_t a = 0, b = 0, c = 0, d = 0;
    GetCpuId(0, 0, &a, &b, &c, &d);

    char vendorStr[13] = {0};
    *reinterpret_cast<uint32_t*>(&vendorStr[0]) = b;
    *reinterpret_cast<uint32_t*>(&vendorStr[4]) = d;
    *reinterpret_cast<uint32_t*>(&vendorStr[8]) = c;
    prof.cpuVendorStr = vendorStr;

    if (prof.cpuVendorStr == "GenuineIntel") {
        prof.cpuVendor = CpuVendor::Intel;
    } else if (prof.cpuVendorStr == "AuthenticAMD") {
        prof.cpuVendor = CpuVendor::Amd;
    } else {
        prof.cpuVendor = CpuVendor::Other;
    }

    // 2. CPU Brand String
    char brandStr[49] = {0};
    uint32_t maxExt = 0;
    GetCpuId(0x80000000, 0, &maxExt, &b, &c, &d);

    if (maxExt >= 0x80000004) {
        for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; ++leaf) {
            GetCpuId(leaf, 0, &a, &b, &c, &d);
            size_t off = (leaf - 0x80000002) * 16;
            *reinterpret_cast<uint32_t*>(&brandStr[off]) = a;
            *reinterpret_cast<uint32_t*>(&brandStr[off + 4]) = b;
            *reinterpret_cast<uint32_t*>(&brandStr[off + 8]) = c;
            *reinterpret_cast<uint32_t*>(&brandStr[off + 12]) = d;
        }
        prof.cpuBrand = brandStr;
        // Trim leading spaces
        size_t first = prof.cpuBrand.find_first_not_of(' ');
        if (first != std::string::npos) {
            prof.cpuBrand = prof.cpuBrand.substr(first);
        }
    }

    // 3. AMD 3D V-Cache (X3D) detection
    if (prof.cpuVendor == CpuVendor::Amd) {
        std::string lowerBrand = prof.cpuBrand;
        std::transform(lowerBrand.begin(), lowerBrand.end(), lowerBrand.begin(), ::tolower);
        if (lowerBrand.find("x3d") != std::string::npos || lowerBrand.find("3d v-cache") != std::string::npos) {
            prof.isAmdX3D = true;
        }
    }

    // 4. ISA Capabilities
    GetCpuId(1, 0, &a, &b, &c, &d);
    prof.hasSse2  = (d & (1 << 26)) != 0;
    prof.hasSse41 = (c & (1 << 19)) != 0;
    prof.hasSse42 = (c & (1 << 20)) != 0;
    prof.hasFma3  = (c & (1 << 12)) != 0;
    prof.hasAvx   = (c & (1 << 28)) != 0;

    GetCpuId(7, 0, &a, &b, &c, &d);
    prof.hasAvx2   = (b & (1 << 5)) != 0;
    prof.hasBmi1   = (b & (1 << 3)) != 0;
    prof.hasBmi2   = (b & (1 << 8)) != 0;
    prof.hasAvx512 = (b & (1 << 16)) != 0; // AVX512F
    bool hybridFlag = (d & (1 << 15)) != 0; // Hybrid processor bit in CPUID 7

    if (maxExt >= 0x80000001) {
        GetCpuId(0x80000001, 0, &a, &b, &c, &d);
        prof.hasRdtscp = (d & (1 << 27)) != 0;
    }
    prof.hasCrc32 = prof.hasSse42;

    // 5. Core Topology & Intel Hybrid P-Core / E-Core Detection via Windows API
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    prof.logicalCores = sysInfo.dwNumberOfProcessors;

    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &length);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && length > 0) {
        std::vector<BYTE> buffer(length);
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pInfo = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());

        if (GetLogicalProcessorInformationEx(RelationProcessorCore, pInfo, &length)) {
            BYTE* ptr = buffer.data();
            BYTE* end = buffer.data() + length;

            uint32_t pCores = 0;
            uint32_t eCores = 0;
            DWORD_PTR pCoresAffinity = 0;

            while (ptr < end) {
                PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX curr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);
                if (curr->Relationship == RelationProcessorCore) {
                    BYTE effClass = curr->Processor.EfficiencyClass;
                    DWORD_PTR coreMask = curr->Processor.GroupMask[0].Mask;

                    if (effClass > 0) {
                        pCores++;
                        pCoresAffinity |= coreMask;
                    } else {
                        eCores++;
                    }
                }
                ptr += curr->Size;
            }

            prof.physicalCores = pCores + eCores;

            // If we found cores with different efficiency classes, or CPUID marked it hybrid
            if ((pCores > 0 && eCores > 0) || (hybridFlag && pCores > 0)) {
                prof.isHybrid = true;
                prof.pCoreCount = pCores;
                prof.eCoreCount = eCores;
                prof.optimalGameAffinityMask = pCoresAffinity;
            } else {
                prof.isHybrid = false;
                prof.pCoreCount = prof.physicalCores;
                prof.eCoreCount = 0;
            }
        }
    }

    if (prof.physicalCores == 0) {
        prof.physicalCores = prof.logicalCores > 2 ? prof.logicalCores / 2 : prof.logicalCores;
        prof.pCoreCount = prof.physicalCores;
    }

    // 6. Memory (RAM) Profiling
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        prof.totalRamBytes = memStatus.ullTotalPhys;
        prof.totalRamGb = static_cast<uint32_t>((prof.totalRamBytes + (1ULL << 29)) / (1ULL << 30));
        prof.isLowRam = (prof.totalRamGb <= 8);
        prof.isHighRam = (prof.totalRamGb >= 16);
    }

    // 7. Optimal Gaming Affinity Calculation
    prof.optimalGameAffinityMask = CalculateOptimalGamingAffinity(prof);

    return prof;
}

DWORD_PTR HardwareDetector::CalculateOptimalGamingAffinity(const HardwareProfile& profile) {
    DWORD_PTR mask = 0;

    // SCENARIO 1: Intel Hybrid Architecture (12th/13th/14th Gen / Arrow Lake / Core Ultra)
    // Strategy: Pin exclusively to Performance Cores (P-Cores). Isolate Core 0 to prevent DPC driver lag.
    if (profile.isHybrid && profile.pCoreCount > 0) {
        mask = profile.optimalGameAffinityMask;
        // If we have at least 4 logical processors for P-cores, isolate the very first P-core (Core 0/1) for Windows DPC
        if (profile.logicalCores >= 8) {
            mask &= ~3ULL; // Clear logical threads 0 and 1
        }
        return mask != 0 ? mask : profile.optimalGameAffinityMask;
    }

    // SCENARIO 2: AMD 3D V-Cache (Ryzen 9 7900X3D / 7950X3D with Asymmetric Dual-CCD)
    // Strategy: Pin to CCD0 (Cores 0-7 / Threads 0-15) which contains the 96MB 3D V-Cache pool.
    if (profile.isAmdX3D && profile.logicalCores >= 24) {
        // Dual-CCD Ryzen with 12 or 16 cores. CCD0 has the 3D V-Cache!
        // Mask for first 8 cores (16 threads): 0xFFFF
        DWORD_PTR ccd0Mask = 0xFFFFULL;
        // Isolate Core 0 (threads 0, 1) for OS interrupts
        mask = ccd0Mask & ~3ULL;
        return mask;
    }

    // SCENARIO 3: Standard Multi-Core (AMD Ryzen 5/7, Intel Core i5/i7 non-hybrid >= 6 cores)
    // Strategy: Isolate Core 0 (bits 0 and 1) for OS interrupts, pin to all remaining physical & SMT cores.
    if (profile.logicalCores >= 6) {
        DWORD_PTR allCores = (profile.logicalCores >= 64) 
            ? ~0ULL 
            : ((1ULL << profile.logicalCores) - 1ULL);
        mask = allCores & ~3ULL; // Clear Core 0
        return mask;
    }

    // SCENARIO 4: Quad-Core or Dual-Core (Budget / Older Hardware)
    // Strategy: Do not starve game of threads. If 4 cores, isolate thread 0 only. If 2 cores, use all.
    if (profile.logicalCores == 4) {
        return 0x0E; // Threads 1, 2, 3
    }

    // Fallback: Use all available logical cores
    return (profile.logicalCores >= 64) ? ~0ULL : ((1ULL << profile.logicalCores) - 1ULL);
}

std::string HardwareDetector::GetIsaTierDescription(const HardwareProfile& profile) {
    if (profile.hasAvx512) {
        return "Tier 4: AVX-512 Ultra (512-bit Vector SIMD Engine)";
    }
    if (profile.hasAvx2) {
        return "Tier 3: AVX2 High-Performance (256-bit Vector SIMD Engine)";
    }
    if (profile.hasAvx) {
        return "Tier 2: AVX Classic (128-bit Vector Floating Point)";
    }
    if (profile.hasSse42) {
        return "Tier 1: SSE4.2 / SSE4.1 (Streaming SIMD Extensions)";
    }
    return "Tier 0: SSE2 Universal Baseline (100% Compatible Fallback)";
}

void HardwareDetector::PrintHardwareReport(const HardwareProfile& profile) {
    UIConsole::PrintHeader("ПРОФИЛЬ ОБОРУДОВАНИЯ И АДАПТИВНОЙ ОПТИМИЗАЦИИ (UNIVERSAL ENGINE)");

    std::cout << "  " << UIConsole::Bold() << "ПРОЦЕССОР (CPU):" << UIConsole::Reset() << "\n";
    std::cout << "  * Модель:            " << UIConsole::BrightCyan() << profile.cpuBrand << UIConsole::Reset() << "\n";
    std::cout << "  * Архитектура:       " << profile.cpuVendorStr;
    if (profile.isHybrid) {
        std::cout << UIConsole::BrightGreen() << " [Intel Hybrid: " << profile.pCoreCount << " P-Cores + " << profile.eCoreCount << " E-Cores]" << UIConsole::Reset();
    } else if (profile.isAmdX3D) {
        std::cout << UIConsole::BrightGreen() << " [AMD 3D V-Cache (X3D) Gaming Architecture]" << UIConsole::Reset();
    } else {
        std::cout << " [Standard Unified Multi-Core]";
    }
    std::cout << "\n";

    std::cout << "  * Ядра / Потоки:     " << profile.physicalCores << " физических ядер / " << profile.logicalCores << " логических потоков\n";
    std::cout << "  * Набор инструкций:  " << UIConsole::BrightYellow() << GetIsaTierDescription(profile) << UIConsole::Reset() << "\n";
    std::cout << "  * Поддержка SIMD:    " 
              << (profile.hasAvx512 ? "[AVX-512] " : "")
              << (profile.hasAvx2 ? "[AVX2] " : "")
              << (profile.hasAvx ? "[AVX] " : "")
              << (profile.hasFma3 ? "[FMA3] " : "")
              << (profile.hasSse42 ? "[SSE4.2] " : "")
              << (profile.hasRdtscp ? "[RDTSCP] " : "")
              << (profile.hasCrc32 ? "[HW-CRC32] " : "")
              << "\n";

    std::cout << "\n  " << UIConsole::Bold() << "ОПЕРАТИВНАЯ ПАМЯТЬ (RAM):" << UIConsole::Reset() << "\n";
    std::cout << "  * Объём памяти:      " << profile.totalRamGb << " GB RAM";
    if (profile.isLowRam) {
        std::cout << UIConsole::Yellow() << " [Бюджетный профиль <= 8GB: приоритет частой выгрузке кэша]" << UIConsole::Reset();
    } else if (profile.isHighRam) {
        std::cout << UIConsole::BrightGreen() << " [Высокопроизводительный профиль >= 16GB: блокировка ядра в ОЗУ (DisablePagingExecutive)]" << UIConsole::Reset();
    }
    std::cout << "\n";

    std::cout << "\n  " << UIConsole::Bold() << "АДАПТИВНАЯ АФФИНИТИ-ПРИВЯЗКА ДЛЯ ИГР:" << UIConsole::Reset() << "\n";
    std::cout << "  * Маска привязки:    0x" << std::hex << profile.optimalGameAffinityMask << std::dec << "\n";
    if (profile.isHybrid) {
        std::cout << "  * Стратегия:         " << UIConsole::BrightGreen() << "Игры зафиксированы строго на P-Cores. E-Cores и Core 0 изолированы от игры!" << UIConsole::Reset() << "\n";
    } else if (profile.isAmdX3D) {
        std::cout << "  * Стратегия:         " << UIConsole::BrightGreen() << "Игры привязаны к пулу 3D V-Cache CCD для максимального FPS!" << UIConsole::Reset() << "\n";
    } else {
        std::cout << "  * Стратегия:         " << UIConsole::BrightGreen() << "Ядро 0 изолировано под прерывания ОС (DPC/ISR). Игра использует остальные ядра!" << UIConsole::Reset() << "\n";
    }

    std::cout << "  ---------------------------------------------------------------------------------\n\n";
}

} // namespace disping
