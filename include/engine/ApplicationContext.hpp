#pragma once

#include <cstdint>

namespace DRE
{

struct ApplicationContext
{
    std::uint64_t   m_EngineFrame = 0;
    std::uint64_t   m_TimeSinceStartUS = 0;
    std::uint64_t   m_SystemTimeUS = 0;
    std::uint64_t   m_DeltaTimeUS = 0;

    bool            m_PauseTime = false;
};

extern ApplicationContext g_AppContext;

}
