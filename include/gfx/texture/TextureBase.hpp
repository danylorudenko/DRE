#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>

#include <vk_wrapper\Format.hpp>

#include <gfx\DeviceChild.hpp>

namespace VKW
{
struct ImageResource;
}

namespace GFX
{

class TextureBase
    : public DeviceChild
{
public:
    explicit TextureBase();
    explicit TextureBase(VKW::Device* device, VKW::ImageResource* image);

    TextureBase(TextureBase&& rhs);
    TextureBase& operator=(TextureBase&& rhs);

    inline std::uint32_t    GetWidth() const { return m_Width; }
    inline std::uint32_t    GetHeight() const { return m_Height; }
    inline std::uint32_t    GetMipsCount() const { return m_MipsCount; }
    inline std::uint32_t    GetArrayLayersCount() const { return m_ArrayLayersCount; }
    inline VKW::Format      GetFormat() const { return m_Format; }

    inline VKW::ImageResource*       GetResource() { return m_Image; }
    inline VKW::ImageResource const* GetResource() const { return m_Image; }

    virtual ~TextureBase();

protected:
    VKW::ImageResource* m_Image;
    std::uint32_t       m_Width;
    std::uint32_t       m_Height;
    std::uint16_t       m_MipsCount;
    std::uint16_t       m_ArrayLayersCount;
    VKW::Format         m_Format;

};

}


