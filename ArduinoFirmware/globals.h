/*
 * This file contains variables and defines related to the configuration of the robot.
 */

#pragma once
#include <string>
#include "Oscillator.h"

// Name of the device
const std::string idCode{"Suzanne"};

// Packet headers and footers
const char commHeader[]{'E', 'D'};
const char commFooter[]{'M', 'O'};

// WiFi support stuff

#define WIFI_SUPPORT 1

#if WIFI_SUPPORT == 1
const std::string hostname{"EDMO: " + idCode};
const char ssid[]{"EDMO"};     //  your network SSID (name)
const char pass[]{"edmotest"}; // your network password
#endif

// Oscilator specifications

Oscillator oscillators[] = {
    Oscillator(100, 600),
    Oscillator(100, 600),
    Oscillator(100, 600),
    Oscillator(100, 600)
};

const uint16_t NUM_OSCILLATORS = sizeof(oscillators) / sizeof(oscillators[0]); // this number has to match entries in array osc[] (do NOT modify!!)
const uint16_t oscillatorColours[] {
    0,120,240,60
};


// SPI has faster throughput, but more wires
#define IMU_SPI 1