#pragma once

#include <foundation\Common.hpp>
#include <chrono>

DRE_BEGIN_NAMESPACE

class Stopwatch
{
public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using TimePeriod = TimePoint::duration;

    Stopwatch();

    std::uint64_t CurrentSeconds() const;
    std::uint64_t CurrentMilliseconds() const;
    std::uint64_t CurrentMicroseconds() const;

    inline bool IsPaused() const { return m_Paused; }

    void Reset();
    void Reset(TimePoint point);

    void Pause();
    void Unpause();

private:
    TimePoint   m_Start;
    TimePoint   m_PausePoint;
    TimePeriod  m_PausedPeriod;
    bool        m_Paused;
};

DRE_END_NAMESPACE
