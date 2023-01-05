#pragma once

#include <vulkan\vulkan.h>
#include <string>

#include <class_features\NonCopyable.hpp>

namespace DRE
{
class ByteBuffer;
}

namespace VKW
{

class ImportTable;
class LogicalDevice;

enum ShaderModuleType
{
    SHADER_MODULE_TYPE_NONE,
    SHADER_MODULE_TYPE_COMPUTE,
    SHADER_MODULE_TYPE_VERTEX,
    SHADER_MODULE_TYPE_FRAGMENT,
    SHADER_MODULE_TYPE_MAX
};

class ShaderModule : public NonCopyable
{
public:
    ShaderModule();
    ShaderModule(ImportTable* table, LogicalDevice* device, DRE::ByteBuffer const& byteBuffer, ShaderModuleType type, char const* entryPoint);

    ShaderModule(ShaderModule&& rhs);
    ShaderModule& operator=(ShaderModule&& rhs);

    ~ShaderModule();

    inline VkShaderModule   GetHandle() const { return handle_; }
    inline ShaderModuleType GetType() const { return type_; }
    inline char const*      GetEntryPoint() const { return entryPoint_; }

private:
    ImportTable*        table_;
    LogicalDevice*             device_;

    VkShaderModule      handle_;
    ShaderModuleType    type_;
    char                entryPoint_[32];
};

}
