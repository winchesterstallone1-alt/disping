cmake_minimum_required(VERSION 3.20)
project(disping VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find NASM executable
find_program(NASM_EXECUTABLE nasm HINTS "C:/msys64/ucrt64/bin")
if(NOT NASM_EXECUTABLE)
    set(NASM_EXECUTABLE "C:/msys64/ucrt64/bin/nasm.exe")
endif()

message(STATUS "Using NASM: ${NASM_EXECUTABLE}")

# Assemble NASM routine
set(ASM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/asm/disping_routines.asm")
set(ASM_OBJ "${CMAKE_CURRENT_BINARY_DIR}/disping_routines.obj")

add_custom_command(
    OUTPUT ${ASM_OBJ}
    COMMAND ${NASM_EXECUTABLE} -f win64 ${ASM_SRC} -o ${ASM_OBJ}
    DEPENDS ${ASM_SRC}
    COMMENT "Assembling ${ASM_SRC} with NASM"
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(COMMON_SOURCES
    src/registry_util.cpp
    src/network_optimizer.cpp
    src/adapter_optimizer.cpp
    src/qos_optimizer.cpp
    src/mtu_optimizer.cpp
    src/dns_optimizer.cpp
    src/system_latency_optimizer.cpp
    src/process_optimizer.cpp
    src/memory_optimizer.cpp
    src/ping_monitor.cpp
    src/udp_proxy.cpp
    src/backup_manager.cpp
    src/ui_console.cpp
    src/benchmark_runner.cpp
    src/vpn_guard.cpp
)

# Main Executable
add_executable(disping
    src/main.cpp
    ${COMMON_SOURCES}
    ${ASM_OBJ}
)

target_link_libraries(disping PRIVATE
    ws2_32
    iphlpapi
    winmm
    ntdll
    avrt
    psapi
    advapi32
)

target_compile_options(disping PRIVATE -O3 -Wall -Wextra -static -static-libgcc -static-libstdc++)
target_link_options(disping PRIVATE -static -static-libgcc -static-libstdc++)

# Tests Executable
add_executable(disping_tests
    tests/test_main.cpp
    ${COMMON_SOURCES}
    ${ASM_OBJ}
)

target_link_libraries(disping_tests PRIVATE
    ws2_32
    iphlpapi
    winmm
    ntdll
    avrt
    psapi
    advapi32
)

target_compile_options(disping_tests PRIVATE -O3 -Wall -Wextra -static -static-libgcc -static-libstdc++)
target_link_options(disping_tests PRIVATE -static -static-libgcc -static-libstdc++)
