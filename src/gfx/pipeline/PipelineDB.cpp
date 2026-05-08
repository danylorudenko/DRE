#include <gfx\pipeline\PipelineDB.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>

#include <engine\io\ShaderDB.hpp>

#include <common\forward.slang>


namespace GFX
{

/////////////////////////////////
// PipelineDB
PipelineDB::PipelineDB(VKW::Device* device, IO::ShaderDB* shaderDB, MaterialsManager* materialsGPUManager)
    : m_Device{ device }
    , m_ShaderDB{ shaderDB }
    , m_MaterialsGPUManager{ materialsGPUManager }
{
}

PipelineDB::~PipelineDB()
{
    m_Pipelines.Clear();
    m_PipelineLayouts.Clear();
    m_SetLayouts.Clear();
    m_ShaderLayouts.Clear();
}

void PipelineDB::CreateDefaultPipelines()
{
    // default plane material shader
    {
        CreateGraphicsForwardPipeline("forward_pbr_main", "forward_pbr_main_vert", "forward_pbr_main_frag");

        CreateGraphicsGBufferPipeline("gbuffer_pbr_main", "gbuffer_pbr_main_vert", "gbuffer_pbr_main_frag");

        CreateGraphicsGizmoPipeline("gizmo_3D_main", "gizmo_3D_main_vert", "gizmo_3D_main_frag");

        CreateComputePipeline("lighting_deferred_main", "lighting_deferred_main_comp");
        CreateComputePipeline("color_encode_main", "color_encode_main_comp");
        CreateComputePipeline("ambient_occlusion_main", "ambient_occlusion_main_comp");
        CreateComputePipeline("temporal_AA_main", "temporal_AA_main_comp");
        CreateComputePipeline("gen_butterfly_main", "gen_butterfly_main_comp");
        CreateComputePipeline("gen_h0_main", "gen_h0_main_comp");
        CreateComputePipeline("gen_hxt_main", "gen_hxt_main_comp");
        CreateComputePipeline("fft_iter_main", "fft_iter_main_comp");
        CreateComputePipeline("fft_inv_perm_main", "fft_inv_perm_main_comp");
        CreateComputePipeline("debug_view_texture_main", "debug_view_texture_main_comp");
        CreateComputePipeline("debug_view_ddgi_probes_main", "debug_view_ddgi_probes_main_comp");

        // DDGI
        CreateComputePipeline("ddgi_probe_border_blend_main", "ddgi_probe_border_blend_main_comp");
        CreateComputePipeline("ddgi_probe_lighting_main", "ddgi_probe_lighting_main_comp");
        CreateComputePipeline("ddgi_probe_scatter_main", "ddgi_probe_scatter_main_comp");
        CreateComputePipeline("ddgi_probe_trace_main", "ddgi_probe_trace_main_comp");

        VKW::Pipeline::Descriptor waterCausticDesc;
        waterCausticDesc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
        //waterCausticDesc.EnableDepthTest(g_GraphicsManager->GetMainDepthFormat(), false);
        AddDREVertexAttributes(waterCausticDesc);
        waterCausticDesc.AddColorOutput(VKW::FORMAT_R8_UNORM);
        CreateCustomGraphicsPipeline("water_caustics_main", "water_caustics_main_vert", "water_caustics_main_frag", waterCausticDesc);

        VKW::Pipeline::Descriptor shadowDesc;
        shadowDesc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
        shadowDesc.EnableDepthTest(VKW::FORMAT_D16_UNORM);
        shadowDesc.AddColorOutput(VKW::FORMAT_R16G16B16A16_FLOAT);
        AddDREVertexAttributes(shadowDesc);
        CreateCustomGraphicsPipeline("forward_shadow_main", "forward_shadow_main_vert", "forward_shadow_main_frag", shadowDesc);
    }
}

void PipelineDB::AddDREVertexAttributes(VKW::Pipeline::Descriptor& desc)
{
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // pos
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // norm
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // tan
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // btan
    desc.AddVertexAttribute(VKW::FORMAT_R32G32_FLOAT);    // uv
}

DRE::String64 const* PipelineDB::CreateCustomGraphicsPipeline(char const* name, char const* vertName, char const* fragName, VKW::Pipeline::Descriptor& descriptor)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName)->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName)->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, vertName };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName };

    descriptor.SetVertexShader(vertModule);
    descriptor.SetFragmentShader(fragModule);
    descriptor.SetLayout(GetLayout(layoutName->GetData()));
    descriptor.SetCullMode(VK_CULL_MODE_BACK_BIT);

    CreatePipeline(name, descriptor);
    return m_Pipelines.Find(name).key;

}

DRE::String64 const* PipelineDB::CreateGraphicsForwardPipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName)->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName)->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, vertName };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetFragmentShader(fragModule);
    desc.SetLayout(GetLayout(layoutName->GetData()));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    desc.EnableDepthTest(g_GraphicsManager->GetMainDepthFormat());
    desc.AddColorOutput(g_GraphicsManager->GetMainColorFormat()); // main color
    desc.AddColorOutput(g_GraphicsManager->GetVelocityBufferFormat()); // velocity vectors
    desc.AddColorOutput(VKW::FORMAT_B8G8R8A8_UNORM);              // object IDs
    static_assert(FORWARD_PASS_OUTPUT_COUNT == 3, "Don't forget to modify PipelineDB and ForwardOpaquePass");

    AddDREVertexAttributes(desc);

    CreatePipeline(name, desc);
    return m_Pipelines.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsGBufferPipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName)->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName)->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, vertName };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetFragmentShader(fragModule);
    desc.SetLayout(GetLayout(layoutName->GetData()));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    desc.EnableDepthTest(g_GraphicsManager->GetMainDepthFormat());

    auto gBufferFormats = g_GraphicsManager->GetGBufferFormats();
    static_assert(gBufferFormats.size() == 4, "Don't forget this");

    desc.AddColorOutput(gBufferFormats[0]); // diffuse_roughness
    desc.AddColorOutput(gBufferFormats[1]); // normal_metalness
    desc.AddColorOutput(gBufferFormats[2]); // velocity
    desc.AddColorOutput(gBufferFormats[3]); // objectID
    desc.AddColorOutput(VKW::FORMAT_R32G32B32A32_FLOAT); // DEBUG_TEXTURE

    AddDREVertexAttributes(desc);

    CreatePipeline(name, desc);
    return m_Pipelines.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsForwardWaterPipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName)->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName)->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, vertName };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetFragmentShader(fragModule);
    desc.SetLayout(GetLayout(layoutName->GetData()));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    //desc.SetPolygonMode(VK_POLYGON_MODE_LINE);
    desc.EnableDepthTest(g_GraphicsManager->GetMainDepthFormat(), false);
    desc.AddColorOutput(g_GraphicsManager->GetMainColorFormat());
    desc.AddColorOutput(VKW::FORMAT_R16G16_FLOAT); // velocity vectors

    AddDREVertexAttributes(desc);

    CreatePipeline(name, desc);
    return m_Pipelines.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsForwardShadowPipeline(char const* name, char const* vertName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, nullptr, nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName)->spirv;
    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, vertName };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetLayout(GetLayout(layoutName->GetData()));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    desc.EnableDepthTest(g_GraphicsManager->GetMainDepthFormat());

    AddDREVertexAttributes(desc);

    CreatePipeline(name, desc);
    return m_Pipelines.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateComputePipeline(char const* name, char const* compName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, nullptr, nullptr, compName);
    VKW::PipelineLayout* layout = GetLayout(layoutName->GetData());
    DRE_ASSERT(layout != nullptr, "Can't find pipeline!");

    IO::ShaderEntry const* shaderData = m_ShaderDB->GetShaderEntry(compName);
    VKW::ShaderModule compModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), shaderData->spirv, shaderData->type, compName };

    VKW::Pipeline::Descriptor desc;
    desc.SetPipelineType(VKW::PIPELINE_TYPE_COMPUTE);
    desc.SetComputeShader(compModule);
    desc.SetLayout(layout);

    CreatePipeline(name, desc);
    return m_Pipelines.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsGizmoPipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName)->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName)->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, vertName };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetFragmentShader(fragModule);
    desc.SetLayout(GetLayout(layoutName->GetData()));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    desc.AddColorOutput(g_GraphicsManager->GetMainColorFormat());

    AddDREVertexAttributes(desc);

    CreatePipeline(name, desc);

    return nullptr;
}

void PipelineDB::ReloadShaderFilePipelines(char const* shaderFileName)
{
    IO::ShaderFile const* shaderFile = m_ShaderDB->GetShaderFile(shaderFileName);
    DRE_ASSERT(shaderFile != nullptr, "Attempt to reload pipelines for non-existing shader file");

    if (!m_ShaderDB->CompileShaderFile(shaderFile->fileName.GetData()))
    {
        std::cout << "Failed to recompile shader file" << shaderFile->fileName.GetData() << ". Pipelines were not recreated." << std::endl;
        return;
    }

    DRE::InplaceVector<DRE::String64, 16> pipelinesToReload;

    for (DRE::U32 i = 0, count = shaderFile->shaderEntries.Size(); i < count; i++)
    {
        DRE::String64 const& shaderEntryName = shaderFile->shaderEntries[i];

        auto pipelinesIt = m_ShaderToPipelines.Find(shaderEntryName);
        if (pipelinesIt.value == nullptr)
            continue;

        for (DRE::U32 j = 0, pCount = pipelinesIt.value->Size(); j < pCount; j++)
        {
            pipelinesToReload.EmplaceBackUnique((*pipelinesIt.value)[j]);
        }
    }

    for (DRE::U32 i = 0, count = pipelinesToReload.Size(); i < count; i++)
    {
        RecreatePipeline(pipelinesToReload[i].GetData());
    }
}

void PipelineDB::RecreatePipeline(char const* name)
{
    VKW::Pipeline* pipeline = GetPipeline(name);
    DRE_ASSERT(pipeline != nullptr, "Attempt to recreate pipeline which did not exist.");

    DRE::U8 const shaderStageCount = pipeline->GetDescriptor().GetShaderStageCount();
    for (DRE::U8 s = 0; s < shaderStageCount; s++)
    {
        DRE::String64 const& stageName = pipeline->GetDescriptor().GetShaderStageName(s);
        IO::ShaderEntry const* shaderEntry = m_ShaderDB->GetShaderEntry(stageName.GetData());
        VKW::ShaderModule shaderModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), shaderEntry->spirv, shaderEntry->type, shaderEntry->name.GetData() };

        switch (shaderEntry->type)
        {
        case VKW::SHADER_MODULE_TYPE_VERTEX:
            pipeline->GetDescriptor().SetVertexShader(shaderModule);
            break;
        case VKW::SHADER_MODULE_TYPE_FRAGMENT:
            pipeline->GetDescriptor().SetFragmentShader(shaderModule);
            break;
        case VKW::SHADER_MODULE_TYPE_COMPUTE:
            pipeline->GetDescriptor().SetComputeShader(shaderModule);
            break;
        }
    }

    VKW::Pipeline::Descriptor& desc = pipeline->GetDescriptor();
    m_Pipelines[DRE::String64{ name }] = VKW::Pipeline{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), desc, name };
}

void PipelineDB::ReloadAllPipelines()
{
    m_ShaderDB->CompileSources(false);

    m_Pipelines.ForEach([this](auto pair)
    {
        RecreatePipeline(pair.key->GetData());
    });
}

DRE::String64 const* PipelineDB::CreatePipelineLayoutFromShader(char const* shaderName,
    char const* vertName,
    char const* fragName,
    char const* compName)
{
    if ((vertName != nullptr || fragName != nullptr) && compName != nullptr)
    {
        DRE_ASSERT(false, "Attempt to create pipeline from both graphics and compute shaders.");
    }

    IO::ShaderEntry const* vertShader = vertName != nullptr ? m_ShaderDB->GetShaderEntry(vertName) : nullptr;
    IO::ShaderEntry const* fragShader = fragName != nullptr ? m_ShaderDB->GetShaderEntry(fragName) : nullptr;
    IO::ShaderEntry const* compShader = compName != nullptr ? m_ShaderDB->GetShaderEntry(compName) : nullptr;

    IO::ShaderInterface shaderInterface;
    if (vertShader != nullptr)
    {
        shaderInterface.Merge(vertShader->bindingInterface);
    }

    if (fragShader != nullptr)
    {
        shaderInterface.Merge(fragShader->bindingInterface);
    }

    if (compShader != nullptr)
    {
        shaderInterface.Merge(compShader->bindingInterface);
    }

    shaderInterface.m_Members.SortBubble([](auto const& lhs, auto const& rhs) {
        return (lhs.set <= rhs.set);
    });

    std::uint32_t const globalLayoutsCount = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();

    // next used set after global descriptor sets
    std::uint32_t const startMemberId = shaderInterface.m_Members.FindIf([globalLayoutsCount](auto const& data) { return data.set >= globalLayoutsCount; });

    auto& layouts = m_ShaderLayouts[shaderName];
    if (startMemberId != shaderInterface.m_Members.Size())
    {
        std::uint8_t prevSet = shaderInterface.m_Members[startMemberId - 1].set;
        std::uint8_t currentSet = shaderInterface.m_Members[startMemberId].set;
        DRE_ASSERT(currentSet == prevSet + 1, "Descriptor sets must be continuous");

        VKW::DescriptorSetLayout::Descriptor setLayoutDesc{};

        for (std::uint32_t i = startMemberId, size = shaderInterface.m_Members.Size(); i < size; i++)
        {
            auto const& m = shaderInterface.m_Members[i];
            if (m.set != currentSet)
            {
                layouts.EmplaceBack(g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), setLayoutDesc);
                setLayoutDesc = VKW::DescriptorSetLayout::Descriptor{};
                prevSet = currentSet;
                currentSet = m.set;
                DRE_ASSERT(currentSet == prevSet + 1, "Descriptor sets must be continuous");
            }

            if (m.arraySize != DRE_U8_MAX)
                setLayoutDesc.Add(m.type, m.binding, m.stage, m.arraySize);
            else
                setLayoutDesc.AddVariableCount(m.type, m.binding, m.stage, VKW::CONSTANTS::TEXTURE_DESCRIPTOR_HEAP_SIZE);
        }
        layouts.EmplaceBack(g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), setLayoutDesc);

    }

    VKW::PipelineLayout::Descriptor layoutDesc;
    AddGlobalLayouts(layoutDesc);
    for (std::uint32_t i = 0, size = layouts.Size(); i < size; i++)
    {
        layoutDesc.Add(&layouts[i]);
    }

    DRE::String64 layoutName{ shaderName }; layoutName.Append("_layout");
    CreatePipelineLayout(layoutName.GetData(), layoutDesc);

    return m_PipelineLayouts.Find(layoutName).key;
}

void PipelineDB::AddGlobalLayouts(VKW::PipelineLayout::Descriptor& descriptor)
{
    DRE_ASSERT(descriptor.GetSetCount() == 0, "Pipeline layout descriptor must be empty before filling default layouts.");

    VKW::DescriptorManager* allocator = m_Device->GetDescriptorManager();

    for (std::uint32_t i = 0, count = allocator->GetGlobalSetLayoutsCount(); i < count; i++)
    {
        descriptor.Add(&allocator->GetGlobalSetLayout(i));
    }

    descriptor.AddPushConstant(4, VKW::DESCRIPTOR_STAGE_ALL);
}

VKW::PipelineLayout* PipelineDB::CreatePipelineLayout(char const* name, VKW::PipelineLayout::Descriptor const& descriptor)
{
    DRE_ASSERT(descriptor.GetLayout(0) == &m_Device->GetDescriptorManager()->GetGlobalSetLayout(0), "Invalid global layout in PipelineDB.");
    DRE_ASSERT(descriptor.GetLayout(1) == &m_Device->GetDescriptorManager()->GetGlobalSetLayout(1), "Invalid global layout in PipelineDB.");
    DRE_ASSERT(descriptor.GetLayout(2) == &m_Device->GetDescriptorManager()->GetGlobalSetLayout(2), "Invalid global layout in PipelineDB.");
    DRE_ASSERT(descriptor.GetPushConstantsCount() == 1, "Each pipeline must have a push constant");
    DRE_ASSERT(descriptor.GetPushConstant(0).size == 4, "Default size for push constant must be 4");
    DRE_ASSERT(descriptor.GetPushConstant(0).stageFlags == VKW::HELPER::DescriptorStageToVK(VKW::DESCRIPTOR_STAGE_ALL), "DescriptorStage for push constant must be VKW::DESCRIPTOR_STAGE_ALL");

    return &(m_PipelineLayouts.Emplace(name, m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor));
}

VKW::DescriptorSetLayout* PipelineDB::CreateDescriptorSetLayout(const char* name, VKW::DescriptorSetLayout::Descriptor const& desc)
{
    return &(m_SetLayouts[name] = VKW::DescriptorSetLayout{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), desc });
}

VKW::Pipeline* PipelineDB::CreatePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor)
{
    for (DRE::U8 i = 0, count = descriptor.GetShaderStageCount(); i < count; i++)
    {
        m_ShaderToPipelines[descriptor.GetShaderStageName(i)].EmplaceBackUnique(DRE::String64{ name });
    }

    return &(m_Pipelines.Emplace(name, m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor, name));
}

VKW::PipelineLayout const* PipelineDB::GetGlobalLayout() const
{
    return m_Device->GetDescriptorManager()->GetGlobalPipelineLayout();
}

VKW::PipelineLayout* PipelineDB::GetLayout(char const* name)
{
    return m_PipelineLayouts.Find(name).value;
}

VKW::Pipeline* PipelineDB::GetPipeline(char const* name)
{
    return m_Pipelines.Find(name).value;
}

VKW::DescriptorSetLayout* PipelineDB::GetSetLayout(char const* name)
{
    return &m_SetLayouts[name];
}

GFX::Material* PipelineDB::CreateMaterial(char const* name, GFX::Material::Type type)
{
    MaterialsManager::MaterialGPU materialGPU = m_MaterialsGPUManager->AllocateMaterial();
    return &m_Materials.Emplace(name, type, materialGPU);
}

}

