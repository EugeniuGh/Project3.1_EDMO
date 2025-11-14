#pragma once

#include "../globals.h"
#include "DummyCommStream.h"

#if WIFI_SUPPORT == 1
#include <WiFi101.h>
#include <WiFiUdp.h>

#include "ICommStream.h"
#include "WiFiCommStream.h"
#include <unordered_map>

class WifiCommManager
{
public:
    void init()
    {
        WiFi.setPins(8, 7, 4, 2);
        WiFi.hostname(hostname.c_str());
        WiFi.begin(ssid, pass);

        udp.begin(2121);
    }

    using PacketHandler = ICommStream::PacketHandler;

    void bindPacketHandler(PacketHandler handler)
    {
        packetHandler = handler;
        packetHandlerBound = true;
    }

    void update()
    {
        int packetSize = udp.parsePacket();

        if (packetSize == 0)
            return;

        // No packet handler is available
        // Drop the packet
        if (!packetHandlerBound)
            return;

        auto remoteIP = udp.remoteIP();
        auto remotePort = udp.remotePort();

        char buffer[packetSize];
        udp.readBytes(buffer, packetSize);

        auto iterator = commStreamMapping.find(remoteIP);

        if (iterator == commStreamMapping.end())
        {
            auto commStreamPair = commStreamMapping.emplace((u_int32_t)remoteIP, new WiFiCommStream(udp, remoteIP, remotePort));
            iterator = commStreamPair.first;

            iterator->second->bindPacketHandler(packetHandler);
        }
        iterator->second->updatePort(remotePort);
        iterator->second->receive(packetSize, buffer);
    }

    void PerformOnAllChannels(std::function<void(ICommStream *)> action)
    {
        for (auto &pair : commStreamMapping)
            action(pair.second);
    }

    ICommStream *GetChannelWithUUID(const UUID &uuid)
    {
        for (auto &pair : commStreamMapping)
            if (pair.second->identifier == uuid)
                return pair.second;

        return &dummyComms;
    }

private:
    WiFiUDP udp{};
    std::unordered_map<uint32_t, WiFiCommStream *> commStreamMapping{};

    PacketHandler packetHandler;
    bool packetHandlerBound{};
};

#else
// A dummied out version of WiFiCommManager
class WifiCommManager
{
public:
    void init()
    {
    }

    void update()
    {
    }

    void PerformOnAllChannels(std::function<void(ICommStream *)> action)
    {
    }

    ICommStream *GetChannelWithUUID(const UUID &uuid)
    {
        return &dummyComms;
    }

    using PacketHandler = ICommStream::PacketHandler;

    void bindPacketHandler(PacketHandler handler)
    {
    }
};
#endif

WifiCommManager WifiComms;
