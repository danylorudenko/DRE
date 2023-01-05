#pragma once

#include <gfx\texture\TextureBase.hpp>

#include <vk_interface\descriptor\DescriptorManager.hpp>

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
    ReadOnlyTexture(VKW::Device* device, VKW::ImageResource* resource, VKW::ImageResourceView* srv, VKW::GlobalDescriptorHandle srvDescriptorHandle);

    ReadOnlyTexture(ReadOnlyTexture&& rhs);
    ReadOnlyTexture& operator=(ReadOnlyTexture&& rhs);

    virtual ~ReadOnlyTexture();

    inline VKW::ImageResourceView* GetShaderReadView() { return m_ShaderReadView; }
    inline VKW::GlobalDescriptorHandle const& GetShaderReadDescriptor() const { return m_ShaderReadDescriptor; }

protected:
    VKW::ImageResourceView* m_ShaderReadView;
    VKW::GlobalDescriptorHandle   m_ShaderReadDescriptor;
};

}

