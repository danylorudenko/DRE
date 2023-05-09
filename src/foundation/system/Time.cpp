#include <foundation\system\Time.hpp>

DRE_BEGIN_NAMESPACE

Stopwatch::Stopwatch()
    : m_Start{ std::chrono::high_resolution_clock::now() }
    , m_PausePoint{ m_Start }
    , m_PausedPeriod{ TimePeriod::zero() }
    , m_Paused{ false }
{}

void Stopwatch::Reset()
{
    m_Start = std::chrono::high_resolution_clock::now();
    m_PausePoint = m_Start;
    m_PausedPeriod = TimePeriod::zero();
    m_Paused = false;
}

void Stopwatch::Reset(TimePoint point)
{
    m_Start = point;
    m_PausePoint = m_Start;
    m_PausedPeriod = TimePeriod::zero();
    m_Paused = false;
}

std::uint64_t Stopwatch::CurrentSeconds() const
{
    auto now = m_Paused ? m_PausePoint : std::chrono::high_resolution_clock::now();
    auto period = (now - m_Start) - m_PausedPeriod;
    return std::chrono::duration_cast<std::chrono::seconds>(period).count();
    
}

std::uint64_t Stopwatch::CurrentMilliseconds() const
{
    auto now = m_Paused ? m_PausePoint : std::chrono::high_resolution_clock::now();
    auto period = (now - m_Start) - m_PausedPeriod;
    return std::chrono::duration_cast<std::chrono::milliseconds>(period).count();
    
    
}

std::uint64_t Stopwatch::CurrentMicroseconds() const
{
    auto now = m_Paused ? m_PausePoint : std::chrono::high_resolution_clock::now();
    auto period = (now - m_Start) - m_PausedPeriod;
    return std::chrono::duration_cast<std::chrono::microseconds>(period).count();
}

void Stopwatch::Pause()
{
    m_PausePoint = std::chrono::high_resolution_clock::now();
    m_Paused = true;
}

void Stopwatch::Unpause()
{
    m_Paused = false;
    m_PausedPeriod += std::chrono::high_resolution_clock::now() - m_PausePoint;
}


DRE_END_NAMESPACE

