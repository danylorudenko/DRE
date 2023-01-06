#include <vk_wrapper\descriptor\DescriptorManager.hpp>

#include <foundation\Common.hpp>
#include <foundation\Container\InplaceVector.hpp>


#include <vk_wrapper\resources\Resource.hpp>

namespace VKW
{

DescriptorManager::DescriptorManager(ImportTable* table, LogicalDevice* device)
    : table_{ table }
    , device_{ device }
    , standalonePool_{ VK_NULL_HANDLE }
    , globalSetPool_{ VK_NULL_HANDLE }
    , globalTexturesPool_{ VK_NULL_HANDLE }
{
    std::uint32_t constexpr STANDALONE_DESCRIPTOR_COUNT = 20;
    std::uint32_t constexpr MAX_STANDALONE_SETS         = 20;
    std::uint32_t constexpr PERSISTENT_DESCRIPTOR_COUNT = 20;

    VkDescriptorPoolSize sizes[4];
    sizes[0].type               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sizes[0].descriptorCount    = STANDALONE_DESCRIPTOR_COUNT;

    sizes[1].type               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    sizes[1].descriptorCount    = STANDALONE_DESCRIPTOR_COUNT;

    sizes[2].type               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sizes[2].descriptorCount    = STANDALONE_DESCRIPTOR_COUNT;

    sizes[3].type               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[3].descriptorCount    = STANDALONE_DESCRIPTOR_COUNT;

    VkDescriptorPoolCreateInfo standalonePoolInfo;
    standalonePoolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    standalonePoolInfo.pNext          = nullptr;
    standalonePoolInfo.flags          = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    standalonePoolInfo.maxSets        = MAX_STANDALONE_SETS;
    standalonePoolInfo.poolSizeCount  = 4;
    standalonePoolInfo.pPoolSizes     = sizes;

    VK_ASSERT(table_->vkCreateDescriptorPool(device_->Handle(), &standalonePoolInfo, nullptr, &standalonePool_));



    sizes[0].type               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[0].descriptorCount    = 2;

    sizes[1].type               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sizes[1].descriptorCount    = 1;

    sizes[2].type               = VK_DESCRIPTOR_TYPE_SAMPLER;
    sizes[2].descriptorCount    = std::uint32_t(SAMPLER_TYPE_MAX);

    VkDescriptorPoolCreateInfo globalSetPoolInfo;
    globalSetPoolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    globalSetPoolInfo.pNext          = nullptr;
    globalSetPoolInfo.flags          = VK_FLAGS_NONE;
    globalSetPoolInfo.maxSets        = 3; // sampler/storage + 2 uniform buffers
    globalSetPoolInfo.poolSizeCount  = 3;
    globalSetPoolInfo.pPoolSizes     = sizes;

    VK_ASSERT(table_->vkCreateDescriptorPool(device_->Handle(), &globalSetPoolInfo, nullptr, &globalSetPool_));


    sizes[0].type               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sizes[0].descriptorCount    = VKW::CONSTANTS::TEXTURE_DESCRIPTOR_HEAP_SIZE;

    VkDescriptorPoolCreateInfo globalTexturesPoolInfo;
    globalTexturesPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    globalTexturesPoolInfo.pNext = nullptr;
    globalTexturesPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    globalTexturesPoolInfo.maxSets = 1;
    globalTexturesPoolInfo.poolSizeCount = 1;
    globalTexturesPoolInfo.pPoolSizes = sizes;

    VK_ASSERT(table_->vkCreateDescriptorPool(device_->Handle(), &globalTexturesPoolInfo, nullptr, &globalTexturesPool_));

    CreateGlobalDescriptorLayouts();
}

DescriptorManager::DescriptorManager(DescriptorManager&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
{
    operator=(std::move(rhs));
}

DescriptorManager& DescriptorManager::operator=(DescriptorManager&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);

    DRE_SWAP_MEMBER(defaultSamplers_);

    DRE_SWAP_MEMBER(globalSetLayouts_);
    DRE_SWAP_MEMBER(globalSetPool_);

    DRE_SWAP_MEMBER(globalSampler_StorageSet_);
    DRE_SWAP_MEMBER(globalUniformSets_);

    DRE_SWAP_MEMBER(globalTexturesPool_);
    DRE_SWAP_MEMBER(globalTexturesSet_);

    DRE_SWAP_MEMBER(standalonePool_);

    return *this;
}

DescriptorManager::~DescriptorManager()
{
    VkDevice device = device_->Handle();

    table_->vkDestroyDescriptorPool(device, standalonePool_, nullptr);
    table_->vkDestroyDescriptorPool(device, globalTexturesPool_, nullptr);
    table_->vkDestroyDescriptorPool(device, globalSetPool_, nullptr);

    for (std::uint16_t i = 0; i < SAMPLER_TYPE_MAX; i++)
    {
        table_->vkDestroySampler(device, defaultSamplers_[i], nullptr);
    }
}

void DescriptorManager::CreateGlobalDescriptorLayouts()
{
    DescriptorSetLayout::Descriptor globalSampler_StorageSetLayoutDesc{ DESCRIPTOR_STAGE_ALL };
    globalSampler_StorageSetLayoutDesc.Add(DESCRIPTOR_TYPE_SAMPLER, 0, std::uint32_t(SAMPLER_TYPE_MAX));
    globalSampler_StorageSetLayoutDesc.Add(DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);
    globalSetLayouts_[0] = DescriptorSetLayout{ table_, device_, globalSampler_StorageSetLayoutDesc };

    DescriptorSetLayout::Descriptor globalTexturesLayoutDesc{ DESCRIPTOR_STAGE_ALL };
    globalTexturesLayoutDesc.AddVariableCount(DESCRIPTOR_TYPE_TEXTURE, 0, CONSTANTS::TEXTURE_DESCRIPTOR_HEAP_SIZE);
    globalSetLayouts_[1] = DescriptorSetLayout{ table_, device_, globalTexturesLayoutDesc };

    DescriptorSetLayout::Descriptor globalUniformLayoutDesc{ DESCRIPTOR_STAGE_ALL };
    globalUniformLayoutDesc.Add(DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0);
    globalSetLayouts_[2] = DescriptorSetLayout{ table_, device_, globalUniformLayoutDesc };

    VKW::PipelineLayout::Descriptor layoutDesc;
    layoutDesc.Add(globalSetLayouts_ + 0);
    layoutDesc.Add(globalSetLayouts_ + 1);
    layoutDesc.Add(globalSetLayouts_ + 2);

    globalPipelineLayout_ = VKW::PipelineLayout{ table_, device_, layoutDesc };
}

void DescriptorManager::AllocateDefaultDescriptors(std::uint8_t globalBuffersCount, BufferResource** globalUniformBuffers, BufferResource* persistentStorageBuffer)
{
    VkDescriptorSetLayout layouts[2];
    VkDescriptorSetAllocateInfo allocateInfo;


    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.descriptorPool = globalSetPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = layouts;
    layouts[0] = globalSetLayouts_[0].GetHandle();
    VK_ASSERT(table_->vkAllocateDescriptorSets(device_->Handle(), &allocateInfo, &globalSampler_StorageSet_));


    std::uint32_t countsData = VKW::CONSTANTS::TEXTURE_DESCRIPTOR_HEAP_SIZE;
    VkDescriptorSetVariableDescriptorCountAllocateInfo counts;
    counts.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    counts.pNext = nullptr;
    counts.descriptorSetCount = 1;
    counts.pDescriptorCounts = &countsData;

    allocateInfo.pNext = &counts;
    allocateInfo.descriptorPool = globalTexturesPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = layouts;
    layouts[0] = globalSetLayouts_[1].GetHandle();
    VK_ASSERT(table_->vkAllocateDescriptorSets(device_->Handle(), &allocateInfo, &globalTexturesSet_));

    allocateInfo.pNext = nullptr;
    allocateInfo.descriptorPool = globalSetPool_;
    allocateInfo.descriptorSetCount = 2;
    allocateInfo.pSetLayouts = layouts;
    layouts[0] = layouts[1] = globalSetLayouts_[2].GetHandle();
    VK_ASSERT(table_->vkAllocateDescriptorSets(device_->Handle(), &allocateInfo, globalUniformSets_));

    //////////////////////////
    VkSamplerCreateInfo descriptorInfo;
    descriptorInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    descriptorInfo.pNext = nullptr;

    //////////////////////////
    descriptorInfo.flags = VK_FLAGS_NONE;
    descriptorInfo.magFilter = VK_FILTER_NEAREST;
    descriptorInfo.minFilter = VK_FILTER_NEAREST;
    descriptorInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    descriptorInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    descriptorInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    descriptorInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    descriptorInfo.mipLodBias = 0.0f;
    descriptorInfo.anisotropyEnable = VK_FALSE;
    descriptorInfo.maxAnisotropy = 0.0f;
    descriptorInfo.compareEnable = VK_FALSE;
    descriptorInfo.compareOp = VK_COMPARE_OP_LESS;
    descriptorInfo.minLod = VK_LOD_CLAMP_NONE;
    descriptorInfo.maxLod = VK_LOD_CLAMP_NONE;
    descriptorInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    descriptorInfo.unnormalizedCoordinates = VK_FALSE;
    VK_ASSERT(table_->vkCreateSampler(device_->Handle(), &descriptorInfo, nullptr, defaultSamplers_ + (int)SAMPLER_TYPE_NEAREST_REPEAT));

    //////////////////////////
    descriptorInfo.flags = VK_FLAGS_NONE;
    descriptorInfo.magFilter = VK_FILTER_LINEAR;
    descriptorInfo.minFilter = VK_FILTER_LINEAR;
    descriptorInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    descriptorInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    descriptorInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    descriptorInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    descriptorInfo.mipLodBias = 0.0f;
    descriptorInfo.anisotropyEnable = VK_FALSE;
    descriptorInfo.maxAnisotropy = 0.0f;
    descriptorInfo.compareEnable = VK_FALSE;
    descriptorInfo.compareOp = VK_COMPARE_OP_LESS;
    descriptorInfo.minLod = VK_LOD_CLAMP_NONE;
    descriptorInfo.maxLod = VK_LOD_CLAMP_NONE;
    descriptorInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    descriptorInfo.unnormalizedCoordinates = VK_FALSE;
    VK_ASSERT(table_->vkCreateSampler(device_->Handle(), &descriptorInfo, nullptr, defaultSamplers_ + (int)SAMPLER_TYPE_LINEAR_REPEAT));

    //////////////////////////
    descriptorInfo.flags = VK_FLAGS_NONE;
    descriptorInfo.magFilter = VK_FILTER_LINEAR;
    descriptorInfo.minFilter = VK_FILTER_LINEAR;
    descriptorInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    descriptorInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    descriptorInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    descriptorInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    descriptorInfo.mipLodBias = 0.0f;
    descriptorInfo.anisotropyEnable = VK_FALSE;
    descriptorInfo.maxAnisotropy = 0.0f;
    descriptorInfo.compareEnable = VK_FALSE;
    descriptorInfo.compareOp = VK_COMPARE_OP_LESS;
    descriptorInfo.minLod = VK_LOD_CLAMP_NONE;
    descriptorInfo.maxLod = VK_LOD_CLAMP_NONE;
    descriptorInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    descriptorInfo.unnormalizedCoordinates = VK_FALSE;
    VK_ASSERT(table_->vkCreateSampler(device_->Handle(), &descriptorInfo, nullptr, defaultSamplers_ + (int)SAMPLER_TYPE_LINEAR_CLAMP));

    //////////////////////////
    descriptorInfo.flags = VK_FLAGS_NONE;
    descriptorInfo.magFilter = VK_FILTER_LINEAR;
    descriptorInfo.minFilter = VK_FILTER_LINEAR;
    descriptorInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    descriptorInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    descriptorInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    descriptorInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    descriptorInfo.mipLodBias = 0.0f;
    descriptorInfo.anisotropyEnable = VK_TRUE;
    descriptorInfo.maxAnisotropy = 8.0f;
    descriptorInfo.compareEnable = VK_FALSE;
    descriptorInfo.compareOp = VK_COMPARE_OP_LESS;
    descriptorInfo.minLod = VK_LOD_CLAMP_NONE;
    descriptorInfo.maxLod = VK_LOD_CLAMP_NONE;
    descriptorInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    descriptorInfo.unnormalizedCoordinates = VK_FALSE;
    VK_ASSERT(table_->vkCreateSampler(device_->Handle(), &descriptorInfo, nullptr, defaultSamplers_ + (int)SAMPLER_TYPE_ANISOTROPIC));


    DRE::InplaceVector<VkDescriptorBufferInfo, VKW::CONSTANTS::FRAMES_BUFFERING> uniformBuffersinfo;
    VkDescriptorBufferInfo persistentStorageInfo;
    VkDescriptorImageInfo samplerInfo[(int)SAMPLER_TYPE_MAX];
    

    for (std::uint8_t i = 0; i < globalBuffersCount; i++)
    {
        VkDescriptorBufferInfo& info = uniformBuffersinfo.EmplaceBack();
        info.buffer = globalUniformBuffers[i]->handle_;
        info.offset = 0;
        info.range = globalUniformBuffers[i]->size_;
    }

    for (std::uint8_t i = 0; i < (int)SAMPLER_TYPE_MAX; i++)
    {
        samplerInfo[i].sampler = defaultSamplers_[i];
    }

    persistentStorageInfo.buffer = persistentStorageBuffer->handle_;
    persistentStorageInfo.offset = 0;
    persistentStorageInfo.range = persistentStorageBuffer->size_;


    std::uint16_t constexpr writeCount = VKW::CONSTANTS::FRAMES_BUFFERING * 2 + 1;
    DRE::InplaceVector<VkWriteDescriptorSet, writeCount> writeInfos;

    for (std::uint16_t i = 0; i < globalBuffersCount; i++)
    {
        VkWriteDescriptorSet& uniformWrite = writeInfos.EmplaceBack();
        uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformWrite.pNext = nullptr;
        uniformWrite.dstSet = globalUniformSets_[i];
        uniformWrite.dstBinding = 0;
        uniformWrite.dstArrayElement = 0;
        uniformWrite.descriptorCount = 1;
        uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformWrite.pImageInfo = nullptr;
        uniformWrite.pBufferInfo = uniformBuffersinfo.Data() + i;
        uniformWrite.pTexelBufferView = nullptr;
    }

    VkWriteDescriptorSet& samplerWrite = writeInfos.EmplaceBack();
    samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    samplerWrite.pNext = nullptr;
    samplerWrite.dstSet = globalSampler_StorageSet_;
    samplerWrite.dstBinding = 0;
    samplerWrite.dstArrayElement = 0;
    samplerWrite.descriptorCount = (int)SAMPLER_TYPE_MAX;
    samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerWrite.pImageInfo = samplerInfo;
    samplerWrite.pBufferInfo = nullptr;
    samplerWrite.pTexelBufferView = nullptr;

    VkWriteDescriptorSet& storageWrite = writeInfos.EmplaceBack();
    storageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    storageWrite.pNext = nullptr;
    storageWrite.dstSet = globalSampler_StorageSet_;
    storageWrite.dstBinding = 1;
    storageWrite.dstArrayElement = 0;
    storageWrite.descriptorCount = 1;
    storageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageWrite.pImageInfo = nullptr;
    storageWrite.pBufferInfo = &persistentStorageInfo;
    storageWrite.pTexelBufferView = nullptr;

    table_->vkUpdateDescriptorSets(device_->Handle(), writeInfos.Size(), writeInfos.Data(), 0, nullptr);
}

VkSampler DescriptorManager::GetDefaultSampler(SamplerType type) const
{
    return defaultSamplers_[(int)type];
}

GlobalDescriptorHandle DescriptorManager::AllocateTextureDescriptor(ImageResourceView const* view)
{
    std::uint16_t id = dynamicTextureHeap_.Allocate(1);

    if (view != nullptr)
    {
        WriteTextureDescriptor(globalTexturesSet_, id, view);
    }

    return GlobalDescriptorHandle{ id };
}

void DescriptorManager::WriteTextureDescriptor(VkDescriptorSet set, std::uint16_t descriptorID, ImageResourceView const* view)
{
    VkDescriptorImageInfo imageInfo;
    imageInfo.sampler = VK_NULL_HANDLE;
    imageInfo.imageView = view->handle_;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writeInfo;
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.pNext = nullptr;
    writeInfo.dstSet = set;
    writeInfo.dstBinding = 0;
    writeInfo.dstArrayElement = descriptorID;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writeInfo.pImageInfo = &imageInfo;
    writeInfo.pBufferInfo = nullptr;
    writeInfo.pTexelBufferView = nullptr;

    table_->vkUpdateDescriptorSets(device_->Handle(), 1, &writeInfo, 0, nullptr);
}

void DescriptorManager::FreeTextureDescriptor(GlobalDescriptorHandle& handle)
{
    dynamicTextureHeap_.Free(handle.id_);

    handle.id_ = 0;
    handle.count_ = 0;
}

VkDescriptorSet DescriptorManager::AllocateStandaloneSet(DescriptorSetLayout const& layout)
{
    VkDescriptorSetLayout vkLayout = layout.GetHandle();

    VkDescriptorSetAllocateInfo info;
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.descriptorPool = standalonePool_;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &vkLayout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    VK_ASSERT(table_->vkAllocateDescriptorSets(device_->Handle(), &info, &set));

    return set;
}

void DescriptorManager::FreeStandaloneSet(VkDescriptorSet set)
{
    VK_ASSERT(table_->vkFreeDescriptorSets(device_->Handle(), standalonePool_, 1, &set));
}

}

