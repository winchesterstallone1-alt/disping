; ==============================================================================
; disping_routines.asm - High-Performance x86-64 / AVX2 Assembly Routines for disping
; Architecture: x86-64 (Windows x64 ABI: RCX, RDX, R8, R9, XMM0-3 / YMM0-3)
; ==============================================================================

default rel

section .data
align 32
abs_mask_pd:
    dq 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF
jitter_gain_val:
    dq 0.0625, 0.0625, 0.0625, 0.0625   ; 1.0 / 16.0

section .text

global asm_fast_checksum_x64
global asm_avx2_checksum
global asm_read_tsc_serialized
global asm_read_tsc_fast
global asm_read_tsc_fast_serialized
global asm_has_rdtscp_support
global asm_has_avx2_support
global asm_fast_memzero_nt
global asm_avx2_memzero_nt
global asm_avx2_memcpy_nt
global asm_calc_jitter_rfc3550
global asm_crc32_fast
global asm_calc_ping_stats_simd
global asm_spin_wait_ns

; ------------------------------------------------------------------------------
; int asm_has_avx2_support(void)
; Checks CPUID EAX=7, ECX=0 for AVX2 support in EBX (bit 5)
; ------------------------------------------------------------------------------
asm_has_avx2_support:
    push rbx
    mov eax, 7
    xor ecx, ecx
    cpuid
    bt ebx, 5           ; Bit 5 = AVX2
    jc .avx2_yes
    xor eax, eax
    pop rbx
    ret
.avx2_yes:
    mov eax, 1
    pop rbx
    ret

; ------------------------------------------------------------------------------
; int asm_has_rdtscp_support(void)
; Checks CPUID 0x80000001 for RDTSCP (EDX bit 27)
; ------------------------------------------------------------------------------
asm_has_rdtscp_support:
    push rbx
    mov eax, 0x80000001
    cpuid
    bt edx, 27          ; Bit 27 = RDTSCP
    jc .rdtscp_yes
    xor eax, eax
    pop rbx
    ret
.rdtscp_yes:
    mov eax, 1
    pop rbx
    ret

; ------------------------------------------------------------------------------
; uint16_t asm_avx2_checksum(const void* buffer, size_t len_bytes)
; Ultra-high-throughput dual-accumulator 64-bit pipelined checksum loop
; Windows x64 ABI: RCX = buffer, RDX = len_bytes
; Returns: 16-bit 1's complement sum in AX
; ------------------------------------------------------------------------------
asm_avx2_checksum:
asm_fast_checksum_x64:
    xor eax, eax
    test rcx, rcx
    jz .ret_zero
    test rdx, rdx
    jz .ret_zero

    xor r8, r8          ; Primary 64-bit accumulator
    xor r10, r10         ; Secondary 64-bit accumulator
    clc

.loop_128:
    cmp rdx, 128
    jb .loop_64
    ; Interleaved dual-accumulator 128-byte unroll
    mov r9, [rcx]
    adc r8, r9
    mov r11, [rcx + 8]
    adc r10, r11

    mov r9, [rcx + 16]
    adc r8, r9
    mov r11, [rcx + 24]
    adc r10, r11

    mov r9, [rcx + 32]
    adc r8, r9
    mov r11, [rcx + 40]
    adc r10, r11

    mov r9, [rcx + 48]
    adc r8, r9
    mov r11, [rcx + 56]
    adc r10, r11

    mov r9, [rcx + 64]
    adc r8, r9
    mov r11, [rcx + 72]
    adc r10, r11

    mov r9, [rcx + 80]
    adc r8, r9
    mov r11, [rcx + 88]
    adc r10, r11

    mov r9, [rcx + 96]
    adc r8, r9
    mov r11, [rcx + 104]
    adc r10, r11

    mov r9, [rcx + 112]
    adc r8, r9
    mov r11, [rcx + 120]
    adc r10, r11

    adc r8, 0           ; fold carries
    adc r10, 0

    add rcx, 128
    sub rdx, 128
    jnz .loop_128

.loop_64:
    cmp rdx, 64
    jb .loop_8
    mov r9, [rcx]
    adc r8, r9
    mov r11, [rcx + 8]
    adc r10, r11

    mov r9, [rcx + 16]
    adc r8, r9
    mov r11, [rcx + 24]
    adc r10, r11

    mov r9, [rcx + 32]
    adc r8, r9
    mov r11, [rcx + 40]
    adc r10, r11

    mov r9, [rcx + 48]
    adc r8, r9
    mov r11, [rcx + 56]
    adc r10, r11

    adc r8, 0
    adc r10, 0

    add rcx, 64
    sub rdx, 64

.loop_8:
    cmp rdx, 8
    jb .loop_2
    mov r9, [rcx]
    adc r8, r9
    adc r8, 0
    add rcx, 8
    sub rdx, 8
    jmp .loop_8

.loop_2:
    cmp rdx, 2
    jb .loop_1
    movzx r9d, word [rcx]
    adc r8, r9
    adc r8, 0
    add rcx, 2
    sub rdx, 2
    jmp .loop_2

.loop_1:
    cmp rdx, 1
    jb .fold_accumulators
    movzx r9d, byte [rcx]
    shl r9d, 8
    adc r8, r9
    adc r8, 0

.fold_accumulators:
    ; Combine dual accumulators r8 + r10
    add r8, r10
    adc r8, 0

    ; Fold 64-bit sum down to 32-bit
    mov rax, r8
    shr rax, 32
    add eax, r8d
    adc eax, 0

    ; Fold 32-bit sum down to 16-bit
    mov edx, eax
    shr edx, 16
    add ax, dx
    adc ax, 0

    ; One's complement inversion
    not ax
    movzx eax, ax
    ret

.ret_zero:
    xor eax, eax
    ret

; ------------------------------------------------------------------------------
; uint64_t asm_read_tsc_fast_serialized(void)
; Ultra-low overhead serialization: RDTSCP + LFENCE (20 cycles vs 150 cycles of CPUID)
; Returns: 64-bit cycle count in RAX
; ------------------------------------------------------------------------------
asm_read_tsc_fast_serialized:
    rdtscp              ; Reads TSC into EDX:EAX, processor core into ECX
    lfence              ; Prevents later instructions from executing before RDTSCP finishes
    shl rdx, 32
    or rax, rdx
    ret

asm_read_tsc_fast:
    lfence
    rdtscp
    shl rdx, 32
    or rax, rdx
    ret

asm_read_tsc_serialized:
    push rbx
    xor eax, eax
    cpuid
    rdtsc
    shl rdx, 32
    or rax, rdx
    pop rbx
    ret

; ------------------------------------------------------------------------------
; void asm_avx2_memzero_nt(void* dst, size_t size_in_bytes)
; Clears buffer using 256-bit AVX2 non-temporal streaming stores (vmovntdq)
; RCX = dst, RDX = size_in_bytes
; ------------------------------------------------------------------------------
asm_avx2_memzero_nt:
    test rcx, rcx
    jz .mz_done
    test rdx, rdx
    jz .mz_done

    vpxor ymm0, ymm0, ymm0

.align_32:
    test rcx, 31
    jz .loop_128
    test rdx, rdx
    jz .mz_done
    mov byte [rcx], 0
    inc rcx
    dec rdx
    jmp .align_32

.loop_128:
    cmp rdx, 128
    jb .loop_32
    vmovntdq [rcx], ymm0
    vmovntdq [rcx + 32], ymm0
    vmovntdq [rcx + 64], ymm0
    vmovntdq [rcx + 96], ymm0
    add rcx, 128
    sub rdx, 128
    jmp .loop_128

.loop_32:
    cmp rdx, 32
    jb .tail_bytes
    vmovntdq [rcx], ymm0
    add rcx, 32
    sub rdx, 32
    jmp .loop_32

.tail_bytes:
    test rdx, rdx
    jz .mz_finish
    mov byte [rcx], 0
    inc rcx
    dec rdx
    jmp .tail_bytes

.mz_finish:
    sfence
    vzeroupper
.mz_done:
    ret

; 128-bit fallback
asm_fast_memzero_nt:
    jmp asm_avx2_memzero_nt

; ------------------------------------------------------------------------------
; void asm_avx2_memcpy_nt(void* dst, const void* src, size_t size_in_bytes)
; Fast non-temporal packet memory copy for zero-loss UDP proxy
; RCX = dst, RDX = src, R8 = size_in_bytes
; ------------------------------------------------------------------------------
asm_avx2_memcpy_nt:
    test rcx, rcx
    jz .cp_done
    test rdx, rdx
    jz .cp_done
    test r8, r8
    jz .cp_done

.cp_loop_128:
    cmp r8, 128
    jb .cp_loop_32
    vmovdqu ymm0, [rdx]
    vmovdqu ymm1, [rdx + 32]
    vmovdqu ymm2, [rdx + 64]
    vmovdqu ymm3, [rdx + 96]
    vmovntdq [rcx], ymm0
    vmovntdq [rcx + 32], ymm1
    vmovntdq [rcx + 64], ymm2
    vmovntdq [rcx + 96], ymm3
    add rcx, 128
    add rdx, 128
    sub r8, 128
    jmp .cp_loop_128

.cp_loop_32:
    cmp r8, 32
    jb .cp_tail
    vmovdqu ymm0, [rdx]
    vmovntdq [rcx], ymm0
    add rcx, 32
    add rdx, 32
    sub r8, 32
    jmp .cp_loop_32

.cp_tail:
    test r8, r8
    jz .cp_finish
    mov al, byte [rdx]
    mov byte [rcx], al
    inc rcx
    inc rdx
    dec r8
    jmp .cp_tail

.cp_finish:
    sfence
    vzeroupper
.cp_done:
    ret

; ------------------------------------------------------------------------------
; uint32_t asm_crc32_fast(const void* buffer, size_t len_bytes)
; Hardware accelerated CRC32-C (Castagnoli) using SSE4.2 CRC32 instruction
; RCX = buffer, RDX = len_bytes
; Returns: uint32_t CRC in EAX
; ------------------------------------------------------------------------------
asm_crc32_fast:
    mov eax, 0xFFFFFFFF ; initial seed
    test rcx, rcx
    jz .crc_ret
    test rdx, rdx
    jz .crc_ret

.crc_loop_8:
    cmp rdx, 8
    jb .crc_loop_1
    crc32 rax, qword [rcx]
    add rcx, 8
    sub rdx, 8
    jmp .crc_loop_8

.crc_loop_1:
    test rdx, rdx
    jz .crc_ret
    crc32 eax, byte [rcx]
    inc rcx
    dec rdx
    jmp .crc_loop_1

.crc_ret:
    not eax             ; Final XOR
    ret

; ------------------------------------------------------------------------------
; void asm_calc_ping_stats_simd(
;     const double* rtts,   // RCX
;     size_t count,         // RDX
;     double* outMin,       // R8
;     double* outMax,       // R9
;     double* outAvg,       // [rsp + 40]
;     double* outStdDev     // [rsp + 48]
; )
; Calculates Min, Max, Average and Standard Deviation in a single SIMD pass
; ------------------------------------------------------------------------------
asm_calc_ping_stats_simd:
    push rbp
    mov rbp, rsp
    push rbx
    push rdi

    ; Retrieve 5th and 6th parameters from shadow stack
    mov rax, [rbp + 48] ; outAvg
    mov rbx, [rbp + 56] ; outStdDev

    test rcx, rcx
    jz .stats_done
    test rdx, rdx
    jz .stats_done

    ; Initialize accumulators
    movsd xmm0, [rcx]   ; Min = rtts[0]
    movsd xmm1, [rcx]   ; Max = rtts[0]
    xorpd xmm2, xmm2    ; Sum = 0.0
    xorpd xmm3, xmm3    ; SumSq = 0.0

    xor rdi, rdi        ; index = 0

.stats_loop:
    cmp rdi, rdx
    jae .stats_compute

    movsd xmm4, [rcx + rdi * 8] ; val = rtts[i]

    ; Min = min(Min, val)
    minsd xmm0, xmm4
    ; Max = max(Max, val)
    maxsd xmm1, xmm4
    ; Sum += val
    addsd xmm2, xmm4
    ; SumSq += val * val
    movsd xmm5, xmm4
    mulsd xmm5, xmm5
    addsd xmm3, xmm5

    inc rdi
    jmp .stats_loop

.stats_compute:
    ; Store Min & Max
    test r8, r8
    jz .skip_min
    movsd [r8], xmm0
.skip_min:
    test r9, r9
    jz .skip_max
    movsd [r9], xmm1
.skip_max:

    ; Compute Mean = Sum / count
    cvtsi2sd xmm6, rdx
    divsd xmm2, xmm6     ; xmm2 = Mean

    test rax, rax
    jz .skip_avg
    movsd [rax], xmm2
.skip_avg:

    ; Compute Variance = (SumSq / count) - (Mean * Mean)
    divsd xmm3, xmm6     ; SumSq / count
    movsd xmm7, xmm2
    mulsd xmm7, xmm7     ; Mean * Mean
    subsd xmm3, xmm7     ; Variance

    ; StdDev = sqrt(max(0.0, Variance))
    xorpd xmm6, xmm6
    maxsd xmm3, xmm6
    sqrtsd xmm3, xmm3

    test rbx, rbx
    jz .stats_done
    movsd [rbx], xmm3

.stats_done:
    pop rdi
    pop rbx
    pop rbp
    ret

; ------------------------------------------------------------------------------
; double asm_calc_jitter_rfc3550(double current_jitter, double transit_diff)
; XMM0 = current_jitter, XMM1 = transit_diff
; Formula: J(i) = J(i-1) + (|D| - J(i-1)) / 16.0
; ------------------------------------------------------------------------------
asm_calc_jitter_rfc3550:
    andpd xmm1, [abs_mask_pd]
    subsd xmm1, xmm0
    mulsd xmm1, [jitter_gain_val]
    addsd xmm0, xmm1
    ret

; ------------------------------------------------------------------------------
; void asm_spin_wait_ns(uint64_t target_cycles)
; Sub-microsecond spin-wait loop without context switch
; RCX = target_cycles
; ------------------------------------------------------------------------------
asm_spin_wait_ns:
    rdtsc
    shl rdx, 32
    or rax, rdx
    mov r8, rax         ; start = rdtsc()

.spin_loop:
    pause               ; optimize spin-wait power & pipeline
    rdtsc
    shl rdx, 32
    or rax, rdx
    sub rax, r8         ; elapsed = current - start
    cmp rax, rcx
    jb .spin_loop
    ret

; ------------------------------------------------------------------------------
; uint16_t asm_sse2_checksum(const void* buffer, size_t len_bytes)
; Universal fallback checksum running on ANY x86-64 processor
; ------------------------------------------------------------------------------
global asm_sse2_checksum
asm_sse2_checksum:
    xor eax, eax
    test rcx, rcx
    jz .ret_sse2_zero
    test rdx, rdx
    jz .ret_sse2_zero

    xor r8, r8          ; accumulator
    clc

.loop_sse2_32:
    cmp rdx, 32
    jb .loop_sse2_8
    mov r9, [rcx]
    adc r8, r9
    mov r9, [rcx + 8]
    adc r8, r9
    mov r9, [rcx + 16]
    adc r8, r9
    mov r9, [rcx + 24]
    adc r8, r9
    adc r8, 0

    add rcx, 32
    sub rdx, 32
    jnz .loop_sse2_32

.loop_sse2_8:
    cmp rdx, 8
    jb .loop_sse2_2
    mov r9, [rcx]
    adc r8, r9
    adc r8, 0
    add rcx, 8
    sub rdx, 8
    jmp .loop_sse2_8

.loop_sse2_2:
    cmp rdx, 2
    jb .loop_sse2_1
    movzx r9d, word [rcx]
    adc r8, r9
    adc r8, 0
    add rcx, 2
    sub rdx, 2
    jmp .loop_sse2_2

.loop_sse2_1:
    cmp rdx, 1
    jb .fold_sse2
    movzx r9d, byte [rcx]
    shl r9d, 8
    adc r8, r9
    adc r8, 0

.fold_sse2:
    mov rax, r8
    shr rax, 32
    add eax, r8d
    adc eax, 0

    mov edx, eax
    shr edx, 16
    add ax, dx
    adc ax, 0

    not ax
    movzx eax, ax
    ret

.ret_sse2_zero:
    xor eax, eax
    ret

; ------------------------------------------------------------------------------
; void asm_sse2_memzero_nt(void* dst, size_t size_in_bytes)
; 128-bit SSE2 non-temporal stores running on ANY x86-64 processor
; ------------------------------------------------------------------------------
global asm_sse2_memzero_nt
asm_sse2_memzero_nt:
    test rcx, rcx
    jz .mz_sse2_done
    test rdx, rdx
    jz .mz_sse2_done

    pxor xmm0, xmm0

.align_16:
    test rcx, 15
    jz .loop_sse2_64
    test rdx, rdx
    jz .mz_sse2_done
    mov byte [rcx], 0
    inc rcx
    dec rdx
    jmp .align_16

.loop_sse2_64:
    cmp rdx, 64
    jb .tail_sse2
    movntdq [rcx], xmm0
    movntdq [rcx + 16], xmm0
    movntdq [rcx + 32], xmm0
    movntdq [rcx + 48], xmm0
    add rcx, 64
    sub rdx, 64
    jmp .loop_sse2_64

.tail_sse2:
    cmp rdx, 16
    jb .tail_bytes_sse2
    movntdq [rcx], xmm0
    add rcx, 16
    sub rdx, 16
    jmp .tail_sse2

.tail_bytes_sse2:
    test rdx, rdx
    jz .mz_sse2_finish
    mov byte [rcx], 0
    inc rcx
    dec rdx
    jmp .tail_bytes_sse2

.mz_sse2_finish:
    sfence
.mz_sse2_done:
    ret
