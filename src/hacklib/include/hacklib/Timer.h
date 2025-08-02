#ifndef HACKLIB_TIMER_H
#define HACKLIB_TIMER_H

#include <chrono>


namespace hl
{

// A simple timer class that doesn't require knowledge of std::chrono.
// It starts when it is being constructed.
class Timer
{
    using Clock = std::chrono::high_resolution_clock;

public:
    void reset() { m_timestamp = Clock::now(); }

    // Returns the current time difference in seconds.
    template <typename T = float>
    T diff() const
    {
        std::chrono::duration<T> fs = Clock::now() - m_timestamp;
        return fs.count();
    }

private:
    Clock::time_point m_timestamp = Clock::now();
};
}

#endif
