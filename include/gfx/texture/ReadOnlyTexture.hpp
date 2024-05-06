#pragma once

#include <gfx\texture\TextureBase.hpp>

#include <vk_wrapper\descriptor\DescriptorManager.hpp>

namespace VKW
{
struct ImageResource;
struct ImageResourceView;
}

namespace GFX
{

class ReadOnlyTexture
    : public TextureBase
{
public:
    ReadOnlyTexture();
    ReadOnlyTexture(VKW::Device* device, VKW::ImageResource* resource, VKW::ImageResourceView* srv, VKW::TextureDescriptorIndex srvDescriptorHandle);

    ReadOnlyTexture(ReadOnlyTexture&& rhs);
    ReadOnlyTexture& operator=(ReadOnlyTexture&& rhs);

    virtual ~ReadOnlyTexture();

    inline VKW::ImageResourceView* GetShaderReadView() { return m_ShaderReadView; }
    inline VKW::TextureDescriptorIndex const& GetShaderGlobalReadDescriptor() const { return m_ShaderGlobalDescriptor; }

protected:
    VKW::ImageResourceView*         m_ShaderReadView;
    VKW::TextureDescriptorIndex     m_ShaderGlobalDescriptor;
};

}

