#include <gfx\pipeline\PipelineDB.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>

#include <engine\io\IOManager.hpp>

namespace GFX
{

PipelineDB::PipelineDB(VKW::Device* device, IO::IOManager* ioManager)
    : m_Device(device)
    , m_IOManager{ ioManager }
{
    VKW::PipelineLayout::Descriptor globalLayoutDescriptor;
    AddGlobalLayouts(globalLayoutDescriptor);
    m_GlobalLayout = VKW::PipelineLayout{ device->GetFuncTable(), device->GetLogicalDevice(), globalLayoutDescriptor };
}

PipelineDB::~PipelineDB()
{
}

void PipelineDB::CreateDefaultPipelines()
{
    // default plane material shader
    {
        CreateMaterialPipeline("default_lit");
        CreateMaterialPipeline("cook_torrance");
    }
}

DRE::String64 const* PipelineDB::CreateMaterialPipeline(char const* name)
{
    DRE::String64 const* layoutName = CreatePipelineLayoutFromShader(name);

    DRE::String64 vertName{ name }; vertName.Append(".vert");
    DRE::String64 fragName{ name }; fragName.Append(".frag");

    IO::IOManager::ShaderData const* vertData = m_IOManager->GetShaderData(vertName.GetData());
    IO::IOManager::ShaderData const* fragData = m_IOManager->GetShaderData(fragName.GetData());

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData->m_Binary, vertData->m_ModuleType, "main" };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData->m_Binary, fragData->m_ModuleType, "main" };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetFragmentShader(fragModule);
    desc.SetLayout(GetLayout(layoutName->GetData()));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    desc.EnableDepthTest(VKW::FORMAT_D24_UNORM_S8_UINT);
    desc.AddOutputViewport(g_GraphicsManager->GetMainDevice()->GetSwapchain()->GetFormat(), VKW::BLEND_TYPE_NONE);

    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // pos
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // norm
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // tan
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // btan
    desc.AddVertexAttribute(VKW::FORMAT_R32G32_FLOAT);    // uv

    CreatePipeline(name, desc);
    return m_Pipelines.Find(name).key;
}

void PipelineDB::ReloadPipeline(char const* name)
{
    std::cout << "Reloading pipeline " << name << std::endl;

    DRE::String128 layoutName{ name }; layoutName.Append("_layout");
    VKW::PipelineLayout* layout = GetLayout(layoutName.GetData());

    DRE::String64 vertName{ name }; vertName.Append(".vert");
    DRE::String64 fragName{ name }; fragName.Append(".frag");

    IO::IOManager::ShaderData* vertData = m_IOManager->GetShaderData(vertName.GetData());
    IO::IOManager::ShaderData* fragData = m_IOManager->GetShaderData(fragName.GetData());

    DRE::String64 vertPath{ "shaders\\"}; vertPath.Append(vertName.GetData());
    DRE::String64 fragPath{ "shaders\\" }; fragPath.Append(fragName.GetData());

    vertData->m_Binary = m_IOManager->CompileGLSL(vertPath.GetData());
    fragData->m_Binary = m_IOManager->CompileGLSL(fragPath.GetData());

    vertPath.Append(".spv");
    fragPath.Append(".spv");

    IO::IOManager::WriteNewFile(vertPath.GetData(), vertData->m_Binary);
    IO::IOManager::WriteNewFile(fragPath.GetData(), fragData->m_Binary);

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData->m_Binary, vertData->m_ModuleType, "main" };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData->m_Binary, fragData->m_ModuleType, "main" };

    VKW::Pipeline* pipeline = GetPipeline(name);
    DRE_ASSERT(pipeline != nullptr, "Attempt to reload pipeline which did not exist.");

    VKW::Pipeline::Descriptor& desc = pipeline->GetDescriptor();

    desc.SetVertexShader(vertModule);
    desc.SetFragmentShader(fragModule);

    m_Pipelines[name] = VKW::Pipeline{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), desc };
}

DRE::String64 const* PipelineDB::CreatePipelineLayoutFromShader(char const* shaderName)
{
    DRE::String128 vertName{ shaderName }; vertName.Append(".vert");
    DRE::String128 fragName{ shaderName }; fragName.Append(".frag");

    IO::IOManager::ShaderData const* vertexShader = m_IOManager->GetShaderData(vertName.GetData());
    IO::IOManager::ShaderData const* fragmentShader = m_IOManager->GetShaderData(fragName.GetData());


    IO::IOManager::ShaderInterface shaderInterface = vertexShader->m_Interface;
    shaderInterface.Merge(fragmentShader->m_Interface);
    shaderInterface.m_Members.SortBubble([](auto const& lhs, auto const& rhs) { return lhs.set < rhs.set; });

    std::uint32_t const globalLayoutsCount = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();

    // next used set after global descriptor sets
    std::uint32_t const startSetId = shaderInterface.m_Members.FindIf([globalLayoutsCount](auto const& data) { return data.set >= globalLayoutsCount; });

    auto& layouts = m_ShaderLayouts[shaderName];
    if (startSetId != shaderInterface.m_Members.Size())
    {
        std::uint8_t prevSet = shaderInterface.m_Members[startSetId - 1].set;
        std::uint8_t currentSet = shaderInterface.m_Members[startSetId].set;
        DRE_ASSERT(currentSet == prevSet + 1, "Descriptor sets must be continuous");

        VKW::DescriptorSetLayout::Descriptor setLayoutDesc{};

        for (std::uint32_t i = startSetId, size = shaderInterface.m_Members.Size(); i < size; i++)
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
}

VKW::PipelineLayout* PipelineDB::CreatePipelineLayout(char const* name, VKW::PipelineLayout::Descriptor const& descriptor)
{
    DRE_ASSERT(descriptor.GetLayout(0) == &m_Device->GetDescriptorManager()->GetGlobalSetLayout(0), "Invalid layout creation in PipelineDB.");
    DRE_ASSERT(descriptor.GetLayout(1) == &m_Device->GetDescriptorManager()->GetGlobalSetLayout(1), "Invalid layout creation in PipelineDB.");
    DRE_ASSERT(descriptor.GetLayout(2) == &m_Device->GetDescriptorManager()->GetGlobalSetLayout(2), "Invalid layout creation in PipelineDB.");

    return &(m_PipelineLayouts.Emplace(name, m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor));
}

VKW::DescriptorSetLayout* PipelineDB::CreateDescriptorSetLayout(const char* name, VKW::DescriptorSetLayout::Descriptor const& desc)
{
    return &(m_SetLayouts[name] = VKW::DescriptorSetLayout{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), desc });
}

VKW::Pipeline* PipelineDB::CreatePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor)
{
    return &(m_Pipelines.Emplace(name, m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor));
}

VKW::PipelineLayout const* PipelineDB::GetGlobalLayout() const
{
    return &m_GlobalLayout;
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

}

