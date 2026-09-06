#pragma once

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// Check if AVX2 instruction set is supported by the CPU
int asm_has_avx2_support(void);

// Check if RDTSCP instruction is supported by the CPU
int asm_has_rdtscp_support(void);

// 64-bit dual-accumulator high-performance RFC 1071 internet checksum in x86-64 assembly
uint16_t asm_fast_checksum_x64(const void* buffer, size_t len_bytes);
uint16_t asm_avx2_checksum(const void* buffer, size_t len_bytes);

// Time Stamp Counter reading
uint64_t asm_read_tsc_serialized(void);
uint64_t asm_read_tsc_fast(void);
uint64_t asm_read_tsc_fast_serialized(void);

// Non-temporal streaming store memory zero (bypasses L1/L2 cache pollution)
void asm_fast_memzero_nt(void* dst, size_t size_in_bytes);
void asm_avx2_memzero_nt(void* dst, size_t size_in_bytes);

// AVX2 Non-temporal packet copier
void asm_avx2_memcpy_nt(void* dst, const void* src, size_t size_in_bytes);

// Hardware accelerated CRC32-C (Castagnoli) 64-bit
uint32_t asm_crc32_fast(const void* buffer, size_t len_bytes);

// Single-pass SIMD statistics calculator (Min, Max, Avg, Variance, StdDev)
void asm_calc_ping_stats_simd(
    const double* rtts, 
    size_t count, 
    double* outMin, 
    double* outMax, 
    double* outAvg, 
    double* outStdDev
);

// RFC 3550 jitter calculation in SIMD: J(i) = J(i-1) + (|D| - J(i-1)) / 16.0
double asm_calc_jitter_rfc3550(double current_jitter, double transit_diff);

// Sub-microsecond spin wait without OS thread context switch
void asm_spin_wait_ns(uint64_t target_cycles);

#ifdef __cplusplus
}
#endif
