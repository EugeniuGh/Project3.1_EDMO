#pragma once
#include <delay.h>
#include <optional>
#include "Communications/ICommStream.h"

class CommunicationSemaphore
{
public:
    CommunicationSemaphore(uint timeout) : lockExpiry(timeout) {}

    // Might wanna do identifier based, since multiple different channels can originate from the same remote...
    bool acquire(const UUID &lockID)
    {
        ulong currentTime = millis();

        // We do not yield the lock, if the lock is held by another communication channel, but the timeout has not been exceeded
        if (lockHolder != lockID && currentTime - lockTime < lockExpiry)
            return false;

        lockHolder = lockID;
        lockTime = currentTime;

        return true;
    }

    bool release(const UUID &lockID)
    {
        if (lockHolder != lockID)
            return false;

        lockHolder = UUID({0, 0, 0, 0});
        lockTime = 0;
        return true;
    }


    const bool lockExpired() const
    {
        ulong currentTime = millis();

        return (currentTime - lockTime) >= lockExpiry;
    }

    const UUID &currentLockHolder() const
    {
        return lockHolder;
    }

private:
    UUID lockHolder{{0, 0, 0, 0}};
    long lockTime{LONG_MIN};

    const ulong lockExpiry;
};