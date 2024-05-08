#include <gfx\texture\ReadOnlyTexture.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\resources\ResourcesController.hpp>

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
#include <backends\imgui_impl_vulkan.h>
#endif

namespace GFX
{

ReadOnlyTexture::ReadOnlyTexture()
    : TextureBase{}
    , m_ShaderReadView{ nullptr }
{}

ReadOnlyTexture::ReadOnlyTexture(VKW::Device* device, VKW::ImageResource* resource, VKW::ImageResourceView* view, VKW::TextureDescriptorIndex descriptor)
    : TextureBase{ device, resource }
    , m_ShaderReadView{ view }
    , m_ShaderGlobalDescriptor{ descriptor }
{
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    m_ImGuiDescriptorSet = VKW::DescriptorSet{ ImGui_ImplVulkan_AddTexture(m_ParentDevice->GetDescriptorManager()->GetDefaultSampler(VKW::SAMPLER_TYPE_LINEAR_REPEAT), m_ShaderReadView->handle_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), nullptr };
#endif
}

ReadOnlyTexture::ReadOnlyTexture(ReadOnlyTexture&& rhs)
    : TextureBase{}
    , m_ShaderReadView{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}


ReadOnlyTexture& ReadOnlyTexture::operator=(ReadOnlyTexture&& rhs)
{
    TextureBase::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_ShaderReadView);
    DRE_SWAP_MEMBER(m_ShaderGlobalDescriptor);

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    DRE_SWAP_MEMBER(m_ImGuiDescriptorSet);
#endif

    return *this;
}

VKW::DescriptorSet ReadOnlyTexture::GetImGuiDescriptor() const
{
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    return m_ImGuiDescriptorSet;
#else
    return VKW::DescriptorSet{};
#endif
}

ReadOnlyTexture::~ReadOnlyTexture()
{
    if (m_ShaderReadView != nullptr)
    {
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
        ImGui_ImplVulkan_RemoveTexture(m_ImGuiDescriptorSet.GetHandle());
        DRE_DEBUG_ONLY(m_ImGuiDescriptorSet = VKW::DescriptorSet{});
#endif

        m_ParentDevice->GetDescriptorManager()->FreeTextureDescriptor(m_ShaderGlobalDescriptor);
        m_ParentDevice->GetResourcesController()->FreeImageView(m_ShaderReadView);

        DRE_DEBUG_ONLY(m_ShaderGlobalDescriptor = VKW::TextureDescriptorIndex{});
        DRE_DEBUG_ONLY(m_ShaderReadView = nullptr);
    }
}

}
