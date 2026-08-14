#pragma once

#include <foundation\Common.hpp>
#include <gfx\FrameID.hpp>
#include <vk_wrapper\Constant.hpp>
#include <foundation\container\InplaceVector.hpp>

namespace GFX
{

template<typename T, DRE::U32 FRAME_COUNT = VKW::CONSTANTS::FRAMES_BUFFERING>
class PerFrame final
{
public:
    PerFrame() = default;

    template<typename TInitializer>
    PerFrame(TInitializer&& initializer)
    {
        for (DRE::U32 i = 0; i < FRAME_COUNT; ++i)
        {
            m_Values.EmplaceBack(initializer());
        }
    }

    template<typename TInitializer>
    PerFrame& operator=(TInitializer&& initializer)
    {
        for (DRE::U32 i = 0; i < FRAME_COUNT; ++i)
        {
            m_Values.EmplaceBack(initializer());
        }
        return *this;
    }

    PerFrame& operator=(PerFrame const&) = default;
    PerFrame& operator=(PerFrame&&) = default;

    constexpr T& Get(GFX::FrameID frameID)
    {
        return m_Values[frameID];
    }

    constexpr T const& Get(GFX::FrameID frameID) const
    {
        return m_Values[frameID];
    }

    template<typename TFunc>
    void ForEach(TFunc&& func)
    {
        for (DRE::U32 i = 0; i < FRAME_COUNT; ++i)
        {
            func(m_Values[i]);
        }
    }

private:
    DRE::InplaceVector<T, FRAME_COUNT> m_Values;
};

} // namespace GFX