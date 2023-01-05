#include <vk_interface\resources\Framebuffer.hpp>

#include <foundation\Common.hpp>
#include <foundation\Container\InplaceVector.hpp>

#include <vk_interface\Constant.hpp>
#include <vk_interface\pipeline\RenderPass.hpp>
#include <vk_interface\ImportTable.hpp>
#include <vk_interface\LogicalDevice.hpp>
#include <vk_interface\Tools.hpp>

namespace VKW
{

Framebuffer::Framebuffer()
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , width_{ 0 }
    , height_{ 0 }
{
}

Framebuffer::Framebuffer(Framebuffer&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , width_{ 0 }
    , height_{ 0 }
{
    operator=(std::move(rhs));
}

Framebuffer& Framebuffer::operator=(Framebuffer&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(handle_);
    DRE_SWAP_MEMBER(width_);
    DRE_SWAP_MEMBER(height_);

    return *this;
}

Framebuffer::Framebuffer(ImportTable* table, LogicalDevice* device,
    RenderPass const& renderPass, std::uint16_t width, std::uint16_t height, std::uint8_t attachmentsCount, VKW::ImageResourceView const* attachments)
    : table_{ table }
    , device_{ device }
    , handle_{ VK_NULL_HANDLE }
    , width_{ width }
    , height_{ height }
{
    DRE_ASSERT(attachmentsCount == renderPass.GetAttachmentCount(), "Attempt to create Framebuffer with null attachments");

    DRE::InplaceVector<VkImageView, VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS> views;
    for (std::uint8_t i = 0; i < attachmentsCount; i++)
    {
        DRE_ASSERT((width <= attachments[i].GetImageWidth()) && (height <= attachments[i].GetImageHeight()), "Width of Framebuffer is more than the texture.");
        VkImageView& view = views.EmplaceBack();
        view = attachments[i].handle_;
    }

    VkFramebufferCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VK_FLAGS_NONE;
    info.renderPass = renderPass.GetHandle();
    info.attachmentCount = attachmentsCount;
    info.pAttachments = views.Data();
    info.width = width;
    info.height = height;
    info.layers = 1;

    VK_ASSERT(table_->vkCreateFramebuffer(device_->Handle(), &info, nullptr, &handle_));
}

Framebuffer::~Framebuffer()
{
    if (handle_ != VK_NULL_HANDLE)
    {
        table_->vkDestroyFramebuffer(device_->Handle(), handle_, nullptr);
    }
}

}

