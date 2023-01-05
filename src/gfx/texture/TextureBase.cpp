#include <gfx\texture\TextureBase.hpp>

#include <foundation\Common.hpp>

#include <vk_interface\Device.hpp>
#include <vk_interface\resources\Resource.hpp>
#include <vk_interface\resources\ResourcesController.hpp>


namespace GFX
{

TextureBase::TextureBase()
    : DeviceChild{ nullptr }
    , m_Image{ nullptr }
    , m_Width{ 0 }
    , m_Height{ 0 }
    , m_MipsCount{ 0 }
    , m_ArrayLayersCount{ 0 }
    , m_Format{ VKW::FORMAT_UNDEFINED }
{
}

TextureBase::TextureBase(VKW::Device* device, VKW::ImageResource* image)
    : DeviceChild{ device }
    , m_Image{ image }
    , m_Width{ image->width_ }
    , m_Height{ image->height_ }
    , m_MipsCount{ 1 }
    , m_ArrayLayersCount{ 1 }
    , m_Format{ image->format_ }
{

}

TextureBase::TextureBase(TextureBase&& rhs)
    : DeviceChild{ nullptr }
    , m_Image{ nullptr }
    , m_Width{ 0 }
    , m_Height{ 0 }
    , m_MipsCount{ 0 }
    , m_ArrayLayersCount{ 0 }
    , m_Format{ VKW::FORMAT_UNDEFINED }
{
    operator=(DRE_MOVE(rhs));
}

TextureBase& TextureBase::operator=(TextureBase&& rhs)
{
    DeviceChild::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_Image);
    DRE_SWAP_MEMBER(m_Width);
    DRE_SWAP_MEMBER(m_Height);
    DRE_SWAP_MEMBER(m_MipsCount);
    DRE_SWAP_MEMBER(m_ArrayLayersCount);
    DRE_SWAP_MEMBER(m_Format);

    return *this;
}

TextureBase::~TextureBase()
{
    if (m_ParentDevice != nullptr)
    {
        m_ParentDevice->GetResourcesController()->FreeImage(m_Image);
        DRE_DEBUG_ONLY(m_Image = nullptr);
    }
}

}
