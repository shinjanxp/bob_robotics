#pragma once

// Standard C++ includes
#include <chrono>

namespace BoBRobotics {
class Stopwatch
{
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::high_resolution_clock::duration;

    //! Start measuring elapsed time from now
    void start()
    {
        m_StartTime = now();
    }

    //! Returns elapsed time since start() was called
    Duration elapsed() const
    {
        return now() - m_StartTime;
    }

    //! Returns the current elapsed time and restarts the Stopwatch
    Duration lap()
    {
        const TimePoint currentTime = now();
        const Duration elapsed = currentTime - m_StartTime;
        m_StartTime = currentTime;
        return elapsed;
    }

    bool running() const
    {
        return m_StartTime != TimePoint::min();
    }

    void stop()
    {
        m_StartTime = TimePoint::min();
    }

    //! Get the current time
    static TimePoint now()
    {
        return std::chrono::high_resolution_clock::now();
    }

private:
    TimePoint m_StartTime = TimePoint::min();
}; // Stopwatch

class EggTimer
  : public Stopwatch
{
public:
    void start(const std::chrono::nanoseconds duration)
    {
        Stopwatch::start();
        m_Duration = duration;
    }

    bool finished() const
    {
        return elapsed() >= m_Duration;
    }

private:
    std::chrono::nanoseconds m_Duration = std::chrono::nanoseconds::zero();
}; // EggTimer
} // BoBRobotics
