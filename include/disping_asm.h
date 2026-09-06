#pragma once

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// 64-bit high performance RFC 1071 internet checksum in x86-64 assembly
uint16_t asm_fast_checksum_x64(const void* buffer, size_t len_bytes);

// Fully serialized Time Stamp Counter via CPUID + RDTSC
uint64_t asm_read_tsc_serialized(void);

// Fast Time Stamp Counter via RDTSCP with LFENCE
uint64_t asm_read_tsc_fast(void);

// Check if RDTSCP instruction is supported by the CPU
int asm_has_rdtscp_support(void);

// Non-temporal streaming store memory zero (bypasses L1/L2 cache pollution)
void asm_fast_memzero_nt(void* dst, size_t size_in_bytes);

// RFC 3550 jitter calculation in SIMD: J(i) = J(i-1) + (|D| - J(i-1)) / 16.0
double asm_calc_jitter_rfc3550(double current_jitter, double transit_diff);

#ifdef __cplusplus
}
#endif
