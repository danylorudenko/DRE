#pragma once

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>
#include <glm\vec3.hpp>

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
    static constexpr DRE::U32 FLAG_DRAW         = 1 << 0;
    static constexpr DRE::U32 FLAG_SHOW_HISTORY = 1 << 1;
    static constexpr DRE::U32 FLAG_SHOW_X       = 1 << 2;
    static constexpr DRE::U32 FLAG_SHOW_Y       = 1 << 3;
    static constexpr DRE::U32 FLAG_SHOW_Z       = 1 << 4;
    static constexpr DRE::U32 FLAG_SHOW_W       = 1 << 5;

    DRE::String128 m_TextureName;
    DRE::U32 m_Flags = FLAG_SHOW_X | FLAG_SHOW_Y | FLAG_SHOW_Z | FLAG_SHOW_W;
    float m_SizeX = 0.5f;
    float m_SizeY = 0.5f;

    float m_OffsetX = 0.0f;
    float m_OffsetY = 0.0f;

    float m_LowerEnd = 0.0f;
    float m_UpperEnd = 1.0f;
};

struct DDGIDebugState
{
    // keep in sync with ddgi_common.slang
    enum VisMode
    {
        Color     = 0,
        Normal    = 1,
        UV        = 2,
    };

    bool m_DrawProbes = false;
    bool m_DrawProbeRays = false;
    bool m_DrawSelectedCage = false;

    float m_SphereScale = 0.25f;
    VisMode m_VisMode = VisMode::Color;
    glm::uvec3 m_FocusProbe;
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

    bool        m_FreezeCursorPosition = false;
    DRE::S32    m_CursorX = 0;
    DRE::S32    m_CursorY = 0;


    // Texture Inspector
    TextureViewState m_TextureInspectorViewState;

    // DDGI Debug
    DDGIDebugState m_DDGIDebugState;
};

extern ApplicationContext g_AppContext;

}
