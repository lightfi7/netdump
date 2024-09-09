// netdump.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <queue>
#include <chrono>
#include <thread>
#include "windivert.h"

#define ntohs(x)            WinDivertHelperNtohs(x)
#define ntohl(x)            WinDivertHelperNtohl(x)

#define MAXBUF              WINDIVERT_MTU_MAX
#define INET6_ADDRSTRLEN    45


// Define a structure to hold packet information along with a timestamp
struct PacketInfo {
    unsigned char packet[MAXBUF];
    UINT packet_len;
    WINDIVERT_ADDRESS addr;
    std::chrono::steady_clock::time_point timestamp;
};

// Queue to hold packets
std::queue<PacketInfo> packetQueue;


int main(int argc, char** argv)
{
    HANDLE handle, console;
    INT16 priority = 0;
    int lag = 0;
    unsigned char packet[MAXBUF];
    UINT packet_len;
    WINDIVERT_ADDRESS addr, send_addr;
    const char* err_str;

    switch (argc)
    {
    case 2:
        break;
    case 3:
        lag = atoi(argv[2]);
        break;
    default:
        fprintf(stderr, "usage: %s lag\n",
            argv[0]);
        fprintf(stderr, "\t%s true\n 100", argv[0]);
        exit(EXIT_FAILURE);
    }


    // Get console for pretty colors.
    console = GetStdHandle(STD_OUTPUT_HANDLE);

    // Divert traffic matching the filter:
    handle = WinDivertOpen(argv[1], WINDIVERT_LAYER_NETWORK, priority, 0);
    if (handle == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_INVALID_PARAMETER &&
            !WinDivertHelperCompileFilter(argv[1], WINDIVERT_LAYER_NETWORK,
                NULL, 0, &err_str, NULL))
        {
            fprintf(stderr, "error: invalid filter \"%s\"\n", err_str);
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "error: failed to open the WinDivert device (%d)\n",
            GetLastError());
        exit(EXIT_FAILURE);
    }

    // Main loop:
    while (TRUE)
    {
        // Read a matching packet.
        if (!WinDivertRecv(handle, packet, sizeof(packet), &packet_len, &addr))
        {
            fprintf(stderr, "warning: failed to read packet (%d)\n",
                GetLastError());
            continue;
        }

        // Store the packet in the queue with the current timestamp
        PacketInfo pktInfo;
        memcpy(pktInfo.packet, packet, packet_len);
        pktInfo.packet_len = packet_len;
        pktInfo.addr = addr;
        pktInfo.timestamp = std::chrono::steady_clock::now();
        packetQueue.push(pktInfo);
        
        SetConsoleTextAttribute(console, FOREGROUND_RED);
        fputs("<< ", stdout);

        // Process packets from the queue after a delay
        while (!packetQueue.empty())
        {
            SetConsoleTextAttribute(console, FOREGROUND_BLUE);
            fputs(">> ", stdout);

            PacketInfo& frontPkt = packetQueue.front();
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - frontPkt.timestamp).count();

            if (duration >= lag) // 100ms delay
            {
                // Process the packet (e.g., modify, drop, or re-inject).
                // ...                
                memcpy(&send_addr, &addr, sizeof(send_addr));
                send_addr.Outbound = !addr.Outbound;
                if (!WinDivertSend(handle, (PVOID)frontPkt.packet, frontPkt.packet_len,
                    NULL, &send_addr))
                {
                    fprintf(stderr, "warning: failed to send packet "
                        "(%d)\n", GetLastError());
                }
                // Remove the packet from the queue
                packetQueue.pop();
            }
            else
            {
                // Break the loop if the delay has not been reached
                break;
            }
        }
    }

    std::cout << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
