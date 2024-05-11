#include <gfx\texture\StorageTexture.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\resources\Resource.hpp>
#include <vk_wrapper\resources\ResourcesController.hpp>

#include <backends\imgui_impl_vulkan.h>


namespace GFX
{

StorageTexture::StorageTexture()
    : TextureBase{}
    , m_ShaderView{ nullptr }
{}

StorageTexture::StorageTexture(VKW::Device* device, VKW::ImageResource* image, VKW::ImageResourceView* view, VKW::TextureDescriptorIndex globalDescriptorHandle)
    : TextureBase{ device, image }
    , m_ShaderView{ view }
    , m_ShaderGlobalDescriptor{ globalDescriptorHandle }
{
    m_Width = m_ShaderView->GetImageWidth();
    m_Height = m_ShaderView->GetImageHeight();
    m_MipsCount = 1;
    m_ArrayLayersCount = 1;
    m_Format = m_ShaderView->GetFormat();

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    m_ImGuiDescriptorSet = VKW::DescriptorSet{ ImGui_ImplVulkan_AddTexture(m_ParentDevice->GetDescriptorManager()->GetDefaultSampler(VKW::SAMPLER_TYPE_LINEAR_REPEAT), m_ShaderView->handle_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), nullptr };
#endif
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
    DRE_SWAP_MEMBER(m_ShaderGlobalDescriptor);

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    DRE_SWAP_MEMBER(m_ImGuiDescriptorSet);
#endif

    return *this;
}

VKW::DescriptorSet StorageTexture::GetImGuiDescriptor() const
{
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    return m_ImGuiDescriptorSet;
#else
    return VKW::DescriptorSet{};
#endif
}

StorageTexture::~StorageTexture()
{
    if (m_ParentDevice != nullptr)
    {
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
        ImGui_ImplVulkan_RemoveTexture(m_ImGuiDescriptorSet.GetHandle());
        DRE_DEBUG_ONLY(m_ImGuiDescriptorSet = VKW::DescriptorSet{});
#endif

        m_ParentDevice->GetDescriptorManager()->FreeTextureDescriptor(m_ShaderGlobalDescriptor);
        m_ParentDevice->GetResourcesController()->FreeImageView(m_ShaderView);

        DRE_DEBUG_ONLY(m_ShaderGlobalDescriptor = VKW::TextureDescriptorIndex{});
        DRE_DEBUG_ONLY(m_ShaderView = nullptr);
    }
}

}
