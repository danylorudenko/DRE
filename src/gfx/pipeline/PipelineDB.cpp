#include <gfx\pipeline\PipelineDB.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\descriptor\DescriptorManager.hpp>

namespace GFX
{

PipelineDB::PipelineDB(VKW::Device* device)
    : m_Device(device)
{
    VKW::PipelineLayout::Descriptor globalLayoutDescriptor;
    AddGlobalLayouts(globalLayoutDescriptor);
    m_GlobalLayout = VKW::PipelineLayout{ device->GetFuncTable(), device->GetLogicalDevice(), globalLayoutDescriptor };
}

PipelineDB::~PipelineDB()
{
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

