#pragma once

#include <foundation\Common.hpp>

namespace GFX
{

class FrameID
{
public:
    operator std::uint8_t() const { return m_ID; }
    FrameID() : m_ID{ DRE_U8_MAX } {}

private:
    friend class GraphicsManager;

    explicit FrameID(std::uint8_t id) : m_ID{ id } {}
    std::uint8_t m_ID;
};

}

