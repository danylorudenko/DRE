#pragma once

#include <foundation\Common.hpp>
#include <gfx\buffer\ReadbackProxy.hpp>

namespace WORLD
{
class ISceneNodeUser;
}

namespace DRE
{

struct ApplicationContext
{
    // Time
    DRE::U64    m_EngineFrame = 0;
    DRE::U64    m_TimeSinceStartUS = 0;
    DRE::U64    m_SystemTimeUS = 0;
    DRE::U64    m_DeltaTimeUS = 0;

    bool        m_PauseTime = false;

    // Editor
    WORLD::ISceneNodeUser* m_FocusedObject = nullptr;
    GFX::ReadbackFuture m_LastObjectIDsFuture;
    DRE::U32    m_PickedObjectID = 0;

    DRE::S32    m_CursorX = 0;
    DRE::S32    m_CursorY = 0;

};

extern ApplicationContext g_AppContext;

}
