#pragma once

#include <foundation\Common.hpp>
#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <foundation\container\InplaceVector.hpp>
#include <foundation\container\InplaceHashTable.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\string\InplaceString.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>
#include <vk_wrapper\descriptor\Descriptor.hpp>


namespace IO
{

class IOManager;
class ShaderDBImpl;

struct ShaderInterface
{
    struct Member
    {
        VKW::DescriptorType type;
        VKW::DescriptorStage stage;
        std::uint8_t set;
        std::uint8_t binding;
        std::uint8_t arraySize;

        bool operator==(Member const& rhs) const;
        bool operator!=(Member const& rhs) const;
    };

    DRE::InplaceVector<Member, 16> m_Members;
    std::uint8_t m_PushConstantSize : 7 = 0;
    std::uint8_t m_PushConstantPresent : 1 = 0;
    VKW::DescriptorStage m_PushConstantStages = VKW::DESCRIPTOR_STAGE_NONE;

    void Merge(ShaderInterface const& rhs);
};

struct ShaderEntry
{
    DRE::String64           name;
    DRE::String64           entryPoint;
    VKW::ShaderModuleType   type;
    DRE::ByteBuffer         spirv;
    DRE::ByteBuffer         source;
    ShaderInterface         bindingInterface;
};

struct ShaderFile
{
    DRE::String64 fileName;
    DRE::InplaceVector<DRE::String64, 8> shaderEntries;
};

/////////////////////////////////
// ShaderModuleDB
class ShaderDB
    : public NonMovable
    , public NonCopyable
{
public:
    ShaderDB(IO::IOManager* io);
    ~ShaderDB();

    void                    CompileSources(bool parallel);

    bool                    CompileShaderFile(DRE::String64 const& name);

    ShaderFile const*       GetShaderFile(DRE::String128 const& name);
    ShaderEntry const*      GetShaderEntry(DRE::String128 const& name);

    static DRE::String128   BuildShaderPath(char const* shaderFile);

    bool                    AreNewShadersPending() const;
    void                    ClearPendingShaders();

    // move-returns pending shaders. Pending shaders are automatically "cleared" after this call
    DRE::InplaceVector<DRE::String128, 12> ReadAndClearPendingShaderFilesCopy();

    DRE::InplaceHashTable<DRE::String128, DRE::String128, 512> const& GetShaderToFileMap() const;

private:
    ShaderDBImpl* m_Impl;
};

}