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
PipelineDB::PipelineDB(VKW::Device* device, IO::ShaderDB* shaderDB)
    : m_Device{ device }
    , m_ShaderDB{ shaderDB }
{
}

PipelineDB::~PipelineDB()
{
    m_PipelineEntries.Clear();
    m_PipelineLayouts.Clear();
    m_SetLayouts.Clear();
    m_ShaderLayouts.Clear();
}

void PipelineDB::CreateDefaultPipelines()
{
    // default plane material shader
    {
        CreateGraphicsForwardPipeline("forward_pbr", "forward_pbr.slang_mainVS", "forward_pbr.slang_mainPS");

        CreateGraphicsGBufferPipeline("gbuffer_pbr", "gbuffer_pbr.slang_mainVS", "gbuffer_pbr.slang_mainPS");

        CreateGraphicsGizmoPipeline("gizmo_3D", "gizmo_3D.slang_mainVS", "gizmo_3D.slang_mainPS");
        CreateComputePipeline("lighting_deferred", "lighting_deferred.slang_mainCS");
        CreateComputePipeline("color_encode", "color_encode.slang_mainCS");
        CreateComputePipeline("ambient_occlusion", "ambient_occlusion.slang_mainCS");
        CreateComputePipeline("temporal_AA", "temporal_AA.slang_mainCS");
        CreateComputePipeline("gen_butterfly", "gen_butterfly.slang_mainCS");
        CreateComputePipeline("gen_h0", "gen_h0.slang_mainCS");
        CreateComputePipeline("gen_hxt", "gen_hxt.slang_mainCS");
        CreateComputePipeline("fft_iter", "fft_iter.slang_mainCS");
        CreateComputePipeline("fft_inv_perm", "fft_inv_perm.slang_mainCS");
        CreateComputePipeline("debug_view_texture", "debug_view_texture.slang_mainCS");

        // DDGI
        CreateComputePipeline("ddgi_probe_border_blend", "ddgi_probe_border_blend.slang_mainCS");
        CreateComputePipeline("ddgi_probe_lighting", "ddgi_probe_lighting.slang_mainCS");
        CreateComputePipeline("ddgi_probe_scatter", "ddgi_probe_scatter.slang_mainCS");
        CreateComputePipeline("ddgi_probe_trace", "ddgi_probe_trace.slang_mainCS");
        CreateComputePipeline("debug_view_ddgi_probes_args", "debug_view_ddgi_probes.slang_indirectArgsFillCS");
        CreateGraphicsGBufferDDGIProbePipeline("debug_view_ddgi_probes_draw", "debug_view_ddgi_probes.slang_drawVS", "debug_view_ddgi_probes.slang_drawPS");

        VKW::Pipeline::Descriptor waterCausticDesc;
        waterCausticDesc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
        //waterCausticDesc.EnableDepthTest(g_GraphicsManager->GetMainDepthFormat(), false);
        AddDREVertexAttributes(waterCausticDesc);
        waterCausticDesc.AddColorOutput(VKW::FORMAT_R8_UNORM);
        CreateCustomGraphicsPipeline("water_caustics", "water_caustics.slang_mainVS", "water_caustics.slang_mainPS", waterCausticDesc);

        VKW::Pipeline::Descriptor shadowDesc;
        shadowDesc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
        shadowDesc.EnableDepthTest(VKW::FORMAT_D16_UNORM);
        shadowDesc.AddColorOutput(VKW::FORMAT_R16G16B16A16_FLOAT);
        AddDREVertexAttributes(shadowDesc);
        CreateCustomGraphicsPipeline("forward_shadow", "forward_shadow.slang_mainVS", "forward_shadow.slang_mainPS", shadowDesc);
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

    IO::ShaderEntry const* vertexShaderEntry = m_ShaderDB->GetShaderEntry(vertName);
    IO::ShaderEntry const* fragmentShaderEntry = m_ShaderDB->GetShaderEntry(fragName);

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_VERTEX, vertName, vertexShaderEntry->entryPoint };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragmentShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName, fragmentShaderEntry->entryPoint };

    descriptor.SetVertexShader(vertModule);
    descriptor.SetFragmentShader(fragModule);
    descriptor.SetLayout(GetLayout(layoutName->GetData()));
    descriptor.SetCullMode(VK_CULL_MODE_BACK_BIT);

    PipelineEntry::ShaderEntries shaderEntries;
    shaderEntries.EmplaceBack(vertexShaderEntry);
    shaderEntries.EmplaceBack(fragmentShaderEntry);
    PipelineEntry* entry = CreatePipeline(name, descriptor, GetLayout(layoutName->GetData()), DRE_MOVE(shaderEntries));
    (void)entry;
    return m_PipelineEntries.Find(name).key;

}

DRE::String64 const* PipelineDB::CreateGraphicsForwardPipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    IO::ShaderEntry const* vertexShaderEntry = m_ShaderDB->GetShaderEntry(vertName);
    IO::ShaderEntry const* fragmentShaderEntry = m_ShaderDB->GetShaderEntry(fragName);

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_VERTEX, vertName, vertexShaderEntry->entryPoint };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragmentShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName,fragmentShaderEntry->entryPoint };

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

    PipelineEntry::ShaderEntries shaderEntries;
    shaderEntries.EmplaceBack(vertexShaderEntry);
    shaderEntries.EmplaceBack(fragmentShaderEntry);
    CreatePipeline(name, desc, GetLayout(layoutName->GetData()), DRE_MOVE(shaderEntries));
    return m_PipelineEntries.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsGBufferPipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    IO::ShaderEntry const* vertexShaderEntry = m_ShaderDB->GetShaderEntry(vertName);
    IO::ShaderEntry const* fragmentShaderEntry = m_ShaderDB->GetShaderEntry(fragName);

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_VERTEX, vertName, vertexShaderEntry->entryPoint };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragmentShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName, fragmentShaderEntry->entryPoint };

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

    PipelineEntry::ShaderEntries shaderEntries;
    shaderEntries.EmplaceBack(vertexShaderEntry);
    shaderEntries.EmplaceBack(fragmentShaderEntry);
    CreatePipeline(name, desc, GetLayout(layoutName->GetData()), DRE_MOVE(shaderEntries));
    return m_PipelineEntries.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsGBufferDDGIProbePipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    IO::ShaderEntry const* vertexShaderEntry = m_ShaderDB->GetShaderEntry(vertName);
    IO::ShaderEntry const* fragmentShaderEntry = m_ShaderDB->GetShaderEntry(fragName);

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_VERTEX, vertName, vertexShaderEntry->entryPoint };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragmentShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName, fragmentShaderEntry->entryPoint };

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
    desc.AddColorOutput(gBufferFormats[3], VKW::BLEND_TYPE_NONE, 0); // objectID
    desc.AddColorOutput(VKW::FORMAT_R32G32B32A32_FLOAT, VKW::BLEND_TYPE_NONE, 0); // DEBUG_TEXTURE

    AddDREVertexAttributes(desc);

    PipelineEntry::ShaderEntries shaderEntries;
    shaderEntries.EmplaceBack(vertexShaderEntry);
    shaderEntries.EmplaceBack(fragmentShaderEntry);
    CreatePipeline(name, desc, GetLayout(layoutName->GetData()), DRE_MOVE(shaderEntries));
    return m_PipelineEntries.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsForwardWaterPipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    IO::ShaderEntry const* vertexShaderEntry = m_ShaderDB->GetShaderEntry(vertName);
    IO::ShaderEntry const* fragmentShaderEntry = m_ShaderDB->GetShaderEntry(fragName);

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_VERTEX, vertName, vertexShaderEntry->entryPoint };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragmentShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName, fragmentShaderEntry->entryPoint };

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

    PipelineEntry::ShaderEntries shaderEntries;
    shaderEntries.EmplaceBack(vertexShaderEntry);
    shaderEntries.EmplaceBack(fragmentShaderEntry);
    CreatePipeline(name, desc, GetLayout(layoutName->GetData()), DRE_MOVE(shaderEntries));
    return m_PipelineEntries.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsForwardShadowPipeline(char const* name, char const* vertName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, nullptr, nullptr);

    IO::ShaderEntry const* vertexShaderEntry = m_ShaderDB->GetShaderEntry(vertName);
    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_VERTEX, vertName, vertexShaderEntry->entryPoint };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetLayout(GetLayout(layoutName->GetData()));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    desc.EnableDepthTest(g_GraphicsManager->GetMainDepthFormat());

    AddDREVertexAttributes(desc);

    PipelineEntry::ShaderEntries shaderEntries;
    shaderEntries.EmplaceBack(vertexShaderEntry);
    CreatePipeline(name, desc, GetLayout(layoutName->GetData()), DRE_MOVE(shaderEntries));
    return m_PipelineEntries.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateComputePipeline(char const* name, char const* compName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, nullptr, nullptr, compName);
    VKW::PipelineLayout* layout = GetLayout(layoutName->GetData());
    DRE_ASSERT(layout != nullptr, "Can't find pipeline!");

    IO::ShaderEntry const* shaderData = m_ShaderDB->GetShaderEntry(compName);
    VKW::ShaderModule compModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), shaderData->spirv, shaderData->type, compName, shaderData->entryPoint };

    VKW::Pipeline::Descriptor desc;
    desc.SetPipelineType(VKW::PIPELINE_TYPE_COMPUTE);
    desc.SetComputeShader(compModule);
    desc.SetLayout(layout);

    PipelineEntry::ShaderEntries shaderEntries;
    shaderEntries.EmplaceBack(shaderData);
    CreatePipeline(name, desc, layout, DRE_MOVE(shaderEntries));
    return m_PipelineEntries.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsGizmoPipeline(char const* name, char const* vertName, char const* fragName)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName, fragName, nullptr);

    IO::ShaderEntry const* vertexShaderEntry = m_ShaderDB->GetShaderEntry(vertName);
    IO::ShaderEntry const* fragmentShaderEntry = m_ShaderDB->GetShaderEntry(fragName);

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertexShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_VERTEX, vertName, vertexShaderEntry->entryPoint };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragmentShaderEntry->spirv, VKW::SHADER_MODULE_TYPE_FRAGMENT, fragName, fragmentShaderEntry->entryPoint };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetFragmentShader(fragModule);
    desc.SetLayout(GetLayout(layoutName->GetData()));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    desc.AddColorOutput(g_GraphicsManager->GetMainColorFormat());

    AddDREVertexAttributes(desc);

    PipelineEntry::ShaderEntries shaderEntries;
    shaderEntries.EmplaceBack(vertexShaderEntry);
    shaderEntries.EmplaceBack(fragmentShaderEntry);
    CreatePipeline(name, desc, GetLayout(layoutName->GetData()), DRE_MOVE(shaderEntries));

    return nullptr;
}

void PipelineDB::ReloadShaderFilePipelines(char const* shaderFileName)
{
    IO::ShaderFile const* shaderFile = m_ShaderDB->GetShaderFile(shaderFileName);
    DRE_ASSERT(shaderFile != nullptr, "Attempt to reload pipelines for non-existing shader file");

    DRE::String128 shaderPath = IO::ShaderDB::BuildShaderPath(shaderFile->fileName);
    if (!m_ShaderDB->CompileShaderFile(shaderPath.GetData()))
    {
        std::cout << "Failed to recompile shader file" << shaderPath.GetData() << ". Pipelines were not recreated." << std::endl;
        return;
    }

    DRE::InplaceVector<DRE::String64, 16> pipelinesToReload;

    for (DRE::U32 i = 0, count = shaderFile->shaderEntries.Size(); i < count; i++)
    {
        DRE::String64 const& shaderEntryName = shaderFile->shaderEntries[i];

        DRE::InplaceVector<DRE::String64, 16>* pipelinesList = m_ShaderToPipelines.Find(shaderEntryName).value;
        if (pipelinesList == nullptr)
            continue;

        for (DRE::U32 j = 0, pCount = pipelinesList->Size(); j < pCount; j++)
        {
            pipelinesToReload.EmplaceBackUnique((*pipelinesList)[j]);
        }
    }

    for (DRE::U32 i = 0, count = pipelinesToReload.Size(); i < count; i++)
    {
        RecreatePipeline(pipelinesToReload[i].GetData());
    }
}

void PipelineDB::RecreatePipeline(char const* name)
{
    PipelineEntry* entry = GetEntry(name);
    DRE_ASSERT(entry != nullptr, "Attempt to recreate pipeline which did not exist.");

    VKW::Pipeline* pipeline = entry->GetPipeline();
    DRE::InplaceVector<VKW::ShaderModule, VKW::Pipeline::MAX_SHADER_STAGES> shaderModulesVector; // keep alive until pipeline is created

    for (DRE::U32 i = 0, count = entry->GetShaderEntries().Size(); i < count; i++)
    {
        IO::ShaderEntry const* shaderEntry = entry->GetShaderEntries()[i];
        VKW::ShaderModule& shaderModule = shaderModulesVector.EmplaceBack(
            g_GraphicsManager->GetVulkanTable(),
            g_GraphicsManager->GetMainDevice()->GetLogicalDevice(),
            shaderEntry->spirv, shaderEntry->type,
            shaderEntry->name.GetData(), shaderEntry->entryPoint);

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
    entry->SetPipeline(VKW::Pipeline{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), desc, name });
}

void PipelineDB::ReloadAllPipelines()
{
    m_ShaderDB->CompileSources(false);

    m_PipelineEntries.ForEach([this](auto pair)
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

    DRE::U32 const globalLayoutsCount = VKW::DescriptorManager::GLOBAL_SET_COUNT;

    // next used set after global descriptor sets
    DRE::U32 const startMemberId = shaderInterface.m_Members.FindIf([globalLayoutsCount](auto const& data) { return data.set >= globalLayoutsCount; });

    auto& layouts = m_ShaderLayouts[shaderName];
    if (startMemberId != shaderInterface.m_Members.Size())
    {
        DRE::U8 prevSet = shaderInterface.m_Members[startMemberId - 1].set;
        DRE::U8 currentSet = shaderInterface.m_Members[startMemberId].set;
        DRE_ASSERT(currentSet == prevSet + 1, "Descriptor sets must be continuous");

        VKW::DescriptorSetLayout::Descriptor setLayoutDesc{};

        for (DRE::U32 i = startMemberId, size = shaderInterface.m_Members.Size(); i < size; i++)
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
    for (DRE::U32 i = 0, size = layouts.Size(); i < size; i++)
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

    for (DRE::U32 i = 0, count = VKW::DescriptorManager::GLOBAL_SET_COUNT; i < count; i++)
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

PipelineEntry* PipelineDB::CreatePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor, VKW::PipelineLayout* layout, PipelineEntry::ShaderEntries&& shaderEntries)
{
    for (DRE::U8 i = 0; i < VKW::SHADER_STAGE_SLOT_MAX; i++)
    {
        if (!descriptor.IsShaderStagePresent(VKW::ShaderStageSlot(i)))
            continue;
        m_ShaderToPipelines[descriptor.GetShaderStageName(VKW::ShaderStageSlot(i))].EmplaceBackUnique(DRE::String64{ name });
    }

    return &m_PipelineEntries.Emplace(name,
        VKW::Pipeline{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor, name },
        layout,
        DRE_MOVE(shaderEntries),
        name);
}

VKW::PipelineLayout const* PipelineDB::GetGlobalLayout() const
{
    return m_Device->GetDescriptorManager()->GetGlobalPipelineLayout();
}

VKW::PipelineLayout* PipelineDB::GetLayout(char const* name)
{
    return m_PipelineLayouts.Find(name).value;
}

PipelineEntry* PipelineDB::GetEntry(char const* name)
{
    return m_PipelineEntries.Find(name).value;
}

VKW::DescriptorSetLayout* PipelineDB::GetSetLayout(char const* name)
{
    return &m_SetLayouts[name];
}

}

