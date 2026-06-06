#pragma once

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>

namespace WORLD
{
class ISceneNodeUser;
}

namespace DRE
{

enum class ObjectFocusEvent
{
    None,
    ImGui,
    Picking
};

struct TextureViewState
{
    bool m_DrawTexture = false;
    DRE::String128 m_TextureName;
    float m_Size = 0.5f;

    float m_LowerEnd = 0.0f;
    float m_UpperEnd = 1.0f;

    bool m_ShowX = true;
    bool m_ShowY = true;
    bool m_ShowZ = true;
    bool m_ShowW = true;
};

struct DDGIDebugState
{
    enum VisMode
    {
        Color,
        Normal,
        UV,
    };

    bool m_DrawProbes = false;
    float m_SphereScale = 1.0f;
    VisMode m_VisMode = VisMode::Color;
};

struct ApplicationContext
{
    // Time
    DRE::U64    m_EngineFrame = 0;
    DRE::U64    m_TimeSinceStartUS = 0;
    DRE::U64    m_SystemTimeUS = 0;
    DRE::U64    m_DeltaTimeUS = 0;

    bool        m_PauseTime = false;

    // Focused Object
    WORLD::ISceneNodeUser*  m_FocusedObject = nullptr;
    DRE::U32                m_MouseHoveredObjectID = 0;

    DRE::S32    m_CursorX = 0;
    DRE::S32    m_CursorY = 0;


    // Texture Inspector
    TextureViewState m_TextureInspectorViewState;

    // DDGI Debug
    DDGIDebugState m_DDGIDebugState;
};

extern ApplicationContext g_AppContext;

}
