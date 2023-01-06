#include <gfx\texture\ReadOnlyTexture.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\resources\ResourcesController.hpp>

namespace GFX
{

ReadOnlyTexture::ReadOnlyTexture()
    : TextureBase{}
    , m_ShaderReadView{ nullptr }
    , m_ShaderReadDescriptor{}
{}

ReadOnlyTexture::ReadOnlyTexture(VKW::Device* device, VKW::ImageResource* resource, VKW::ImageResourceView* view, VKW::GlobalDescriptorHandle descriptor)
    : TextureBase{ device, resource }
    , m_ShaderReadView{ view }
    , m_ShaderReadDescriptor{ descriptor }
{}

ReadOnlyTexture::ReadOnlyTexture(ReadOnlyTexture&& rhs)
    : TextureBase{}
    , m_ShaderReadView{ nullptr }
    , m_ShaderReadDescriptor{}
{
    operator=(DRE_MOVE(rhs));
}


ReadOnlyTexture& ReadOnlyTexture::operator=(ReadOnlyTexture&& rhs)
{
    TextureBase::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_ShaderReadView);
    DRE_SWAP_MEMBER(m_ShaderReadDescriptor);

    return *this;
}

ReadOnlyTexture::~ReadOnlyTexture()
{
    if (m_ShaderReadView != nullptr)
    {
        m_ParentDevice->GetDescriptorAllocator()->FreeTextureDescriptor(m_ShaderReadDescriptor);
        m_ParentDevice->GetResourcesController()->FreeImageView(m_ShaderReadView);

        DRE_DEBUG_ONLY(m_ShaderReadDescriptor = VKW::GlobalDescriptorHandle{});
        DRE_DEBUG_ONLY(m_ShaderReadView = nullptr);
    }
}

}
