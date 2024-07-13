#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>

#include <vk_wrapper\Constant.hpp>
#include <vk_wrapper\Format.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>

#include <gfx\DeviceChild.hpp>

namespace VKW
{
struct ImageResource;
struct ImageResourceView;
}

namespace GFX
{

class Texture
    : public DeviceChild
{
public:
    Texture();
    Texture(VKW::Device* device, VKW::ImageResource* image, VKW::ImageResourceView* view, VKW::TextureDescriptorIndex descriptorHandle);

    Texture(Texture&& rhs);
    Texture& operator=(Texture&& rhs);

    inline std::uint32_t    GetWidth() const { return m_Width; }
    inline std::uint32_t    GetHeight() const { return m_Height; }
    inline std::uint32_t    GetMipsCount() const { return m_MipsCount; }
    inline std::uint32_t    GetArrayLayersCount() const { return m_ArrayLayersCount; }
    inline VKW::Format      GetFormat() const { return m_Format; }

    inline VKW::ImageResource*       GetResource() { return m_Image; }
    inline VKW::ImageResource const* GetResource() const { return m_Image; }

    inline VKW::ImageResourceView*              GetShaderView() const { return m_ShaderView; }
    inline VKW::TextureDescriptorIndex const&   GetShaderGlobalDescriptor() const { return m_ShaderGlobalDescriptor; }

    VKW::DescriptorSet GetImGuiDescriptor() const;

    virtual ~Texture();

protected:
    VKW::ImageResource* m_Image;
    std::uint32_t       m_Width;
    std::uint32_t       m_Height;
    std::uint16_t       m_MipsCount;
    std::uint16_t       m_ArrayLayersCount;
    VKW::Format         m_Format;

    VKW::ImageResourceView*         m_ShaderView;
    VKW::TextureDescriptorIndex     m_ShaderGlobalDescriptor;

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    VKW::DescriptorSet  m_ImGuiDescriptorSet;
#endif

};

}


