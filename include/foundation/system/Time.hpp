#pragma once

#include <foundation\Common.hpp>
#include <chrono>

DRE_BEGIN_NAMESPACE

class Stopwatch
{
public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    Stopwatch();

    std::uint64_t CurrentSeconds();
    std::uint64_t CurrentMilliseconds();
    std::uint64_t CurrentMicroseconds();

    void Reset();
    void Reset(TimePoint point);

private:
    TimePoint m_Start;
};

DRE_END_NAMESPACE
