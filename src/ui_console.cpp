#include "ui_console.hpp"
#include "ping_monitor.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <windows.h>

namespace disping {

void UIConsole::InitConsole() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    SetConsoleOutputCP(CP_UTF8);
}

void UIConsole::ClearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

std::string UIConsole::Reset()        { return "\033[0m"; }
std::string UIConsole::Bold()         { return "\033[1m"; }
std::string UIConsole::Red()          { return "\033[31m"; }
std::string UIConsole::Green()        { return "\033[32m"; }
std::string UIConsole::Yellow()       { return "\033[33m"; }
std::string UIConsole::Blue()         { return "\033[34m"; }
std::string UIConsole::Magenta()      { return "\033[35m"; }
std::string UIConsole::Cyan()         { return "\033[36m"; }
std::string UIConsole::White()        { return "\033[37m"; }
std::string UIConsole::BrightGreen()  { return "\033[92m"; }
std::string UIConsole::BrightCyan()   { return "\033[96m"; }
std::string UIConsole::BrightYellow() { return "\033[93m"; }

void UIConsole::PrintBanner() {
    std::cout << BrightCyan() << Bold() << R"(
  ██████╗ ██╗███████╗██████╗ ██╗███╗   ██╗ ██████╗ 
  ██╔══██╗██║██╔════╝██╔══██╗██║████╗  ██║██╔════╝ 
  ██║  ██║██║███████╗██████╔╝██║██╔██╗ ██║██║  ███╗
  ██║  ██║██║╚════██║██╔═══╝ ██║██║╚██╗██║██║   ██║
  ██████╔╝██║███████║██║     ██║██║ ╚████║╚██████╔╝
  ╚═════╝ ╚═╝╚══════╝╚═╝     ╚═╝╚═╝  ╚═══╝ ╚═════╝ 
)" << Reset();
    std::cout << Cyan() << "  Ultimate Network, Latency & FPS Optimization Engine for Competitive Gaming" << Reset() << "\n";
    std::cout << Yellow() << "  Assembly Accelerated | Sub-millisecond Timers | Zero-Jitter TCP Stack" << Reset() << "\n";
    std::cout << "  -----------------------------------------------------------------------\n";
}

void UIConsole::PrintSystemStatus(bool isAdmin, double timerMs, bool netTweaked, bool vpnShieldActive) {
    std::cout << "  [System Status]\n";
    std::cout << "  * Privileges:        " << (isAdmin ? (BrightGreen() + "[ADMINISTRATOR]" + Reset()) : (Red() + "[LIMITED USER - Run as Admin!]" + Reset())) << "\n";
    std::cout << "  * OS Timer Rate:     " << (timerMs <= 1.0 ? BrightGreen() : Yellow()) << std::fixed << std::setprecision(3) << timerMs << " ms" << Reset() << "\n";
    std::cout << "  * Network Tweaks:    " << (netTweaked ? (BrightGreen() + "[ACTIVE - MAXIMUM PRIORITY]" + Reset()) : (Yellow() + "[DEFAULT / NOT APPLIED]" + Reset())) << "\n";
    std::cout << "  * VPN & DPI Shield:  " << (vpnShieldActive ? (BrightGreen() + "[ACTIVE - Zapret/Incy/Happ PROTECTED]" + Reset()) : (White() + "[IDLE - No VPN Running]" + Reset())) << "\n";
    std::cout << "  -----------------------------------------------------------------------\n";
}

void UIConsole::PrintMenu() {
    std::cout << "\n" << Bold() << White() << "  AVAILABLE ACTIONS:" << Reset() << "\n\n";
    std::cout << "   " << BrightGreen() << "[!] 1-CLICK EXTREME GAMING BOOST" << Reset() << " (Network, Adapters, 0.5ms Timer, MMCSS, QoS)\n";
    std::cout << "   " << BrightCyan()  << "[1]" << Reset() << " Apply TCP/IP Network Tweaks (Disable Nagle, ACK delay, Throttling)\n";
    std::cout << "   " << BrightCyan()  << "[2]" << Reset() << " Optimize Network Adapter Hardware (Interrupt Moderation, Buffers, LSO)\n";
    std::cout << "   " << BrightCyan()  << "[3]" << Reset() << " Lock 0.500 ms High-Resolution Timer & Boost MMCSS Gaming Profile\n";
    std::cout << "   " << BrightCyan()  << "[4]" << Reset() << " Detect & Boost Active Games (High Priority & P-Core Affinity Pinning)\n";
    std::cout << "   " << BrightCyan()  << "[5]" << Reset() << " Discover Optimal MTU/MSS (Don't Fragment Binary Search)\n";
    std::cout << "   " << BrightCyan()  << "[6]" << Reset() << " Benchmark & Auto-Select Fastest Gaming DNS Resolver\n";
    std::cout << "   " << BrightCyan()  << "[7]" << Reset() << " Run Microsecond Ping & Jitter Telemetry (Live Graph)\n";
    std::cout << "   " << BrightCyan()  << "[8]" << Reset() << " Clean Standby List & Process Working Sets (Fix Stutters)\n";
    std::cout << "   " << BrightCyan()  << "[9]" << Reset() << " Start Zero-Copy UDP Fast-Relay Proxy\n";
    std::cout << "   " << BrightYellow()<< "[B]" << Reset() << " Create System State Backup\n";
    std::cout << "   " << BrightYellow()<< "[R]" << Reset() << " Restore Original Windows Defaults (Safe Rollback)\n";
    std::cout << "   " << Magenta()     << "[T]" << Reset() << " Run Built-in Automated Verification Tests\n";
    std::cout << "   " << BrightGreen() << "[C]" << Reset() << " Run Real-World Benchmark (Compare BEFORE vs AFTER)\n";
    std::cout << "   " << BrightCyan()  << "[V]" << Reset() << " View VPN & DPI Shield Status (Zapret, Incy, Happ, Wintun)\n";
    std::cout << "   " << BrightCyan()  << "[H]" << Reset() << " Universal Hardware Profiler (Intel Hybrid, AMD X3D, ISA & RAM Tier)\n";
    std::cout << "   " << BrightGreen() << "[S]" << Reset() << " View Live System Status (Registry, Timer, MMCSS, Hardware)\n";
    std::cout << "   " << Red()         << "[Q]" << Reset() << " Exit disping\n\n";
    std::cout << Bold() << "  Select an option [!/1-9/B/R/T/C/V/H/S/Q]: " << Reset() << std::flush;
}

void UIConsole::PrintHeader(const std::string& title) {
    std::cout << "\n" << Bold() << Cyan() << "=== " << title << " ===" << Reset() << "\n\n";
}

void UIConsole::PrintOperationResult(const OperationResult& res) {
    if (res.success) {
        std::cout << BrightGreen() << "[SUCCESS] " << Reset() << res.message << "\n";
    } else {
        std::cout << Red() << "[FAILED]  " << Reset() << res.message << "\n";
    }
    if (!res.details.empty()) {
        std::cout << White() << res.details << Reset() << "\n";
    }
}

static std::string FormatDouble(double val, int precision = 1) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << val;
    return ss.str();
}

void UIConsole::PrintPingTable(const std::vector<PingStats>& stats) {
    std::cout << "\n" << std::left
              << std::setw(22) << "Target Hub"
              << std::setw(12) << "Min (ms)"
              << std::setw(12) << "Avg (ms)"
              << std::setw(12) << "Max (ms)"
              << std::setw(12) << "Jitter (ms)"
              << std::setw(10) << "Loss %"
              << "Sparkline" << "\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& s : stats) {
        std::string spark = PingMonitor::GenerateSparkline(s.rttHistory, 16);
        std::cout << std::left
                  << std::setw(22) << s.target
                  << std::setw(12) << (s.minRttMs > 0.0 ? FormatDouble(s.minRttMs) : "N/A")
                  << std::setw(12) << (s.avgRttMs > 0.0 ? FormatDouble(s.avgRttMs) : "N/A")
                  << std::setw(12) << (s.maxRttMs > 0.0 ? FormatDouble(s.maxRttMs) : "N/A")
                  << std::setw(12) << FormatDouble(s.jitterMs, 2)
                  << std::setw(10) << (FormatDouble(s.lossRate, 1) + "%")
                  << BrightGreen() << spark << Reset() << "\n";
    }
    std::cout << std::string(80, '-') << "\n";
}

void UIConsole::PrintDnsTable(const std::vector<DnsBenchmarkResult>& dnsList) {
    std::cout << "\n" << std::left
              << std::setw(25) << "DNS Provider"
              << std::setw(18) << "Primary IP"
              << std::setw(16) << "Latency (ms)"
              << "Status" << "\n";
    std::cout << std::string(68, '-') << "\n";

    for (const auto& d : dnsList) {
        std::cout << std::left
                  << std::setw(25) << d.name
                  << std::setw(18) << d.primaryIp;
        if (d.reachable) {
            std::cout << std::setw(16) << FormatDouble(d.avgLatencyMs)
                      << BrightGreen() << "[ONLINE - FAST]" << Reset() << "\n";
        } else {
            std::cout << std::setw(16) << "TIMEOUT"
                      << Red() << "[OFFLINE]" << Reset() << "\n";
        }
    }
    std::cout << std::string(68, '-') << "\n";
}

void UIConsole::PrintAdaptersTable(const std::vector<NetworkAdapterInfo>& adapters) {
    std::cout << "\n" << std::left
              << std::setw(20) << "Interface Name"
              << std::setw(18) << "IPv4 Address"
              << std::setw(18) << "MAC Address"
              << std::setw(8)  << "MTU"
              << "Link Speed" << "\n";
    std::cout << std::string(74, '-') << "\n";

    for (const auto& nic : adapters) {
        std::cout << std::left
                  << std::setw(20) << (nic.name.length() > 18 ? (nic.name.substr(0, 15) + "...") : nic.name)
                  << std::setw(18) << (nic.ipv4Address.empty() ? "N/A" : nic.ipv4Address)
                  << std::setw(18) << nic.macAddress
                  << std::setw(8)  << nic.mtu
                  << nic.speedMbps << " Mbps\n";
    }
    std::cout << std::string(74, '-') << "\n";
}

} // namespace disping
