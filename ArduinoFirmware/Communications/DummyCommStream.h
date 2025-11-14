#pragma once
#include "ICommStream.h"

class DummyCommStream : public virtual ICommStream
{
public:
    void init() override
    {
    }

    void write(const uint8_t *data, size_t length)
    {
    }

    void write(const uint8_t byte)
    {
    }

    void write(const char *const data, size_t length) override
    {
    }

    void update() override
    {
    }

    void begin() override
    {
    }

    void end() override
    {
    }
};

DummyCommStream dummyComms;