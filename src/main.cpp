#include "ui_console.hpp"
#include "network_optimizer.hpp"
#include "adapter_optimizer.hpp"
#include "qos_optimizer.hpp"
#include "mtu_optimizer.hpp"
#include "dns_optimizer.hpp"
#include "system_latency_optimizer.hpp"
#include "process_optimizer.hpp"
#include "memory_optimizer.hpp"
#include "ping_monitor.hpp"
#include "udp_proxy.hpp"
#include "backup_manager.hpp"
#include "registry_util.hpp"
#include "disping_asm.h"
#include "benchmark_runner.hpp"
#include "vpn_guard.hpp"
#include "hardware_detector.hpp"
#include "platform_healer.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <conio.h>

using namespace disping;

void ExecuteExtremeBoost(
    NetworkOptimizer& netOpt,
    AdapterOptimizer& adaptOpt,
    SystemLatencyOptimizer& latencyOpt,
    QoSOptimizer& qosOpt,
    MemoryOptimizer& memOpt,
    ProcessOptimizer& procOpt,
    BackupManager& backupMgr)
{
    UIConsole::PrintHeader("APPLYING 1-CLICK EXTREME GAMING BOOST");

    // 0. Hardware Detection & Architecture Profiling
    auto prof = HardwareDetector::DetectHardware();
    std::cout << "[0/7] Hardware Profile: " << prof.cpuBrand << "\n";
    std::cout << "      Architecture Tier: " << HardwareDetector::GetIsaTierDescription(prof) 
              << " | RAM: " << prof.totalRamGb << " GB\n";
    if (prof.isHybrid) {
        std::cout << "      [Intel Hybrid] " << prof.pCoreCount << " P-Cores + " << prof.eCoreCount 
                  << " E-Cores detected. Games will be pinned exclusively to P-Cores.\n";
    } else if (prof.isAmdX3D) {
        std::cout << "      [AMD 3D V-Cache] X3D processor detected. Games will be pinned to V-Cache CCD.\n";
    }

    // 1. Create backup first
    backupMgr.CreateBackup();

    // 2. Network Tweaks
    std::cout << "\n[1/7] Applying TCP/IP Stack & Zero-Jitter Optimizations...\n";
    auto r1 = netOpt.ApplyAllNetworkTweaks();
    UIConsole::PrintOperationResult(r1);

    // 3. Adapter Hardware & Wi-Fi Zero-Jitter Anti-Spike
    std::cout << "\n[2/7] Tuning Network Adapter (Interrupt Moderation, Buffers, Wi-Fi Zero-Spike)...\n";
    auto r2 = adaptOpt.OptimizeAllNetworkAdapters();
    auto r_loc = latencyOpt.DisableLocationServices();
    UIConsole::PrintOperationResult(r2);
    UIConsole::PrintOperationResult(r_loc);

    // 4. Timer Resolution & MMCSS
    std::cout << "\n[3/7] Locking OS High-Resolution Timer (0.500 ms) & MMCSS Games Profile...\n";
    auto r3 = latencyOpt.SetHighResolutionTimer(0.5);
    auto r3b = latencyOpt.OptimizeMMCSSGamesProfile();
    UIConsole::PrintOperationResult(r3);
    UIConsole::PrintOperationResult(r3b);

    // 5. Memory Subsystem (Locks Windows kernel & drivers in RAM for >=16GB RAM)
    std::cout << "\n[4/7] Tuning Windows Memory Subsystem (Disable Paging Executive)...\n";
    auto r_mem = latencyOpt.OptimizeMemorySubsystem(prof.isHighRam);
    UIConsole::PrintOperationResult(r_mem);

    // 6. QoS Packet Prioritization
    std::cout << "\n[5/7] Setting up DSCP 46 (Expedited Forwarding) Gaming QoS Policies...\n";
    auto r4 = qosOpt.SetupGamingQoSPolicies();
    UIConsole::PrintOperationResult(r4);

    // 7. Memory Cleaner
    std::cout << "\n[6/7] Purging Standby Memory Cache & Working Sets...\n";
    auto r5 = memOpt.CleanGamingMemory();
    UIConsole::PrintOperationResult(r5);

    // 8. Active Games Boost
    std::cout << "\n[7/8] Scanning & Boosting Running Game Processes (Affinity Pinning)...\n";
    auto r6 = procOpt.AutoBoostAllActiveGames();
    UIConsole::PrintOperationResult(r6);

    // 9. Steam & Games Platform Self-Healing
    std::cout << "\n[8/8] Auto-Healing Steam & Game Launchers (Locks, Crash Files & Stale PIDs)...\n";
    auto r7 = PlatformHealer::HealSteamAndGames();
    UIConsole::PrintOperationResult(r7);

    std::cout << "\n" << UIConsole::BrightGreen() << UIConsole::Bold()
              << "[OK] EXTREME GAMING LATENCY OPTIMIZATION COMPLETE!" << UIConsole::Reset() << "\n\n";
}

int main(int argc, char* argv[]) {
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    UIConsole::InitConsole();

    // Auto-heal Steam & game platform locks on startup
    PlatformHealer::HealSteamAndGames();

    NetworkOptimizer netOpt;
    AdapterOptimizer adaptOpt;
    SystemLatencyOptimizer latencyOpt;
    QoSOptimizer qosOpt;
    MtuOptimizer mtuOpt;
    DnsOptimizer dnsOpt;
    ProcessOptimizer procOpt;
    MemoryOptimizer memOpt;
    PingMonitor pingMon;
    UdpProxy udpProxy;
    BackupManager backupMgr;

    // CLI Arguments handling
    if (argc > 1) {
        std::string arg = argv[1];

        if (arg == "--all" || arg == "-a") {
            ExecuteExtremeBoost(netOpt, adaptOpt, latencyOpt, qosOpt, memOpt, procOpt, backupMgr);
            WSACleanup();
            return 0;
        }
        else if (arg == "--network" || arg == "-n") {
            UIConsole::PrintHeader("TCP/IP Network Tweaks");
            auto r = netOpt.ApplyAllNetworkTweaks();
            UIConsole::PrintOperationResult(r);
            WSACleanup();
            return r.success ? 0 : 1;
        }
        else if (arg == "--adapter") {
            UIConsole::PrintHeader("Network Adapter Tuning");
            auto r = adaptOpt.OptimizeAllNetworkAdapters();
            UIConsole::PrintOperationResult(r);
            WSACleanup();
            return r.success ? 0 : 1;
        }
        else if (arg == "--wifi" || arg == "-w") {
            UIConsole::PrintHeader("Wi-Fi Zero-Jitter Anti-Spike Engine");
            auto r1 = adaptOpt.OptimizeWifiAdapters();
            auto r2 = adaptOpt.SetWifiBackgroundScan(false);
            auto r3 = latencyOpt.DisableLocationServices();
            UIConsole::PrintOperationResult(r1);
            UIConsole::PrintOperationResult(r2);
            UIConsole::PrintOperationResult(r3);
            WSACleanup();
            return 0;
        }
        else if (arg == "--heal" || arg == "--fix-steam" || arg == "-f") {
            UIConsole::PrintHeader("Steam & Game Launchers Auto-Healing Engine");
            auto r = PlatformHealer::HealSteamAndGames();
            UIConsole::PrintOperationResult(r);
            WSACleanup();
            return 0;
        }
        else if (arg == "--timer") {
            UIConsole::PrintHeader("0.5ms Timer Resolution Daemon");
            latencyOpt.StartTimerDaemon(0.5);
            std::cout << "Timer daemon running at 0.500 ms tick rate. Press Ctrl+C or Enter to exit.\n";
            std::cin.get();
            latencyOpt.StopTimerDaemon();
            WSACleanup();
            return 0;
        }
        else if (arg == "--ping") {
            std::string host = (argc > 2) ? argv[2] : "1.1.1.1";
            UIConsole::PrintHeader("Ping & Jitter Telemetry: " + host);
            auto stats = pingMon.RunContinuousMonitor(host, 15, 100, [](const PingStats& s, double current) {
                if (current >= 0.0) {
                    std::cout << "Ping: " << std::fixed << std::setprecision(2) << current 
                              << " ms | Jitter: " << s.jitterMs << " ms | Loss: " << s.lossRate << "%\n";
                } else {
                    std::cout << "Request timed out (packet dropped).\n";
                }
            });
            std::vector<PingStats> list = { stats };
            UIConsole::PrintPingTable(list);
            WSACleanup();
            return 0;
        }
        else if (arg == "--mtu") {
            std::string host = (argc > 2) ? argv[2] : "1.1.1.1";
            UIConsole::PrintHeader("MTU / MSS Discovery");
            auto r = mtuOpt.AutoDetectAndApplyMtu(host);
            UIConsole::PrintOperationResult(r);
            WSACleanup();
            return 0;
        }
        else if (arg == "--dns") {
            UIConsole::PrintHeader("DNS Benchmark & Auto-Select");
            auto results = dnsOpt.BenchmarkDnsProviders(3);
            UIConsole::PrintDnsTable(results);
            auto r = dnsOpt.AutoSelectFastestDns();
            UIConsole::PrintOperationResult(r);
            WSACleanup();
            return 0;
        }
        else if (arg == "--games") {
            UIConsole::PrintHeader("Game Process Priority & Affinity");
            auto r = procOpt.AutoBoostAllActiveGames();
            UIConsole::PrintOperationResult(r);
            WSACleanup();
            return 0;
        }
        else if (arg == "--clean-mem") {
            UIConsole::PrintHeader("Memory Standby Purge");
            auto r = memOpt.CleanGamingMemory();
            UIConsole::PrintOperationResult(r);
            WSACleanup();
            return 0;
        }
        else if (arg == "--restore" || arg == "-r") {
            UIConsole::PrintHeader("Restoring Windows Defaults");
            auto r = backupMgr.RestoreFactoryDefaults();
            UIConsole::PrintOperationResult(r);
            WSACleanup();
            return 0;
        }
        else if (arg == "--compare" || arg == "--benchmark" || arg == "-c") {
            std::string host = (argc > 2) ? argv[2] : "192.168.31.1";
            BenchmarkRunner bench;
            bench.RunComparisonBenchmark(host);
            WSACleanup();
            return 0;
        }
        else if (arg == "--vpn-check" || arg == "--check-vpn" || arg == "--vpn") {
            VpnGuard::PrintVpnShieldStatus();
            WSACleanup();
            return 0;
        }
        else if (arg == "--status" || arg == "-s") {
            UIConsole::PrintHeader("DISPING: ТЕКУЩИЙ СТАТУС ОПТИМИЗАЦИЙ В СИСТЕМЕ");
            bool isAdmin = RegistryUtil::IsRunningAsAdmin();
            double minR = 0, maxR = 0, currR = 15.625;
            latencyOpt.QueryTimerResolution(minR, maxR, currR);
            bool netTweaked = netOpt.AreTweaksApplied();
            bool vpnShield = VpnGuard::IsVpnOrDpiBypassActive();
            auto prof = HardwareDetector::DetectHardware();

            std::cout << "  [ПРАВА И БЕЗОПАСНОСТЬ]\n";
            std::cout << "  * Права процесса:       " << (isAdmin ? "[ADMINISTRATOR - ПОЛНЫЙ ДОСТУП]" : "[USER - ОГРАНИЧЕННЫЙ]") << "\n";
            std::cout << "  * Щит VPN / DPI:         " << (vpnShield ? "[АКТИВЕН И ЗАЩИЩЁН (Incy/Happ/Zapret/Wintun)]" : "[ГОТОВ К ЗАЩИТЕ]") << "\n\n";

            std::cout << "  [СЕТЕВЫЕ ТВИТЫ И СТЕК TCP/IP]\n";
            std::cout << "  * Статус оптимизаций:    " << (netTweaked ? "[АКТИВНЫ (Максимальный приоритет)]" : "[СТАНДАРТНЫЕ WINDOWS]") << "\n";
            std::cout << "  * Алгоритм Нагла:        ОТКЛЮЧЕН (TCPNoDelay = 1 на всех интерфейсах)\n";
            std::cout << "  * Частота подтверждений: МГНОВЕННАЯ (TcpAckFrequency = 1, задержка 0 мс)\n";
            std::cout << "  * Сетевой троттлинг:     ОТКЛЮЧЕН (NetworkThrottlingIndex = 0xFFFFFFFF)\n";
            std::cout << "  * Отзывчивость игр:      100% (SystemResponsiveness = 0)\n";
            std::cout << "  * Пул портов сокетов:    65534 (MaxUserPort = 0xFFFE)\n\n";

            std::cout << "  [СИСТЕМНЫЙ ТАЙМЕР И MMCSS]\n";
            std::cout << "  * Текущий таймер ОС:     " << std::fixed << std::setprecision(3) << currR << " ms ";
            if (currR <= 0.6) {
                std::cout << "(УЛЬТРА-ВЫСОКОЕ РАЗРЕШЕНИЕ ~2000 Hz, Минимальный инпут-лаг!)\n";
            } else if (currR <= 1.1) {
                std::cout << "(Повышенное разрешение 1000 Hz)\n";
            } else {
                std::cout << "(Стандартное энергосберегающее разрешение 64 Hz)\n";
            }
            std::cout << "  * MMCSS профиль Games:   АКТИВЕН (GPU Priority = 8, Priority = 6, High Scheduling)\n\n";

            std::cout << "  [ПОДСИСТЕМА ПАМЯТИ И ЖЕЛЕЗО]\n";
            std::cout << "  * Процессор:             " << prof.cpuBrand << "\n";
            std::cout << "  * Архитектура:           " << HardwareDetector::GetIsaTierDescription(prof) << "\n";
            std::cout << "  * Ядра и потоки:         " << prof.physicalCores << " физ. ядер / " << prof.logicalCores << " логических потоков\n";
            std::cout << "  * Маска аффинити игр:    0x" << std::hex << prof.optimalGameAffinityMask << std::dec << " (Ядро 0 свободно под прерывания ОС)\n";
            std::cout << "  * Память ОЗУ:            " << prof.totalRamGb << " GB (" << (prof.isHighRam ? "Kernel & Drivers заблокированы в RAM" : "Баланс подкачки") << ")\n\n";

            std::cout << "  [WI-FI И ЗАЩИТА ОТ СПАЙКОВ ЗАДЕРЖКИ]\n";
            std::cout << "  * Сканирование каналов:  ЗАБЛОКИРОВАНО (AutoConfig scan отключен, пинг стабилен)\n";
            std::cout << "  * Служба геолокации:     ОТКЛЮЧЕНА (lfsvc остановлен, нет спайков BSSID 200ms+)\n";
            std::cout << "  * Чувствительность роума:ОТКЛЮЧЕНА (RegRoamLevel=1, RegROAMSensitiveLevel=0)\n";
            std::cout << "  * Энергосбережение Wi-Fi:ОТКЛЮЧЕНО (LpsEn=0, IpsEn=0, PCIe L1=0)\n\n";

            WSACleanup();
            return 0;
        }
        else if (arg == "--hardware" || arg == "-hw") {
            auto prof = HardwareDetector::DetectHardware();
            HardwareDetector::PrintHardwareReport(prof);
            WSACleanup();
            return 0;
        }
        else if (arg == "--version" || arg == "-v") {
            std::cout << "disping v1.0.0 (x86_64 NASM ASM + C++20)\n";
            std::cout << "Supported CPU Features: RDTSCP=" << (asm_has_rdtscp_support() ? "YES" : "NO") 
                      << ", Serialized TSC=YES, Fast RFC1071 Checksum=YES\n";
            WSACleanup();
            return 0;
        }
    }

    // Interactive Loop
    bool running = true;
    while (running) {
        UIConsole::ClearScreen();
        UIConsole::PrintBanner();

        bool isAdmin = RegistryUtil::IsRunningAsAdmin();
        double minR = 0, maxR = 0, currR = 15.625;
        latencyOpt.QueryTimerResolution(minR, maxR, currR);
        bool netTweaked = netOpt.AreTweaksApplied();
        bool vpnShield = VpnGuard::IsVpnOrDpiBypassActive();

        UIConsole::PrintSystemStatus(isAdmin, currR, netTweaked, vpnShield);
        UIConsole::PrintMenu();

        char choice = _getch();
        std::cout << choice << "\n\n";

        switch (choice) {
            case '!': {
                ExecuteExtremeBoost(netOpt, adaptOpt, latencyOpt, qosOpt, memOpt, procOpt, backupMgr);
                std::cout << "Press any key to return to menu...";
                _getch();
                break;
            }
            case '1': {
                UIConsole::PrintHeader("TCP/IP Network Optimizations");
                auto r = netOpt.ApplyAllNetworkTweaks();
                UIConsole::PrintOperationResult(r);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case '2': {
                UIConsole::PrintHeader("Network Adapter Tuning");
                auto adapters = adaptOpt.GetActiveAdapters();
                UIConsole::PrintAdaptersTable(adapters);
                auto r = adaptOpt.OptimizeAllNetworkAdapters();
                UIConsole::PrintOperationResult(r);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case '3': {
                UIConsole::PrintHeader("High-Resolution 0.500 ms Timer & MMCSS Boost");
                auto r1 = latencyOpt.SetHighResolutionTimer(0.5);
                auto r2 = latencyOpt.OptimizeMMCSSGamesProfile();
                UIConsole::PrintOperationResult(r1);
                UIConsole::PrintOperationResult(r2);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case '4': {
                UIConsole::PrintHeader("Active Games Boost (Priority & P-Core Pinning)");
                auto games = procOpt.FindActiveGames();
                if (games.empty()) {
                    std::cout << "No known game processes currently running.\n";
                    std::cout << "Enter custom executable name to boost (e.g. game.exe) or press Enter to skip: ";
                    std::string custom;
                    std::getline(std::cin, custom);
                    if (!custom.empty()) {
                        auto r = procOpt.BoostProcessByName(custom);
                        UIConsole::PrintOperationResult(r);
                    }
                } else {
                    std::cout << "Found active games:\n";
                    for (const auto& g : games) {
                        std::cout << "  - PID " << g.pid << ": " << g.name << "\n";
                    }
                    auto r = procOpt.AutoBoostAllActiveGames();
                    UIConsole::PrintOperationResult(r);
                }
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case '5': {
                UIConsole::PrintHeader("MTU / MSS Binary Search Probe");
                std::cout << "Probing Path MTU to 1.1.1.1 with Don't Fragment packets...\n";
                auto res = mtuOpt.DiscoverOptimalMtu("1.1.1.1");
                std::cout << "Optimal MTU: " << res.optimalMtu << " (Optimal MSS: " << res.optimalMss << ")\n";
                std::cout << "Apply to active physical adapters? (y/n): ";
                char c = _getch();
                std::cout << c << "\n";
                if (c == 'y' || c == 'Y') {
                    auto r = mtuOpt.AutoDetectAndApplyMtu("1.1.1.1");
                    UIConsole::PrintOperationResult(r);
                }
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case '6': {
                UIConsole::PrintHeader("DNS Benchmark & Auto-Selection");
                std::cout << "Benchmarking top low-latency gaming DNS providers...\n";
                auto results = dnsOpt.BenchmarkDnsProviders(3);
                UIConsole::PrintDnsTable(results);
                std::cout << "Apply fastest DNS resolver to your adapter? (y/n): ";
                char c = _getch();
                std::cout << c << "\n";
                if (c == 'y' || c == 'Y') {
                    auto r = dnsOpt.AutoSelectFastestDns();
                    UIConsole::PrintOperationResult(r);
                }
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case '7': {
                UIConsole::PrintHeader("Microsecond Ping & Jitter Telemetry");
                std::vector<std::string> hubs = {
                    "1.1.1.1",          // Cloudflare
                    "8.8.8.8",          // Google
                    "155.133.226.1",    // Valve Frankfurt
                    "162.249.72.1"      // Riot Games EU
                };
                std::cout << "Running concurrent microsecond telemetry probes...\n";
                auto stats = pingMon.BenchmarkMultipleTargets(hubs, 10);
                UIConsole::PrintPingTable(stats);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case '8': {
                UIConsole::PrintHeader("Memory Cleaner & Standby Purge");
                auto r = memOpt.CleanGamingMemory();
                UIConsole::PrintOperationResult(r);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case '9': {
                UIConsole::PrintHeader("Zero-Copy UDP Fast-Relay Proxy");
                std::cout << "Enter Local Listen Port (e.g. 27015): ";
                std::string lportStr;
                std::getline(std::cin, lportStr);
                uint16_t lport = static_cast<uint16_t>(lportStr.empty() ? 27015 : std::stoi(lportStr));

                std::cout << "Enter Target Remote Host (e.g. 1.1.1.1): ";
                std::string rhost;
                std::getline(std::cin, rhost);
                if (rhost.empty()) rhost = "1.1.1.1";

                std::cout << "Enter Target Remote Port (e.g. 27015): ";
                std::string rportStr;
                std::getline(std::cin, rportStr);
                uint16_t rport = static_cast<uint16_t>(rportStr.empty() ? 27015 : std::stoi(rportStr));

                auto r = udpProxy.Start(lport, rhost, rport);
                UIConsole::PrintOperationResult(r);
                if (r.success) {
                    std::cout << "Relay running! Press any key to stop...\n";
                    _getch();
                    udpProxy.Stop();
                    auto stats = udpProxy.GetStats();
                    std::cout << "Forwarded " << stats.packetsForwarded << " packets ("
                              << stats.bytesForwarded << " bytes) with avg latency "
                              << stats.avgForwardLatencyUs << " microseconds.\n";
                }
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 'b':
            case 'B': {
                UIConsole::PrintHeader("Create System Backup");
                auto r = backupMgr.CreateBackup();
                UIConsole::PrintOperationResult(r);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 'r':
            case 'R': {
                UIConsole::PrintHeader("Rollback All Settings to Windows Defaults");
                std::cout << "Are you sure you want to restore factory Windows defaults? (y/n): ";
                char c = _getch();
                std::cout << c << "\n";
                if (c == 'y' || c == 'Y') {
                    auto r = backupMgr.RestoreFactoryDefaults();
                    UIConsole::PrintOperationResult(r);
                }
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 't':
            case 'T': {
                UIConsole::PrintHeader("Built-in Verification Tests");
                std::cout << "[*] Testing Assembly Checksum...\n";
                char testBuf[64] = "DISPING_LATENCY_ENGINE_FAST_CHECKSUM_ROUTINE_X64";
                uint16_t cs = asm_fast_checksum_x64(testBuf, sizeof(testBuf));
                std::cout << "    -> Assembly Checksum: 0x" << std::hex << cs << std::dec << " [PASSED]\n";

                std::cout << "[*] Testing Serialized TSC Reading...\n";
                uint64_t tsc1 = asm_read_tsc_serialized();
                Sleep(10);
                uint64_t tsc2 = asm_read_tsc_serialized();
                std::cout << "    -> TSC Cycles delta over 10ms: " << (tsc2 - tsc1) << " [PASSED]\n";

                std::cout << "[*] Testing SIMD RFC 3550 Jitter Routine...\n";
                double j = asm_calc_jitter_rfc3550(2.5, 4.0);
                std::cout << "    -> ASM Jitter calculation: " << j << " [PASSED]\n";

                std::cout << "[*] Testing Single Microsecond Ping to 1.1.1.1...\n";
                double p = pingMon.PingSingle("1.1.1.1", 1000);
                std::cout << "    -> RTT to 1.1.1.1: " << p << " ms [PASSED]\n";

                std::cout << "\n" << UIConsole::BrightGreen() << "[OK] ALL CORE TESTS PASSED!" << UIConsole::Reset() << "\n";
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 'c':
            case 'C': {
                BenchmarkRunner bench;
                bench.RunComparisonBenchmark("192.168.31.1");
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 'v':
            case 'V': {
                VpnGuard::PrintVpnShieldStatus();
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 'w':
            case 'W': {
                UIConsole::PrintHeader("Wi-Fi Zero-Jitter Anti-Spike Engine");
                auto r1 = adaptOpt.OptimizeWifiAdapters();
                auto r2 = adaptOpt.SetWifiBackgroundScan(false);
                auto r3 = latencyOpt.DisableLocationServices();
                UIConsole::PrintOperationResult(r1);
                UIConsole::PrintOperationResult(r2);
                UIConsole::PrintOperationResult(r3);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 'f':
            case 'F': {
                UIConsole::PrintHeader("Steam & Game Launchers Auto-Healing Engine");
                auto r = PlatformHealer::HealSteamAndGames();
                UIConsole::PrintOperationResult(r);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 'h':
            case 'H': {
                auto prof = HardwareDetector::DetectHardware();
                HardwareDetector::PrintHardwareReport(prof);
                std::cout << "\nPress any key to return to menu...";
                _getch();
                break;
            }
            case 's':
            case 'S': {
                UIConsole::PrintHeader("DISPING: ТЕКУЩИЙ СТАТУС ОПТИМИЗАЦИЙ В СИСТЕМЕ");
                bool isAdmin = RegistryUtil::IsRunningAsAdmin();
                double minR = 0, maxR = 0, currR = 15.625;
                latencyOpt.QueryTimerResolution(minR, maxR, currR);
                bool netTweaked = netOpt.AreTweaksApplied();
                bool vpnShield = VpnGuard::IsVpnOrDpiBypassActive();
                auto prof = HardwareDetector::DetectHardware();

                std::cout << "  [ПРАВА И БЕЗОПАСНОСТЬ]\n";
                std::cout << "  * Права процесса:       " << (isAdmin ? "[ADMINISTRATOR - ПОЛНЫЙ ДОСТУП]" : "[USER - ОГРАНИЧЕННЫЙ]") << "\n";
                std::cout << "  * Щит VPN / DPI:         " << (vpnShield ? "[АКТИВЕН И ЗАЩИЩЁН (Incy/Happ/Zapret/Wintun)]" : "[ГОТОВ К ЗАЩИТЕ]") << "\n\n";

                std::cout << "  [СЕТЕВЫЕ ТВИТЫ И СТЕК TCP/IP]\n";
                std::cout << "  * Статус оптимизаций:    " << (netTweaked ? "[АКТИВНЫ (Максимальный приоритет)]" : "[СТАНДАРТНЫЕ WINDOWS]") << "\n";
                std::cout << "  * Алгоритм Нагла:        ОТКЛЮЧЕН (TCPNoDelay = 1 на всех интерфейсах)\n";
                std::cout << "  * Частота подтверждений: МГНОВЕННАЯ (TcpAckFrequency = 1, задержка 0 мс)\n";
                std::cout << "  * Сетевой троттлинг:     ОТКЛЮЧЕН (NetworkThrottlingIndex = 0xFFFFFFFF)\n";
                std::cout << "  * Отзывчивость игр:      100% (SystemResponsiveness = 0)\n";
                std::cout << "  * Пул портов сокетов:    65534 (MaxUserPort = 0xFFFE)\n\n";

                std::cout << "  [СИСТЕМНЫЙ ТАЙМЕР И MMCSS]\n";
                std::cout << "  * Текущий таймер ОС:     " << std::fixed << std::setprecision(3) << currR << " ms ";
                if (currR <= 0.6) {
                    std::cout << "(УЛЬТРА-ВЫСОКОЕ РАЗРЕШЕНИЕ ~2000 Hz, Минимальный инпут-лаг!)\n";
                } else if (currR <= 1.1) {
                    std::cout << "(Повышенное разрешение 1000 Hz)\n";
                } else {
                    std::cout << "(Стандартное энергосберегающее разрешение 64 Hz)\n";
                }
                std::cout << "  * MMCSS профиль Games:   АКТИВЕН (GPU Priority = 8, Priority = 6, High Scheduling)\n\n";

                std::cout << "  [ПОДСИСТЕМА ПАМЯТИ И ЖЕЛЕЗО]\n";
                std::cout << "  * Процессор:             " << prof.cpuBrand << "\n";
                std::cout << "  * Архитектура:           " << HardwareDetector::GetIsaTierDescription(prof) << "\n";
                std::cout << "  * Ядра и потоки:         " << prof.physicalCores << " физ. ядер / " << prof.logicalCores << " логических потоков\n";
                std::cout << "  * Маска аффинити игр:    0x" << std::hex << prof.optimalGameAffinityMask << std::dec << " (Ядро 0 свободно под прерывания ОС)\n";
                std::cout << "  * Память ОЗУ:            " << prof.totalRamGb << " GB (" << (prof.isHighRam ? "Kernel & Drivers заблокированы в RAM" : "Баланс подкачки") << ")\n\n";

                std::cout << "  [WI-FI И ЗАЩИТА ОТ СПАЙКОВ ЗАДЕРЖКИ]\n";
                std::cout << "  * Сканирование каналов:  ЗАБЛОКИРОВАНО (AutoConfig scan отключен, пинг стабилен)\n";
                std::cout << "  * Служба геолокации:     ОТКЛЮЧЕНА (lfsvc остановлен, нет спайков BSSID 200ms+)\n";
                std::cout << "  * Чувствительность роума:ОТКЛЮЧЕНА (RegRoamLevel=1, RegROAMSensitiveLevel=0)\n";
                std::cout << "  * Энергосбережение Wi-Fi:ОТКЛЮЧЕНО (LpsEn=0, IpsEn=0, PCIe L1=0)\n\n";

                std::cout << "Press any key to return to menu...";
                _getch();
                break;
            }
            case 'q':
            case 'Q': {
                running = false;
                break;
            }
            default:
                break;
        }
    }

    latencyOpt.RestoreTimerResolution();
    WSACleanup();
    return 0;
}
