#pragma once

#include <cstdint>
#include <vulkan\vulkan.h>

#include <foundation\class_features\NonCopyable.hpp>

namespace VKW
{

struct ImageResourceView;

class ImportTable;
class LogicalDevice;
class RenderPass;

class Framebuffer : public NonCopyable
{
public:
    Framebuffer();
    Framebuffer(ImportTable* table, LogicalDevice* device,
        RenderPass const& renderPass, std::uint16_t width, std::uint16_t height, std::uint8_t attachmentsCount, VKW::ImageResourceView const* attachments);

    Framebuffer(Framebuffer&& rhs);
    Framebuffer& operator=(Framebuffer&& rhs);

    inline std::uint16_t GetWidth() const { return width_; }
    inline std::uint16_t GetHeight() const { return height_; }
    inline VkFramebuffer GetHandle() const { return handle_; }

    ~Framebuffer();

private:
    ImportTable*    table_;
    LogicalDevice*         device_;

    VkFramebuffer   handle_;

    std::uint16_t   width_;
    std::uint16_t   height_;
};



}