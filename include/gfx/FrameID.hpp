#pragma once

#include <foundation\Common.hpp>

namespace GFX
{

class FrameID
{
public:
    operator DRE::U8() const { return m_ID; }
    FrameID() : m_ID{ DRE_U8_MAX } {}

private:
    friend class GraphicsManager;

    explicit FrameID(DRE::U8 id) : m_ID{ id } {}
    DRE::U8 m_ID;
};

}

