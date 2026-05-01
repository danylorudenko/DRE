#include <gfx\pipeline\PipelineDB.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>

#include <engine\io\ShaderDB.hpp>

#include <common\forward.h>


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
        CreateGraphicsForwardPipeline("forward_pbr");

        CreateGraphicsGBufferPipeline("gbuffer_pbr");

        CreateGraphicsGizmoPipeline("gizmo_3D");

        CreateComputePipeline("lighting_deferred");
        CreateComputePipeline("color_encode");
        CreateComputePipeline("ambient_occlusion");
        CreateComputePipeline("temporal_AA");
        CreateComputePipeline("gen_butterfly");
        CreateComputePipeline("gen_h0");
        CreateComputePipeline("gen_hxt");
        CreateComputePipeline("fft_iter");
        CreateComputePipeline("fft_inv_perm");
        CreateComputePipeline("debug_view_texture");
        CreateComputePipeline("debug_view_ddgi_probes");

        // DDGI
        CreateComputePipeline("ddgi_probe_border_blend");
        CreateComputePipeline("ddgi_probe_lighting");
        CreateComputePipeline("ddgi_probe_scatter");
        CreateComputePipeline("ddgi_probe_trace");

        VKW::Pipeline::Descriptor waterCausticDesc;
        waterCausticDesc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
        //waterCausticDesc.EnableDepthTest(g_GraphicsManager->GetMainDepthFormat(), false);
        AddDREVertexAttributes(waterCausticDesc);
        waterCausticDesc.AddColorOutput(VKW::FORMAT_R8_UNORM);
        CreateCustomGraphicsPipeline("water_caustics", waterCausticDesc);

        VKW::Pipeline::Descriptor shadowDesc;
        shadowDesc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
        shadowDesc.EnableDepthTest(VKW::FORMAT_D16_UNORM);
        shadowDesc.AddColorOutput(VKW::FORMAT_R16G16B16A16_FLOAT);
        AddDREVertexAttributes(shadowDesc);
        CreateCustomGraphicsPipeline("forward_shadow", shadowDesc);
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

DRE::String64 const* PipelineDB::CreateCustomGraphicsPipeline(char const* name, VKW::Pipeline::Descriptor& descriptor)
{
    DRE::String64 vertName{ name }; vertName.Append(".vert");
    DRE::String64 fragName{ name }; fragName.Append(".frag");

    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName.GetData(), fragName.GetData(), nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName.GetData())->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName.GetData())->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, "main" };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, "main" };

    descriptor.SetVertexShader(vertModule);
    descriptor.SetFragmentShader(fragModule);
    descriptor.SetLayout(GetLayout(layoutName->GetData()));
    descriptor.SetCullMode(VK_CULL_MODE_BACK_BIT);

    CreatePipeline(name, descriptor);
    return m_Pipelines.Find(name).key;

}

DRE::String64 const* PipelineDB::CreateGraphicsForwardPipeline(char const* name)
{
    DRE::String64 vertName{ name }; vertName.Append(".vert");
    DRE::String64 fragName{ name }; fragName.Append(".frag");

    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName.GetData(), fragName.GetData(), nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName.GetData())->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName.GetData())->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, "main" };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, "main" };

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

DRE::String64 const* PipelineDB::CreateGraphicsGBufferPipeline(char const* name)
{
    DRE::String64 vertName{ name }; vertName.Append(".vert");
    DRE::String64 fragName{ name }; fragName.Append(".frag");

    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName.GetData(), fragName.GetData(), nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName.GetData())->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName.GetData())->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, "main" };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, "main" };

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

DRE::String64 const* PipelineDB::CreateGraphicsForwardWaterPipeline(char const* name)
{
    DRE::String64 vertName{ name }; vertName.Append(".vert");
    DRE::String64 fragName{ name }; fragName.Append(".frag");

    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName.GetData(), fragName.GetData(), nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName.GetData())->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName.GetData())->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, "main" };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, "main" };

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

DRE::String64 const* PipelineDB::CreateGraphicsForwardShadowPipeline(char const* name)
{
    DRE::String64 vertName{ name }; vertName.Append(".vert");

    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName.GetData(), nullptr, nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName.GetData())->spirv;
    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, "main" };

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

DRE::String64 const* PipelineDB::CreateComputePipeline(char const* name)
{
    DRE::String64 compName{ name }; compName.Append(".comp");

    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, nullptr, nullptr, compName.GetData());
    VKW::PipelineLayout* layout = GetLayout(layoutName->GetData());
    DRE_ASSERT(layout != nullptr, "Can't find pipeline!");

    IO::ShaderEntry const* shaderData = m_ShaderDB->GetShaderEntry(compName.GetData());
    VKW::ShaderModule compModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), shaderData->spirv, shaderData->type, "main" };

    VKW::Pipeline::Descriptor desc;
    desc.SetPipelineType(VKW::PIPELINE_TYPE_COMPUTE);
    desc.SetComputeShader(compModule);
    desc.SetLayout(layout);

    CreatePipeline(name, desc);
    return m_Pipelines.Find(name).key;
}

DRE::String64 const* PipelineDB::CreateGraphicsGizmoPipeline(char const* name)
{
    VKW::Pipeline::Descriptor pipeDesc;

    DRE::String64 vertName{ name }; vertName.Append(".vert");
    DRE::String64 fragName{ name }; fragName.Append(".frag");

    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name, vertName.GetData(), fragName.GetData(), nullptr);

    DRE::ByteBuffer const& vertData = m_ShaderDB->GetShaderEntry(vertName.GetData())->spirv;
    DRE::ByteBuffer const& fragData = m_ShaderDB->GetShaderEntry(fragName.GetData())->spirv;

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData, VKW::SHADER_MODULE_TYPE_VERTEX, "main" };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData, VKW::SHADER_MODULE_TYPE_FRAGMENT, "main" };

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

void PipelineDB::ReloadPipeline(char const* name)
{
    DRE::String128 layoutName{ name }; layoutName.Append("_layout");
    VKW::PipelineLayout* layout = GetLayout(layoutName.GetData());

    DRE::String64 vertName{ name }; vertName.Append(".vert");
    DRE::String64 fragName{ name }; fragName.Append(".frag");
    DRE::String64 compName{ name }; compName.Append(".comp");

    IO::ShaderEntry const* vertData = m_ShaderDB->GetShaderEntry(vertName.GetData());
    IO::ShaderEntry const* fragData = m_ShaderDB->GetShaderEntry(fragName.GetData());
    IO::ShaderEntry const* compData = m_ShaderDB->GetShaderEntry(compName.GetData());

    VKW::ShaderModule vertModule;
    VKW::ShaderModule fragModule;
    VKW::ShaderModule compModule;

    VKW::Pipeline* pipeline = GetPipeline(name);
    DRE_ASSERT(pipeline != nullptr, "Attempt to reload pipeline which did not exist.");
    VKW::Pipeline::Descriptor& desc = pipeline->GetDescriptor();

    if (vertData != nullptr)
    {
        DRE::String64 vertPath{ "shaders\\" }; vertPath.Append(vertName.GetData());
        if (!m_ShaderDB->CompileShader(vertPath.GetData(), VKW::SHADER_MODULE_TYPE_VERTEX))
        {
            std::cout << "Failed to recompile shader " << vertPath.GetData() << ". Pipeline was not cecreated." << std::endl;
            return;
        }

        vertModule = VKW::ShaderModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData->spirv, vertData->type, "main" };
        desc.SetVertexShader(vertModule);
    }
    
    if (fragData != nullptr)
    {
        DRE::String64 fragPath{ "shaders\\" }; fragPath.Append(fragName.GetData());
        if (!m_ShaderDB->CompileShader(fragPath.GetData(), VKW::SHADER_MODULE_TYPE_FRAGMENT))
        {
            std::cout << "Failed to recompile shader " << fragPath.GetData() << ". Pipeline was not cecreated." << std::endl;
            return;
        }

        fragModule = VKW::ShaderModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData->spirv, fragData->type, "main" };
        desc.SetFragmentShader(fragModule);
    }

    if (compData != nullptr)
    {
        DRE::String64 compPath{ "shaders\\" }; compPath.Append(compName.GetData());
        if (!m_ShaderDB->CompileShader(compPath.GetData(), VKW::SHADER_MODULE_TYPE_COMPUTE))
        {
            std::cout << "Failed to recompile shader " << compPath.GetData() << ". Pipeline was not cecreated." << std::endl;
            return;
        }

        compModule = VKW::ShaderModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), compData->spirv, compData->type, "main" };
        desc.SetComputeShader(compModule);
    }

    m_Pipelines[name] = VKW::Pipeline{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), desc, name };
}

void PipelineDB::ReloadAllPipelines()
{
    m_Pipelines.ForEach([this](auto pair)
    {
        ReloadPipeline(pair.key->GetData());
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

