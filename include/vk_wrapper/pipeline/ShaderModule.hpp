#pragma once

#include <vulkan\vulkan.h>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\string\InplaceString.hpp>

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
    ShaderModule(ImportTable* table, LogicalDevice* device, DRE::ByteBuffer const& byteBuffer, ShaderModuleType type, char const* name);

    ShaderModule(ShaderModule&& rhs);
    ShaderModule& operator=(ShaderModule&& rhs);

    ~ShaderModule();

    inline VkShaderModule   GetHandle() const { return handle_; }
    inline ShaderModuleType GetType() const { return type_; }
    inline char const*      GetName() const { return name_.GetData(); }

private:
    ImportTable*        table_;
    LogicalDevice*      device_;

    DRE::String64       name_;

    VkShaderModule      handle_;
    ShaderModuleType    type_;
};

}
