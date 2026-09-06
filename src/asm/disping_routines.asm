; ==============================================================================
; disping_routines.asm - High-Performance x86-64 Assembly Routines for disping
; Architecture: x86-64 (Windows x64 ABI: RCX, RDX, R8, R9, XMM0-3)
; ==============================================================================

default rel

section .text

global asm_fast_checksum_x64
global asm_read_tsc_serialized
global asm_read_tsc_fast
global asm_fast_memzero_nt
global asm_calc_jitter_rfc3550
global asm_has_rdtscp_support

; ------------------------------------------------------------------------------
; uint16_t asm_fast_checksum_x64(const void* buffer, size_t len_bytes)
; Windows x64 ABI: RCX = buffer, RDX = len_bytes
; Returns: uint16_t checksum in AX
; ------------------------------------------------------------------------------
asm_fast_checksum_x64:
    xor eax, eax
    test rcx, rcx
    jz .ret_zero
    test rdx, rdx
    jz .ret_zero

    xor r8, r8          ; 64-bit accumulator
    clc                 ; clear carry flag

.loop_64:
    cmp rdx, 64
    jb .loop_8
    ; Unrolled 64-byte block (8 x 64-bit words)
    mov r9, [rcx]
    adc r8, r9
    mov r9, [rcx + 8]
    adc r8, r9
    mov r9, [rcx + 16]
    adc r8, r9
    mov r9, [rcx + 24]
    adc r8, r9
    mov r9, [rcx + 32]
    adc r8, r9
    mov r9, [rcx + 40]
    adc r8, r9
    mov r9, [rcx + 48]
    adc r8, r9
    mov r9, [rcx + 56]
    adc r8, r9
    adc r8, 0           ; fold intermediate carry

    add rcx, 64
    sub rdx, 64
    jnz .loop_64

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
    jb .fold_sum
    movzx r9d, byte [rcx]
    ; Byte must be padded with 0 as high byte in network byte order
    shl r9d, 8
    adc r8, r9
    adc r8, 0

.fold_sum:
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
; uint64_t asm_read_tsc_serialized(void)
; Uses CPUID + RDTSC for full serialization
; Returns: 64-bit cycle count in RAX
; ------------------------------------------------------------------------------
asm_read_tsc_serialized:
    push rbx
    xor eax, eax
    cpuid               ; Serialize instruction pipeline
    rdtsc               ; Read time-stamp counter into EDX:EAX
    shl rdx, 32
    or rax, rdx         ; Combine into 64-bit RAX
    pop rbx
    ret

; ------------------------------------------------------------------------------
; uint64_t asm_read_tsc_fast(void)
; Uses RDTSCP with instruction fence
; Returns: 64-bit cycle count in RAX
; ------------------------------------------------------------------------------
asm_read_tsc_fast:
    lfence
    rdtscp              ; Reads TSC and processor ID into EDX:EAX:ECX
    shl rdx, 32
    or rax, rdx
    ret

; ------------------------------------------------------------------------------
; int asm_has_rdtscp_support(void)
; Checks CPUID extended function 0x80000001 for RDTSCP (EDX bit 27)
; Returns: 1 if supported, 0 otherwise
; ------------------------------------------------------------------------------
asm_has_rdtscp_support:
    push rbx
    mov eax, 0x80000001
    cpuid
    bt edx, 27          ; Bit 27 of EDX is RDTSCP
    jc .supported
    xor eax, eax
    pop rbx
    ret

.supported:
    mov eax, 1
    pop rbx
    ret

; ------------------------------------------------------------------------------
; void asm_fast_memzero_nt(void* dst, size_t size_in_bytes)
; RCX = dst (must ideally be 16-byte aligned), RDX = size_in_bytes
; Uses non-temporal stores (movntdq) to bypass cache hierarchy for packet buffers
; ------------------------------------------------------------------------------
asm_fast_memzero_nt:
    test rcx, rcx
    jz .mem_done
    test rdx, rdx
    jz .mem_done

    pxor xmm0, xmm0     ; 128-bit zero vector

    ; Align to 16-byte boundary if needed
.align_loop:
    test rcx, 15
    jz .aligned_start
    test rdx, rdx
    jz .mem_done
    mov byte [rcx], 0
    inc rcx
    dec rdx
    jmp .align_loop

.aligned_start:
    ; Process 64 bytes per iteration with streaming stores
.nt_loop64:
    cmp rdx, 64
    jb .nt_loop16
    movntdq [rcx], xmm0
    movntdq [rcx + 16], xmm0
    movntdq [rcx + 32], xmm0
    movntdq [rcx + 48], xmm0
    add rcx, 64
    sub rdx, 64
    jmp .nt_loop64

.nt_loop16:
    cmp rdx, 16
    jb .nt_tail
    movntdq [rcx], xmm0
    add rcx, 16
    sub rdx, 16
    jmp .nt_loop16

.nt_tail:
    test rdx, rdx
    jz .fence_done
    mov byte [rcx], 0
    inc rcx
    dec rdx
    jmp .nt_tail

.fence_done:
    sfence              ; Ensure non-temporal stores are globally visible
.mem_done:
    ret

; ------------------------------------------------------------------------------
; double asm_calc_jitter_rfc3550(double current_jitter, double transit_diff)
; Windows x64 ABI: XMM0 = current_jitter, XMM1 = transit_diff
; Formula: J(i) = J(i-1) + (|D(i-1, i)| - J(i-1)) / 16.0
; Returns: new jitter in XMM0
; ------------------------------------------------------------------------------
section .data
align 16
abs_mask:
    dq 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF
jitter_gain:
    dq 0.0625, 0.0625   ; 1.0 / 16.0

section .text
asm_calc_jitter_rfc3550:
    ; Absolute value of transit_diff: XMM1 = |transit_diff|
    andpd xmm1, [abs_mask]
    ; XMM1 = |D| - J(i-1)
    subsd xmm1, xmm0
    ; XMM1 = (|D| - J(i-1)) * (1.0 / 16.0)
    mulsd xmm1, [jitter_gain]
    ; XMM0 = J(i-1) + result
    addsd xmm0, xmm1
    ret
