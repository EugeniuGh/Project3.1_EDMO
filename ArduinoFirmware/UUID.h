#pragma once

// Weak expectation, most uses of UUID will be transferred over the wire
class UUID
{
public:
    UUID()
    {
        for (int i = 0; i < 4; ++i)
            _words[i] = random();
    }

    UUID(const uint32_t (&words)[4])
    {
        for (int i = 0; i < 4; ++i)
            _words[i] = words[i];
    }

    const uint32_t* asWords() const{
        return _words;
    }

    static inline bool equals(const UUID &lhs, const UUID &rhs)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (lhs._words[i] != rhs._words[i])
                return false;
        }
        return true;
    }

private:
    uint32_t _words[4];
};

static inline bool operator==(const UUID& lhs, const UUID& rhs){
    return UUID::equals(lhs, rhs);
}

static inline bool operator!=(const UUID& lhs, const UUID& rhs)
{
    return !UUID::equals(lhs, rhs);
}