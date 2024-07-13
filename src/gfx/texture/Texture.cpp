#include <gfx\texture\Texture.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\resources\Resource.hpp>
#include <vk_wrapper\resources\ResourcesController.hpp>

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
#include <backends\imgui_impl_vulkan.h>
#endif



namespace GFX
{

Texture::Texture()
    : DeviceChild{ nullptr }
    , m_Image{ nullptr }
    , m_Width{ 0 }
    , m_Height{ 0 }
    , m_MipsCount{ 0 }
    , m_ArrayLayersCount{ 0 }
    , m_Format{ VKW::FORMAT_UNDEFINED }
    , m_ShaderView{ nullptr }
{
}

Texture::Texture(VKW::Device* device, VKW::ImageResource* image, VKW::ImageResourceView* view, VKW::TextureDescriptorIndex globalDescriptorHandle)
    : DeviceChild{ device }
    , m_Image{ image }
    , m_Width{ image->width_ }
    , m_Height{ image->height_ }
    , m_MipsCount{ 1 }
    , m_ArrayLayersCount{ 1 }
    , m_Format{ image->format_ }
    , m_ShaderView{ view }
    , m_ShaderGlobalDescriptor{ globalDescriptorHandle }
{
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    m_ImGuiDescriptorSet = VKW::DescriptorSet{ ImGui_ImplVulkan_AddTexture(m_ParentDevice->GetDescriptorManager()->GetDefaultSampler(VKW::SAMPLER_TYPE_LINEAR_REPEAT), m_ShaderView->handle_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), nullptr };
#endif
}

Texture::Texture(Texture&& rhs)
    : DeviceChild{ nullptr }
    , m_Image{ nullptr }
    , m_Width{ 0 }
    , m_Height{ 0 }
    , m_MipsCount{ 0 }
    , m_ArrayLayersCount{ 0 }
    , m_Format{ VKW::FORMAT_UNDEFINED }
    , m_ShaderView{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

Texture& Texture::operator=(Texture&& rhs)
{
    DeviceChild::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_Image);
    DRE_SWAP_MEMBER(m_Width);
    DRE_SWAP_MEMBER(m_Height);
    DRE_SWAP_MEMBER(m_MipsCount);
    DRE_SWAP_MEMBER(m_ArrayLayersCount);
    DRE_SWAP_MEMBER(m_Format);

    DRE_SWAP_MEMBER(m_ShaderView);
    DRE_SWAP_MEMBER(m_ShaderGlobalDescriptor);

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    DRE_SWAP_MEMBER(m_ImGuiDescriptorSet);
#endif

    return *this;
}


VKW::DescriptorSet Texture::GetImGuiDescriptor() const
{
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    return m_ImGuiDescriptorSet;
#else
    return VKW::DescriptorSet{};
#endif
}

Texture::~Texture()
{
    if (m_ParentDevice != nullptr)
    {
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
        ImGui_ImplVulkan_RemoveTexture(m_ImGuiDescriptorSet.GetHandle());
        DRE_DEBUG_ONLY(m_ImGuiDescriptorSet = VKW::DescriptorSet{});
#endif
        
        m_ParentDevice->GetDescriptorManager()->FreeTextureDescriptor(m_ShaderGlobalDescriptor);
        m_ParentDevice->GetResourcesController()->FreeImageView(m_ShaderView);
        m_ParentDevice->GetResourcesController()->FreeImage(m_Image);

        DRE_DEBUG_ONLY(m_ShaderGlobalDescriptor = VKW::TextureDescriptorIndex{});
        DRE_DEBUG_ONLY(m_ShaderView = nullptr);
        DRE_DEBUG_ONLY(m_Image = nullptr);

    }
}

}
