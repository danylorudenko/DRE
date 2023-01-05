#include <gfx\texture\StorageTexture.hpp>

#include <foundation\Common.hpp>

#include <vk_interface\Device.hpp>
#include <vk_interface\resources\Resource.hpp>
#include <vk_interface\resources\ResourcesController.hpp>


namespace GFX
{

StorageTexture::StorageTexture()
    : TextureBase{}
    , m_ShaderView{ nullptr }
{}

StorageTexture::StorageTexture(VKW::Device* device, VKW::ImageResource* image, VKW::ImageResourceView* view)
    : TextureBase{ device, image }
    , m_ShaderView{ view }
{

    m_Width = m_ShaderView->GetImageWidth();
    m_Height = m_ShaderView->GetImageHeight();
    m_MipsCount = 1;
    m_ArrayLayersCount = 1;
    m_Format = m_ShaderView->GetFormat();
}

StorageTexture::StorageTexture(StorageTexture&& rhs)
    : TextureBase{}
{
    operator=(DRE_MOVE(rhs));
}

StorageTexture& StorageTexture::operator=(StorageTexture&& rhs)
{
    TextureBase::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_ShaderView);

    return *this;
}

StorageTexture::~StorageTexture()
{
    if (m_ParentDevice != nullptr)
    {
        m_ParentDevice->GetResourcesController()->FreeImageView(m_ShaderView);
    }
}

}
