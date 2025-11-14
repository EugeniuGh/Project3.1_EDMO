#pragma once

#include "../globals.h"
#include "ICommStream.h"

#if WIFI_SUPPORT == 1

#include <WiFi101.h>
#include <WiFiUdp.h>
class WiFiCommStream : public virtual ICommStream
{
private:
    WiFiUDP& udp;
    const IPAddress remoteIP;
    uint16_t remotePort;

    WiFiCommStream(WiFiUDP& host, IPAddress remoteIP, u_int16_t remotePort)
        : udp{host}, remoteIP{remoteIP}, remotePort{remotePort} { }

public:

    void init()
    {
        // Intentionally dummied out, no initialisation is necessary
    }

    void write(const uint8_t *const data, size_t length) override
    {
        udp.write(data, length);
    }

    void write(uint8_t byte) override
    {
        udp.write(byte);
    }

    void begin() override
    {
        udp.beginPacket(remoteIP, remotePort);
    }

    void end() override
    {
        udp.endPacket();
    }

    void receive(int packetSize, char *packetBuffer)
    {
        if (!buffcmp(packetBuffer, commHeader, 2) || !buffcmp(packetBuffer + packetSize - 2, commFooter, 2))
            return;

        parsePacket(packetBuffer, packetSize, this);
    }

    void updatePort(uint16_t port){
        remotePort = port;
    }

    void update() override
    {
        // Intentionally dummied out, this stream doesn't manage its own updates
    }

    friend class WifiCommManager;
};
#else
#include "DummyCommStream.h"

using WiFiCommStream = DummyCommStream;
#endif