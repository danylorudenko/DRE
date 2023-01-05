#include <gfx\scheduling\DependencyManager.hpp>

#include <foundation\Common.hpp>

#include <vk_interface\ImportTable.hpp>
#include <vk_interface\Helper.hpp>
#include <vk_interface\pipeline\Dependency.hpp>
#include <vk_interface\Context.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\pass\BasePass.hpp>
#include <gfx\scheduling\GraphResourcesManager.hpp>

namespace GFX
{

DependencyManager::DependencyManager(GraphResourcesManager* graphResources)
{}

DependencyManager::~DependencyManager()
{
}

inline VkImageMemoryBarrier2KHR FillBarrierFlags(VKW::ResourceAccess prevAccess, VKW::Stages prevStage, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t queueFamily)
{
    VkImageMemoryBarrier2KHR b{};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
    b.pNext = nullptr;
    b.srcStageMask  = VKW::StagesToFlags(prevStage);
    b.dstStageMask  = VKW::StagesToFlags(stage);
    b.srcAccessMask = VKW::AccessToFlags(prevAccess);
    b.dstAccessMask = VKW::AccessToFlags(access);
    b.oldLayout     = VKW::AccessToLayout(prevAccess);
    b.newLayout     = VKW::AccessToLayout(access);
    b.srcQueueFamilyIndex = queueFamily;
    b.dstQueueFamilyIndex = queueFamily;

    return b;
}

template<typename T>
bool VectorContains(std::vector<T> const& list, T const& b)
{
    return std::find(list.begin(), list.end(), b) != list.end(); 
}

void DependencyManager::ResourceBarrier(VKW::Context& context, VKW::ImageResource* resource, VKW::ResourceAccess access, VKW::Stages stageFlags)
{
    TextureAccessEntry& entry = m_TextureHistory[resource];

    context.CmdResourceDependency(resource, 
        entry.access,   entry.stage,
        access,         stageFlags);

    entry.access = access;
    entry.stage  = stageFlags;

}

void DependencyManager::ResourceBarrier(VKW::Context& context, VKW::BufferResource* resource, VKW::ResourceAccess access, VKW::Stages stageFlags)
{
    BufferAccessEntry& entry = m_BufferHistory[resource];

    context.CmdResourceDependency(resource,
        entry.access,   entry.stage,
        access,         stageFlags);

    entry.access = access;
    entry.stage  = stageFlags;
}

}

