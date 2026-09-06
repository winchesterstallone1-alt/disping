#include "disping_asm.h"

namespace {

using ChecksumFn = uint16_t (*)(const void*, size_t);
using MemzeroFn = void (*)(void*, size_t);
using TscFn = uint64_t (*)(void);

ChecksumFn g_checksum_ptr = nullptr;
MemzeroFn g_memzero_ptr = nullptr;
TscFn g_tsc_ptr = nullptr;

void InitDispatchTable() {
    // Checksum dispatcher: AVX2 -> SSE2 universal fallback
    if (asm_has_avx2_support()) {
        g_checksum_ptr = asm_avx2_checksum;
    } else {
        g_checksum_ptr = asm_sse2_checksum;
    }

    // Memzero dispatcher: AVX2 -> SSE2 universal fallback
    if (asm_has_avx2_support()) {
        g_memzero_ptr = asm_avx2_memzero_nt;
    } else {
        g_memzero_ptr = asm_sse2_memzero_nt;
    }

    // TSC dispatcher: RDTSCP (20 cycles) -> CPUID serialization fallback
    if (asm_has_rdtscp_support()) {
        g_tsc_ptr = asm_read_tsc_fast_serialized;
    } else {
        g_tsc_ptr = asm_read_tsc_serialized;
    }
}

struct DispatchInit {
    DispatchInit() {
        InitDispatchTable();
    }
};

static DispatchInit s_init;

} // namespace

extern "C" {

uint16_t disping_fast_checksum_auto(const void* buffer, size_t len_bytes) {
    if (!g_checksum_ptr) InitDispatchTable();
    return g_checksum_ptr(buffer, len_bytes);
}

void disping_fast_memzero_auto(void* dst, size_t size_in_bytes) {
    if (!g_memzero_ptr) InitDispatchTable();
    g_memzero_ptr(dst, size_in_bytes);
}

uint64_t disping_read_tsc_auto(void) {
    if (!g_tsc_ptr) InitDispatchTable();
    return g_tsc_ptr();
}

}
