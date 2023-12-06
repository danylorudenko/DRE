#pragma once

#include <foundation\Common.hpp>

namespace WORLD
{
class ISceneNodeUser;
}

namespace DRE
{

struct ApplicationContext
{
    // Time
    std::uint64_t   m_EngineFrame = 0;
    std::uint64_t   m_TimeSinceStartUS = 0;
    std::uint64_t   m_SystemTimeUS = 0;
    std::uint64_t   m_DeltaTimeUS = 0;

    bool            m_PauseTime = false;

    // Editor
    WORLD::ISceneNodeUser* m_FocusedObject = nullptr;
};

extern ApplicationContext g_AppContext;

}
