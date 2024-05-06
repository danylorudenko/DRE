#include <gfx\texture\ReadOnlyTexture.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\resources\ResourcesController.hpp>

namespace GFX
{

ReadOnlyTexture::ReadOnlyTexture()
    : TextureBase{}
    , m_ShaderReadView{ nullptr }
    , m_ShaderGlobalDescriptor{}
{}

ReadOnlyTexture::ReadOnlyTexture(VKW::Device* device, VKW::ImageResource* resource, VKW::ImageResourceView* view, VKW::TextureDescriptorIndex descriptor)
    : TextureBase{ device, resource }
    , m_ShaderReadView{ view }
    , m_ShaderGlobalDescriptor{ descriptor }
{}

ReadOnlyTexture::ReadOnlyTexture(ReadOnlyTexture&& rhs)
    : TextureBase{}
    , m_ShaderReadView{ nullptr }
    , m_ShaderGlobalDescriptor{}
{
    operator=(DRE_MOVE(rhs));
}


ReadOnlyTexture& ReadOnlyTexture::operator=(ReadOnlyTexture&& rhs)
{
    TextureBase::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_ShaderReadView);
    DRE_SWAP_MEMBER(m_ShaderGlobalDescriptor);

    return *this;
}

ReadOnlyTexture::~ReadOnlyTexture()
{
    if (m_ShaderReadView != nullptr)
    {
        m_ParentDevice->GetDescriptorManager()->FreeTextureDescriptor(m_ShaderGlobalDescriptor);
        m_ParentDevice->GetResourcesController()->FreeImageView(m_ShaderReadView);

        DRE_DEBUG_ONLY(m_ShaderGlobalDescriptor = VKW::TextureDescriptorIndex{});
        DRE_DEBUG_ONLY(m_ShaderReadView = nullptr);
    }
}

}
