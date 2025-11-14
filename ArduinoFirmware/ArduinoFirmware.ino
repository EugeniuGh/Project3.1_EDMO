
#include "globals.h"
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>
#include <cstring>
#include "Oscillator.h"
#include "IMUSensor.h"

#include "Communications/PacketUtils.h"
#include "Communications/WiFiCommManager.h"
#include "CommunicationSemaphore.h"
#include "Communications/SerialCommStream.h"

#include "TimingUtils.h"
#include <optional>
// Timing variables
unsigned long lastTime = 0;
unsigned long timeStep = 10; // period used to update CPG state variables and servo motor control (do NOT modify!!)

const float MS_TO_S = 1.0f / 1000.0f;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

bool pwmPresent = false;

// Create a semaphore that weakens after 10s
CommunicationSemaphore semaphore{10000};
ICommStream *secondaryCommstream = &dummyComms;

void setup()
{
    SerialComms.init();
    SerialComms.bindPacketHandler(packetHandler);

    WifiComms.init();
    WifiComms.bindPacketHandler(packetHandler);

    imu.init();

    pwmPresent = pwm.begin();

    if (pwmPresent)
    {
        pwm.setPWMFreq(50);
        delay(4);

        for (Oscillator &o : oscillators)
            o.setPWM(pwm);
    }
    pinMode(LED_BUILTIN, OUTPUT);
    analogWrite(LED_BUILTIN, 0);
}

uint8_t ledState = 0;
bool reversing = true;

void ledControl()
{
    const uint8_t fadeSpeed = 4;

    uint8_t newLedState = ledState + (reversing ? -fadeSpeed : fadeSpeed);

    if (newLedState == 0)
    {
        reversing = !reversing;
        return;
    }
    ledState = newLedState;

    analogWrite(LED_BUILTIN, ledState);
}

bool sessionStarted = false;
size_t lastPacketUpdateTime = 0;
size_t updateInterval = 100;

void loop()
{
    // Used as a visual indicator of functionality
    ledControl();

    // Reading input can happen regardless of whether CPG updates
    // This ensures that we don't waste time doing nothing and will maximize responsiveness
    SerialComms.update();
    WifiComms.update();
    imu.update();

    unsigned long time = millis();
    unsigned long deltaTimeMS = time - lastTime;

    // If it hasn't been 10ms since last update, we skip updating the oscillator
    if (deltaTimeMS < 10)
        return;

    lastTime = time; // update the previous time step (do NOT modify!!)

    double deltaTimeS = deltaTimeMS * MS_TO_S; // Make an interval with seconds
    if (pwmPresent)
    {
        for (auto &oscillator : oscillators)
            oscillator.update(deltaTimeS);
    }

    if (sessionStarted)
    {
        lastPacketUpdateTime += deltaTimeMS;
        if (lastPacketUpdateTime >= updateInterval)
        {
            sendDataBatch(&SerialComms, secondaryCommstream);
            lastPacketUpdateTime = 0;
        }
    }
}

enum PacketInstructions
{
    IDENTIFY = 0,
    SESSION_START = 1,
    GET_TIME = 2,

    UPDATE_OSCILLATOR = 3,
    SEND_MOTOR_DATA = 4,
    SEND_IMU_DATA = 5,

    SESSION_END = 6,

    SEND_ALL_DATA = 69,
};
void copy(const char *from, char *to, size_t length, size_t offset)
{
    for (int i = 0; i < length; ++i)
    {
        to[i + offset] = from[i];
    }
}
void sendDataBatch(ICommStream *commStream, ICommStream *commStream2)
{
    commStream->begin();
    commStream2->begin();
    writeTo(commStream, commHeader, 2);
    writeTo(commStream2, commHeader, 2);
    char unescapedBuffer[1 + sizeof(size_t) + sizeof(OscillatorState) * NUM_OSCILLATORS + sizeof(IMUSensor::IMUData)];

    unescapedBuffer[0] = SEND_ALL_DATA;

    size_t offset = 1;

    // Provides the current time based on the reference timestamp, used by the server to keep track of the last known arduino time
    //  (Probably could compute it themselves tbh)
    // The sent data is escaped as it may contain the header/footer bytes due to the variable nature of the time
    {
        auto returnedTime = TimingUtils::getTimeMillis();
        auto dataBytes = reinterpret_cast<const char *>(&returnedTime);

        copy(dataBytes, unescapedBuffer, sizeof(size_t), offset);
        offset += sizeof(size_t);
    }

    // Transfer current motor state
    for (auto &osc : oscillators)
    {
        auto &data = osc.getState();

        const char *dataBytes = reinterpret_cast<const char *>(&data);

        copy(dataBytes, unescapedBuffer, sizeof(OscillatorState), offset);
        offset += sizeof(OscillatorState);
    }

    // Transfer IMU data
    {
        auto &data = imu.rawData;
        const char *dataBytes = reinterpret_cast<const char *>(&data);

        copy(dataBytes, unescapedBuffer, sizeof(IMUSensor::IMUData), offset);
    }

    // These data bytes may accidentally contain the header or footer, let's escape it to be safe
    auto adjustedLength = countEscapedLength(unescapedBuffer, sizeof(unescapedBuffer));

    char escapedData[adjustedLength];
    escapeData(unescapedBuffer, escapedData, sizeof(unescapedBuffer));

    writeTo(commStream, escapedData, adjustedLength);
    writeTo(commStream2, escapedData, adjustedLength);

    writeTo(commStream, commFooter, 2);
    writeTo(commStream2, commFooter, 2);
    commStream->end();
    commStream2->end();
}

void writeTo(ICommStream *commStream, const char *data, size_t length)
{
    if (commStream == nullptr)
        return;

    commStream->write(data, length);
}
void writeTo(ICommStream *commStream, byte data)
{
    if (commStream == nullptr)
        return;

    commStream->write(data);
}

// Takes parses a received packet, which contain an instruction and optionally additional data
// This method may write a response via the provided ICommStream pointer
// This method returns true if a response is written, false otherwise.
//
// Any invalid instruction is dropped silently
void packetHandler(char *packet, size_t packetSize, ICommStream *commStream)
{
    // Length of the packet without the header/footer
    size_t packetLength = packetSize - 4;

    // Packet without header
    char *packetBuffer = packet + 2;

    // Unescape packet contents in place
    unescapeBuffer(packetBuffer, packetLength);

    // The first byte of the packet contents is expected to be the instruction
    char &packetInstruction = packetBuffer[0];

    // The rest of the packet contain data/arguments associated with the instruction
    // The length of the data is always fixed based on the called instruction
    char *packetData = packetBuffer + 1;

    switch (packetInstruction)
    {
    // Simply announce the robots name
    // Also used during Wifi communication to indicate presence on the network (aka pinging)
    case IDENTIFY:
    {
        // Retrieve the unique identifier of the remote, will be used for locks
        std::memcpy(&(commStream->identifier), packetData, sizeof(UUID));

        announceIdentity(commStream);
        break;
    }

    // The host has declared that a session has started/continued
    // This is required in order to align the IMU timestamps with the session timestamp
    // A manual time offset is provided as packet data,
    //    this is used to realign the timestamp if the device is reset while a session is active
    //    (otherwise the restarted device will report back with timestamp 0, causing serverside logging to be non-linear)
    case SESSION_START:
    {
        if (!semaphore.acquire(commStream->identifier))
            break;

        announceIdentity(&SerialComms);
        WifiComms.PerformOnAllChannels(announceIdentity);

        // Make sure all oscillators are reset to initial state
        for (auto osc : oscillators)
            osc.reset();

        auto currentTime = millis();

        TimingUtils::setReferenceTime(currentTime);

        size_t offsetTime{};
        std::memcpy(&offsetTime, packetData, sizeof(size_t));

        TimingUtils::setOffsetTime(offsetTime);
        sessionStarted = true;
        lastPacketUpdateTime = 0;

        secondaryCommstream = WifiComms.GetChannelWithUUID(commStream->identifier);

        break;
    }

    case SEND_ALL_DATA:
        sendDataBatch(commStream, secondaryCommstream);
        break;

    // The host is sending updated oscillator parameters
    // The packet data contains 2 arguments, the first byte is the target oscillator
    // The rest of the packet data is the oscillator parameters
    case UPDATE_OSCILLATOR:
    {
        // This has the side effect of refreshing the lock
        if (!semaphore.acquire(commStream->identifier))
            break;

        uint8_t targetOscillator = packetData[0];
        OscillatorParams updateCommand{};

        std::memcpy(&updateCommand, packetData + 1, sizeof(OscillatorParams));
        oscillators[targetOscillator].setParams(updateCommand);

        break;
    }

    case SESSION_END:
    {
        if (!semaphore.release(commStream->identifier))
            break;

        sessionStarted = false;
        announceIdentity(&SerialComms);
        WifiComms.PerformOnAllChannels(announceIdentity);
        break;
    }
    }
}

void announceIdentity(ICommStream *commStream)
{
    commStream->begin();
    commStream->write(commHeader, 2);
    commStream->write(IDENTIFY); // Write the instruction as a response
    commStream->write((uint8_t *)idCode.c_str(), idCode.length());
    commStream->write(0);
    commStream->write(NUM_OSCILLATORS);
    commStream->write((byte *)oscillatorColours, sizeof(u_int16_t) * NUM_OSCILLATORS);

    // Write the current lock state
    commStream->write(!semaphore.lockExpired() && semaphore.currentLockHolder() != commStream->identifier);
    commStream->write(commFooter, 2);

    commStream->end();
}