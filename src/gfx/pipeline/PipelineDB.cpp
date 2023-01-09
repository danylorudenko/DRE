#include <gfx\pipeline\PipelineDB.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

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

    CreateDefaultPipelines();
}

PipelineDB::~PipelineDB()
{
}

void PipelineDB::CreateDefaultPipelines()
{
    // default plane material shader
    {
        IO::IOManager::ShaderData const* shaderData = m_IOManager->GetShaderData("plane.vert");
        //shaderData->m_ReflResources.

        /*
        VKW::ShaderModule vModule = VKW::ShaderModule{ 
            m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), 
            *m_IOManager->GetShaderFileBuffer("plane.vert"), VKW::SHADER_MODULE_TYPE_VERTEX, "main" };

        VKW::ShaderModule fModule = VKW::ShaderModule{
            m_Device->GetFuncTable(), m_Device->GetLogicalDevice(),
            *m_IOManager->GetShaderFileBuffer("plane.frag"), VKW::SHADER_MODULE_TYPE_FRAGMENT, "main" };

        VKW::Pipeline::Descriptor pipeDesc;
        pipeDesc.SetVertexShader(vModule);
        pipeDesc.SetFragmentShader(fModule);
        */
    }
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

    return &(m_PipelineLayouts[DRE::String128(name)] = VKW::PipelineLayout{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor });
}

VKW::Pipeline* PipelineDB::CreatePipeline(char const* name, VKW::Pipeline::Descriptor& descriptor)
{
    return &(m_Pipelines[DRE::String128(name)] = VKW::Pipeline{ m_Device->GetFuncTable(), m_Device->GetLogicalDevice(), descriptor });
}

VKW::PipelineLayout const* PipelineDB::GetGlobalLayout() const
{
    return &m_GlobalLayout;
}

VKW::PipelineLayout* PipelineDB::GetLayout(char const* name)
{
    return m_PipelineLayouts.Find(DRE::String128(name)).value;
}

VKW::Pipeline* PipelineDB::GetPipeline(char const* name)
{
    return m_Pipelines.Find(DRE::String128(name)).value;
}

}

