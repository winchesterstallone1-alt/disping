#pragma once

#include "disping_types.hpp"
#include <string>
#include <vector>

namespace disping {

class UIConsole {
public:
    static void InitConsole();
    static void ClearScreen();

    // Color helpers (ANSI)
    static std::string Reset();
    static std::string Bold();
    static std::string Red();
    static std::string Green();
    static std::string Yellow();
    static std::string Blue();
    static std::string Magenta();
    static std::string Cyan();
    static std::string White();
    static std::string BrightGreen();
    static std::string BrightCyan();
    static std::string BrightYellow();

    // Visual elements
    static void PrintBanner();
    static void PrintSystemStatus(bool isAdmin, double timerMs, bool netTweaked);
    static void PrintMenu();
    static void PrintPingTable(const std::vector<PingStats>& stats);
    static void PrintDnsTable(const std::vector<DnsBenchmarkResult>& dnsList);
    static void PrintAdaptersTable(const std::vector<NetworkAdapterInfo>& adapters);
    static void PrintOperationResult(const OperationResult& res);
    static void PrintHeader(const std::string& title);
};

} // namespace disping
