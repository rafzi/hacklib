#ifndef HACKLIB_TIMER_H
#define HACKLIB_TIMER_H

#include <chrono>


namespace hl {


class Timer
{
public:
    void reset()
    {
        m_timestamp = std::chrono::high_resolution_clock::now();
    }

    template <typename T = float>
    T diff()
    {
        std::chrono::duration<T> fs = std::chrono::high_resolution_clock::now() - m_timestamp;
        return fs.count();
    }

private:
    std::chrono::high_resolution_clock::time_point m_timestamp = std::chrono::high_resolution_clock::now();

};

}

#endif
