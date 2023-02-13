#include <foundation\system\Time.hpp>

DRE_BEGIN_NAMESPACE

Stopwatch::Stopwatch()
    : m_Start{ std::chrono::high_resolution_clock::now() }
{}

void Stopwatch::Reset()
{
    m_Start = std::chrono::high_resolution_clock::now();
}

void Stopwatch::Reset(TimePoint point)
{
    m_Start = point;
}

std::uint64_t Stopwatch::CurrentSeconds()
{
    auto period = std::chrono::high_resolution_clock::now() - m_Start;
    return std::chrono::duration_cast<std::chrono::seconds>(period).count();
}

std::uint64_t Stopwatch::CurrentMilliseconds()
{
    auto period = std::chrono::high_resolution_clock::now() - m_Start;
    return std::chrono::duration_cast<std::chrono::milliseconds>(period).count();
}

std::uint64_t Stopwatch::CurrentMicroseconds()
{
    auto period = std::chrono::high_resolution_clock::now() - m_Start;
    return std::chrono::duration_cast<std::chrono::microseconds>(period).count();
}

DRE_END_NAMESPACE

