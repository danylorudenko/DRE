#include <vk_interface/pipeline/ShaderModule.hpp>

#include <memory/ByteBuffer.hpp>

#include <vk_interface/Tools.hpp>
#include <vk_interface/ImportTable.hpp>
#include <vk_interface/LogicalDevice.hpp>

namespace VKW
{

ShaderModule::ShaderModule()
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , type_{ SHADER_MODULE_TYPE_NONE }
{
    entryPoint_[0] = '\0';
}

ShaderModule::ShaderModule(ImportTable* table, LogicalDevice* device, DRE::ByteBuffer const& byteBuffer, ShaderModuleType type, char const* entryPoint)
    : table_{ table}
    , device_{ device } 
    , handle_{ VK_NULL_HANDLE }
    , type_{ type }
{
    std::strcpy(entryPoint_, entryPoint);

    VkShaderModuleCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VK_FLAGS_NONE;
    info.codeSize = byteBuffer.Size();
    info.pCode = byteBuffer.As<std::uint32_t*>();

    VK_ASSERT(table_->vkCreateShaderModule(device_->Handle(), &info, nullptr, &handle_));
}

ShaderModule::ShaderModule(ShaderModule&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , type_{ SHADER_MODULE_TYPE_NONE }
{
    operator=(DRE_MOVE(rhs));
}

ShaderModule& ShaderModule::operator=(ShaderModule&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(handle_);
    DRE_SWAP_MEMBER(type_);

    std::strcpy(entryPoint_, rhs.entryPoint_);

    return *this;
}

ShaderModule::~ShaderModule()
{
    if (handle_ != VK_NULL_HANDLE)
    {
        table_->vkDestroyShaderModule(device_->Handle(), handle_, nullptr);
    }
}

}
