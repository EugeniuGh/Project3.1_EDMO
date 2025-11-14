#pragma once

#include <Adafruit_PWMServoDriver.h>
#include <math.h>
#include "OscillatorState.h"

// This class holds all state and methods related to oscillators
class Oscillator
{
public:
    Oscillator(unsigned int servoMin = 100, unsigned int servoMax = 450, const OscillatorParams &parameters = {})
        : id{nextID++}, servoMin{servoMin}, servoMax{servoMax}, state{parameters}, params{parameters}, defaultParams{parameters}
    {
    }

    void setPWM(Adafruit_PWMServoDriver &pwm) { this->pwm = &pwm; }

    // Updates the current state of the motor based on the elapsed time (in seconds)
    void update(double dt)
    {
        // Calculate deltas
        float offsetDelta = OFFSET_CHANGE_FACTOR * (params.offset - state.offset);
        float amplitudeDelta = AMPLITUDE_CHANGE_FACTOR * (params.amp - state.amp);
        float phaseDelta = (TWO_PI * state.freq);
        float phaseShiftDelta = PHASESHIFT_CHANGE_FACTOR * (params.phaseShift - state.phaseShift);
        float freqDelta = FREQ_CHANGE_FACTOR * (params.freq - state.freq);

        // Apply derivative from last tick/frame/timestep
        state.amp += amplitudeDelta * dt;
        state.phase += phaseDelta * dt;
        state.offset += offsetDelta * dt;
        state.phaseShift += phaseShiftDelta * dt;
        state.freq += freqDelta * dt;

        float position = state.amp * sinf(state.phase - state.phaseShift) + state.offset;

        uint16_t motorAngle = map(constrain(position, 25, 180), 0, 180, servoMin, servoMax);

        pwm->setPWM(id, 0, motorAngle);
    }

    void reset()
    {
        state = params = defaultParams;
    }

    // Adjusts a parameter of this oscillator using information obtained from an OscillatorUpdateCommand
    // Assuming the struct is used as a Serial communication format, one can easily reinterpret a 12byte char[] as an OscillatorUpdateCommand
    void setParams(const OscillatorParams &command)
    {
        params = command;
    }

    const OscillatorState &getState() const
    {
        return state;
    }

    const uint8_t id;

private:
    Adafruit_PWMServoDriver *pwm = nullptr;
    static uint8_t nextID;

    const float AMPLITUDE_CHANGE_FACTOR = 1;
    const float OFFSET_CHANGE_FACTOR = 0.5;
    const float FREQ_CHANGE_FACTOR = 0.5;
    const float PHASESHIFT_CHANGE_FACTOR = 1;

    OscillatorParams params;
    OscillatorState state;

    const OscillatorParams defaultParams;

    unsigned int servoMin, servoMax;
};

uint8_t Oscillator::nextID = 0;
