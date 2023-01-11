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
    }
}

DRE::String128 const* PipelineDB::CreateMaterialPipeline(char const* name)
{
    DRE::String128 const* layoutName = CreatePipelineLayoutFromShader(name);

    DRE::String128 vertName{ name }; vertName.Append(".vert");
    DRE::String128 fragName{ name }; fragName.Append(".frag");

    IO::IOManager::ShaderData const* vertData = m_IOManager->GetShaderData(vertName);
    IO::IOManager::ShaderData const* fragData = m_IOManager->GetShaderData(fragName);

    VKW::ShaderModule vertModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), vertData->m_Binary, vertData->m_ModuleType, "main" };
    VKW::ShaderModule fragModule{ g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), fragData->m_Binary, fragData->m_ModuleType, "main" };

    VKW::Pipeline::Descriptor desc;

    desc.SetPipelineType(VKW::PIPELINE_TYPE_GRAPHIC);
    desc.SetVertexShader(vertModule);
    desc.SetFragmentShader(fragModule);
    desc.SetLayout(GetLayout(*layoutName));
    desc.SetCullMode(VK_CULL_MODE_BACK_BIT);
    desc.EnableDepthStencilTest(VKW::FORMAT_D16_UNORM);
    desc.AddOutputViewport(g_GraphicsManager->GetMainDevice()->GetSwapchain()->GetFormat(), VKW::BLEND_TYPE_NONE);

    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // pos
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // norm
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // tan
    desc.AddVertexAttribute(VKW::FORMAT_R32G32B32_FLOAT); // btan
    desc.AddVertexAttribute(VKW::FORMAT_R32G32_FLOAT);    // uv

    CreatePipeline(name, desc); validation errors here
    return m_Pipelines.Find(name).key;
}

DRE::String128 const* PipelineDB::CreatePipelineLayoutFromShader(char const* shaderName)
{
    DRE::String128 vertName{ shaderName }; vertName.Append(".vert");
    DRE::String128 fragName{ shaderName }; fragName.Append(".frag");

    IO::IOManager::ShaderData const* vertexShader = m_IOManager->GetShaderData(vertName);
    IO::IOManager::ShaderData const* fragmentShader = m_IOManager->GetShaderData(fragName);


    IO::IOManager::ShaderInterface shaderInterface = vertexShader->m_Interface;
    shaderInterface.Merge(fragmentShader->m_Interface);
    shaderInterface.m_Members.SortBubble([](auto const& lhs, auto const& rhs) { return lhs.set < rhs.set; });

    std::uint32_t const globalLayoutsCount = g_GraphicsManager->GetMainDevice()->GetDescriptorAllocator()->GetGlobalSetLayoutsCount();

    // next used set after global descriptor sets
    std::uint32_t const startSetId = shaderInterface.m_Members.FindIf([globalLayoutsCount](auto const& data) { return data.set >= globalLayoutsCount; });

    DRE::InplaceVector<VKW::DescriptorSetLayout, VKW::CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS> layouts;
    if (startSetId != shaderInterface.m_Members.Size())
    {
        std::uint8_t currentSet = shaderInterface.m_Members[startSetId].set;
        DRE_ASSERT(currentSet == globalLayoutsCount, "Descriptor sets must be continuous");

        VKW::DescriptorSetLayout::Descriptor setLayoutDesc{};

        for (std::uint32_t i = startSetId, size = shaderInterface.m_Members.Size(); i < size; i++)
        {
            auto const& m = shaderInterface.m_Members[i];
            if (m.set != currentSet)
            {
                layouts.EmplaceBack(g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), setLayoutDesc);
                setLayoutDesc = VKW::DescriptorSetLayout::Descriptor{};
                currentSet = m.set;
                DRE_ASSERT(currentSet == (globalLayoutsCount + layouts.Size()), "Descriptor sets must be continuous");
            }

            setLayoutDesc.Add(m.type, m.binding, m.stage, m.arraySize);
        }
        layouts.EmplaceBack(g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice(), setLayoutDesc);

    }

    VKW::PipelineLayout::Descriptor layoutDesc;
    AddGlobalLayouts(layoutDesc);
    for (std::uint32_t i = 0, size = layouts.Size(); i < size; i++)
    {
        layoutDesc.Add(&layouts[i]);
    }

    DRE::String128 layoutName{ shaderName }; layoutName.Append("_layout");
    CreatePipelineLayout(layoutName, layoutDesc);

    return m_PipelineLayouts.Find(layoutName).key;
}

void PipelineDB::AddGlobalLayouts(VKW::PipelineLayout::Descriptor& descriptor)
{
    DRE_ASSERT(descriptor.GetSetCount() == 0, "Pipeline layout descriptor must be empty before filling default layouts.");

    VKW::DescriptorManager* allocator = m_Device->GetDescriptorAllocator();

    for (std::uint32_t i = 0, count = allocator->GetGlobalSetLayoutsCount(); i < count; i++)
    {
        descriptor.Add(&allocator->GetGlobalSetLayout(i));
    }
}

VKW::PipelineLayout* PipelineDB::CreatePipelineLayout(char const* name, VKW::PipelineLayout::Descriptor const& descriptor)
{
    DRE_ASSERT(descriptor.GetLayout(0) == &m_Device->GetDescriptorAllocator()->GetGlobalSetLayout(0), "Invalid layout creation in PipelineDB.");
    DRE_ASSERT(descriptor.GetLayout(1) == &m_Device->GetDescriptorAllocator()->GetGlobalSetLayout(1), "Invalid layout creation in PipelineDB.");
    DRE_ASSERT(descriptor.GetLayout(2) == &m_Device->GetDescriptorAllocator()->GetGlobalSetLayout(2), "Invalid layout creation in PipelineDB.");

    return &(m_PipelineLayouts[name] = VKW::PipelineLayout{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor });
}

VKW::Pipeline* PipelineDB::CreatePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor)
{
    return &(m_Pipelines[name] = VKW::Pipeline{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor });
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

}

