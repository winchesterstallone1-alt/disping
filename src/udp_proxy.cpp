#include "udp_proxy.hpp"
#include "disping_asm.h"
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <iostream>
#include <vector>
#include <chrono>

#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

namespace disping {

UdpProxy::~UdpProxy() {
    Stop();
}

void UdpProxy::Stop() {
    if (m_running.load()) {
        m_running.store(false);
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
    }
}

UdpProxyStats UdpProxy::GetStats() const {
    return m_stats;
}

OperationResult UdpProxy::Start(
    uint16_t listenPort,
    const std::string& remoteHost,
    uint16_t remotePort)
{
    OperationResult res;
    if (m_running.load()) {
        res.success = false;
        res.message = "UDP Proxy is already active.";
        return res;
    }

    m_running.store(true);
    m_stats = UdpProxyStats();

    m_workerThread = std::thread([this, listenPort, remoteHost, remotePort]() {
        SOCKET listenSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (listenSock == INVALID_SOCKET) {
            m_running.store(false);
            return;
        }

        // 1. Maximize socket receive and send buffers to 4MB
        int bufSize = 4 * 1024 * 1024;
        setsockopt(listenSock, SOL_SOCKET, SO_RCVBUF, (const char*)&bufSize, sizeof(bufSize));
        setsockopt(listenSock, SOL_SOCKET, SO_SNDBUF, (const char*)&bufSize, sizeof(bufSize));

        // 2. Set DSCP 46 (Expedited Forwarding: TOS 0xB8)
        int tos = 0xB8;
        setsockopt(listenSock, IPPROTO_IP, IP_TOS, (const char*)&tos, sizeof(tos));

        // 3. Disable SIO_UDP_CONNRESET (prevents silent socket teardowns on ICMP port unreachable)
        BOOL bNewBehavior = FALSE;
        DWORD dwBytesReturned = 0;
        WSAIoctl(listenSock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior),
                 NULL, 0, &dwBytesReturned, NULL, NULL);

        // Bind listen socket
        sockaddr_in localAddr;
        ZeroMemory(&localAddr, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = INADDR_ANY;
        localAddr.sin_port = htons(listenPort);

        if (bind(listenSock, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
            closesocket(listenSock);
            m_running.store(false);
            return;
        }

        // Target remote address
        sockaddr_in remoteAddr;
        ZeroMemory(&remoteAddr, sizeof(remoteAddr));
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port = htons(remotePort);
        inet_pton(AF_INET, remoteHost.c_str(), &remoteAddr.sin_addr);

        // 100ms receive timeout so thread can poll m_running flag cleanly
        DWORD timeout = 100;
        setsockopt(listenSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        alignas(16) char packetBuffer[65536];
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);

        while (m_running.load()) {
            int received = recvfrom(listenSock, packetBuffer, sizeof(packetBuffer), 0,
                                    (sockaddr*)&clientAddr, &clientLen);

            if (received > 0) {
                auto t1 = std::chrono::high_resolution_clock::now();

                // Forward immediately to remote game server
                int sent = sendto(listenSock, packetBuffer, received, 0,
                                  (sockaddr*)&remoteAddr, sizeof(remoteAddr));

                auto t2 = std::chrono::high_resolution_clock::now();
                double us = std::chrono::duration<double, std::micro>(t2 - t1).count();

                if (sent > 0) {
                    m_stats.packetsForwarded++;
                    m_stats.bytesForwarded += sent;
                    m_stats.avgForwardLatencyUs = (m_stats.avgForwardLatencyUs * 0.95) + (us * 0.05);
                }

                // Fast non-temporal clear of buffer using assembly
                asm_fast_memzero_nt(packetBuffer, static_cast<size_t>(received));
            }
        }

        closesocket(listenSock);
    });

    res.success = true;
    res.message = "Started UDP Fast Relay on port " + std::to_string(listenPort) + 
                  " -> " + remoteHost + ":" + std::to_string(remotePort);
    return res;
}

} // namespace disping
