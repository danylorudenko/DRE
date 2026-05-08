#pragma once

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\Container\InplaceHashTable.hpp>
#include <foundation\String\InplaceString.hpp>

#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\descriptor\DescriptorLayout.hpp>

#include <gfx\renderer\Material.hpp>

namespace VKW
{
class Device;
}

namespace IO
{
class ShaderDB;
}

namespace GFX
{

class ShaderDBImpl;
class MaterialsManager;

class PipelineDB
    : public NonCopyable
    , public NonMovable
{
public:
    PipelineDB(VKW::Device* device, IO::ShaderDB* shaderDB, MaterialsManager* materialsGPUManager);
    ~PipelineDB();

    void                        AddGlobalLayouts(VKW::PipelineLayout::Descriptor& descriptor);


    VKW::DescriptorSetLayout*   CreateDescriptorSetLayout(const char* name, VKW::DescriptorSetLayout::Descriptor const& desc);
    VKW::PipelineLayout*        CreatePipelineLayout(char const* name, VKW::PipelineLayout::Descriptor const& descriptor);
    VKW::Pipeline*              CreatePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor);
    void                        CreateDefaultPipelines();

    void                        ReloadShaderFilePipelines(char const* fileName);
    void                        ReloadAllPipelines();


    GFX::Material*              CreateMaterial(char const* name, GFX::Material::Type type);


    VKW::PipelineLayout const*  GetGlobalLayout() const;
    VKW::PipelineLayout*        GetLayout(char const* name);
    VKW::Pipeline*              GetPipeline(char const* name);
    VKW::DescriptorSetLayout*   GetSetLayout(char const* name);

    // will find all passed modules and combine all their layouts into one with name "{name}_layout"
    DRE::String64 const*        CreatePipelineLayoutFromShader(char const* shaderName, 
        char const* vertexName, 
        char const* fragmentName, 
        char const* computeName);

private:
    DRE::String64 const*    CreateGraphicsForwardPipeline(char const* name, char const* vertName, char const* fragName);
    DRE::String64 const*    CreateGraphicsGBufferPipeline(char const* name, char const* vertName, char const* fragName);
    DRE::String64 const*    CreateGraphicsForwardWaterPipeline(char const* name, char const* vertName, char const* fragName);
    DRE::String64 const*    CreateGraphicsForwardShadowPipeline(char const* name, char const* vertName);
    DRE::String64 const*    CreateComputePipeline(char const* name, char const* compName);
    DRE::String64 const*    CreateCustomGraphicsPipeline(char const* name, char const* vertName, char const* fragName, VKW::Pipeline::Descriptor& descriptor);
    DRE::String64 const*    CreateCustomComputePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor);

    DRE::String64 const*    CreateGraphicsGizmoPipeline(char const* name, char const* vertName, char const* fragName);

    void                    RecreatePipeline(char const* name);

    static void             AddDREVertexAttributes(VKW::Pipeline::Descriptor& descriptor);

private:
    VKW::Device*            m_Device;
    IO::ShaderDB*           m_ShaderDB;
    GFX::MaterialsManager*  m_MaterialsGPUManager;

    using ShaderLayoutsMap = DRE::InplaceHashTable<DRE::String64, DRE::InplaceVector<VKW::DescriptorSetLayout, VKW::CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS - 3>>;

    ShaderLayoutsMap                                                m_ShaderLayouts;
    DRE::InplaceHashTable<DRE::String64, VKW::DescriptorSetLayout>  m_SetLayouts;
    DRE::InplaceHashTable<DRE::String64, VKW::PipelineLayout>       m_PipelineLayouts;
    DRE::InplaceHashTable<DRE::String64, VKW::Pipeline>             m_Pipelines;

    // shader entry name -> names of pipelines that use it
    DRE::InplaceHashTable<DRE::String64, DRE::InplaceVector<DRE::String64, 16>> m_ShaderToPipelines;

    DRE::InplaceHashTable<DRE::String64, GFX::Material>             m_Materials;
};

}

