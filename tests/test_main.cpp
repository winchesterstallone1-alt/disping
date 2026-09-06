#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <cstring>
#include <windows.h>

#include "disping_asm.h"
#include "disping_types.hpp"
#include "system_latency_optimizer.hpp"
#include "ping_monitor.hpp"
#include "process_optimizer.hpp"
#include "mtu_optimizer.hpp"

using namespace disping;

int g_passedTests = 0;
int g_failedTests = 0;

#define RUN_TEST(fn) \
    do { \
        std::cout << "[RUNNING] " << #fn << "..." << std::flush; \
        try { \
            fn(); \
            std::cout << " \033[32m[PASS]\033[0m\n"; \
            g_passedTests++; \
        } catch (const std::exception& ex) { \
            std::cout << " \033[31m[FAIL: " << ex.what() << "]\033[0m\n"; \
            g_failedTests++; \
        } catch (...) { \
            std::cout << " \033[31m[FAIL: Unknown exception]\033[0m\n"; \
            g_failedTests++; \
        } \
    } while (0)

void Test_AssemblyChecksum() {
    // 1. Zero length
    uint16_t c0 = asm_fast_checksum_x64(nullptr, 0);
    assert(c0 == 0);

    // 2. Simple pattern
    char data1[4] = {0x01, 0x02, 0x03, 0x04};
    uint16_t c1 = asm_fast_checksum_x64(data1, sizeof(data1));
    assert(c1 != 0);

    // 3. Odd length
    char data2[5] = {0x10, 0x20, 0x30, 0x40, 0x50};
    uint16_t c2 = asm_fast_checksum_x64(data2, sizeof(data2));
    assert(c2 != 0);

    // 4. Large buffer (>64 bytes for unrolled loop)
    std::vector<char> data3(256, 0x55);
    uint16_t c3 = asm_fast_checksum_x64(data3.data(), data3.size());
    assert(c3 != 0);
}

void Test_AssemblyTsc() {
    uint64_t t1 = asm_read_tsc_serialized();
    Sleep(5);
    uint64_t t2 = asm_read_tsc_serialized();
    assert(t2 > t1);
    assert((t2 - t1) > 1000ULL); // At least several thousand cycles
}

void Test_AssemblyJitter() {
    double j0 = 10.0;
    double d = 26.0;
    // Expected: 10.0 + (|26.0| - 10.0) / 16.0 = 10.0 + 16.0 / 16.0 = 11.0
    double j1 = asm_calc_jitter_rfc3550(j0, d);
    assert(std::fabs(j1 - 11.0) < 0.0001);

    // Negative transit diff: |-8.0| = 8.0, 10.0 + (8.0 - 10.0) / 16.0 = 10.0 - 0.125 = 9.875
    double j2 = asm_calc_jitter_rfc3550(10.0, -8.0);
    assert(std::fabs(j2 - 9.875) < 0.0001);
}

void Test_AssemblyMemzero() {
    alignas(16) char buffer[128];
    std::memset(buffer, 0xAA, sizeof(buffer));

    // Memzero using ASM non-temporal routine
    asm_fast_memzero_nt(buffer, sizeof(buffer));

    for (size_t i = 0; i < sizeof(buffer); ++i) {
        assert(buffer[i] == 0);
    }
}

void Test_AssemblyAvx2Support() {
    int hasAvx2 = asm_has_avx2_support();
    // AMD Zen 3+ supports AVX2
    assert(hasAvx2 == 1);
}

void Test_AssemblyFastTsc() {
    uint64_t t1 = asm_read_tsc_fast_serialized();
    Sleep(5);
    uint64_t t2 = asm_read_tsc_fast_serialized();
    assert(t2 > t1);
    assert((t2 - t1) > 1000ULL);
}

void Test_AssemblyCrc32() {
    const char msg[] = "123456789";
    uint32_t crc = asm_crc32_fast(msg, 9);
    assert(crc != 0);
    assert(crc == 0xe3069283); // Standard CRC32-C of "123456789"
}

void Test_AssemblyMemcpy() {
    alignas(32) char src[256];
    alignas(32) char dst[256];
    for (size_t i = 0; i < sizeof(src); ++i) src[i] = static_cast<char>(i);
    std::memset(dst, 0, sizeof(dst));

    asm_avx2_memcpy_nt(dst, src, sizeof(src));

    for (size_t i = 0; i < sizeof(src); ++i) {
        assert(dst[i] == src[i]);
    }
}

void Test_AssemblySimdStats() {
    std::vector<double> rtts = {10.0, 20.0, 30.0, 40.0};
    double outMin = 0, outMax = 0, outAvg = 0, outStdDev = 0;
    asm_calc_ping_stats_simd(rtts.data(), rtts.size(), &outMin, &outMax, &outAvg, &outStdDev);

    assert(std::fabs(outMin - 10.0) < 0.001);
    assert(std::fabs(outMax - 40.0) < 0.001);
    assert(std::fabs(outAvg - 25.0) < 0.001);
    // Population std dev of [10, 20, 30, 40] = sqrt(125) = ~11.1803
    assert(std::fabs(outStdDev - std::sqrt(125.0)) < 0.01);
}

void Test_AssemblySpinWait() {
    uint64_t t1 = asm_read_tsc_fast();
    asm_spin_wait_ns(5000); // 5000 cycles
    uint64_t t2 = asm_read_tsc_fast();
    assert(t2 >= t1 + 5000);
}

void Test_TimerResolutionQuery() {
    SystemLatencyOptimizer latOpt;
    double minMs = 0, maxMs = 0, currMs = 0;
    bool ok = latOpt.QueryTimerResolution(minMs, maxMs, currMs);
    assert(ok);
    assert(minMs > 0.0);
    assert(currMs > 0.0);
}

void Test_SparklineGeneration() {
    std::vector<double> history = {10.0, 12.0, 15.0, 20.0, -1.0, 25.0};
    std::string spark = PingMonitor::GenerateSparkline(history, 10);
    assert(!spark.empty());
    assert(spark.find('X') != std::string::npos); // Has packet loss indicator 'X'
}

void Test_ProcessAffinityMaskCalculation() {
    ProcessOptimizer procOpt;
    DWORD_PTR mask = procOpt.GetPhysicalCoresAffinityMask();
    assert(mask != 0);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors > 2) {
        // Bit 0 must be 0 (Core 0 isolated for OS DPC / Interrupt processing)
        assert((mask & 1ULL) == 0);
    }
}

void Test_LocalPing() {
    PingMonitor mon;
    double rtt = mon.PingSingle("127.0.0.1", 1000);
    (void)rtt;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    std::cout << "\n========================================\n";
    std::cout << "   disping Automated Test Suite         \n";
    std::cout << "========================================\n\n";

    RUN_TEST(Test_AssemblyChecksum);
    RUN_TEST(Test_AssemblyTsc);
    RUN_TEST(Test_AssemblyJitter);
    RUN_TEST(Test_AssemblyMemzero);
    RUN_TEST(Test_AssemblyAvx2Support);
    RUN_TEST(Test_AssemblyFastTsc);
    RUN_TEST(Test_AssemblyCrc32);
    RUN_TEST(Test_AssemblyMemcpy);
    RUN_TEST(Test_AssemblySimdStats);
    RUN_TEST(Test_AssemblySpinWait);
    RUN_TEST(Test_TimerResolutionQuery);
    RUN_TEST(Test_SparklineGeneration);
    RUN_TEST(Test_ProcessAffinityMaskCalculation);
    RUN_TEST(Test_LocalPing);

    std::cout << "\n----------------------------------------\n";
    std::cout << "Tests Summary: Passed = " << g_passedTests 
              << ", Failed = " << g_failedTests << "\n";
    std::cout << "----------------------------------------\n\n";

    WSACleanup();
    return (g_failedTests == 0) ? 0 : 1;
}
