#include <vk_wrapper\pipeline\Pipeline.hpp>

#include <foundation\Common.hpp>

#include <vk_wrapper\Tools.hpp>

#include <vk_wrapper\ImportTable.hpp>
#include <vk_wrapper\LogicalDevice.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>
#include <vk_wrapper\descriptor\DescriptorLayout.hpp>

namespace VKW
{

Pipeline::Descriptor::Descriptor()
    : type_{ PIEPLINE_TYPE_INVALID }
    , depthTestEnabled_{ false }
    , stencilTestEnabled_{ false }
    , shaderStagesCount_{ 0 }
    , vertexAttributeCount_{ 0 }
    , colorOutputCount_{ 0 }
    , depthAttachmentFormat_{ VK_FORMAT_UNDEFINED }
    , stencilAttachmentFormat_{ VK_FORMAT_UNDEFINED }
    , pipelineLayout_{ nullptr }
{
    renderingCreateInfo_.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingCreateInfo_.pNext = nullptr;
    renderingCreateInfo_.viewMask = 0;
    renderingCreateInfo_.colorAttachmentCount = 0;
    renderingCreateInfo_.pColorAttachmentFormats = nullptr;
    renderingCreateInfo_.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    renderingCreateInfo_.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    vertexInputState_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState_.pNext = nullptr;
    vertexInputState_.flags = VK_FLAGS_NONE;
    vertexInputState_.vertexBindingDescriptionCount = 0;
    vertexInputState_.pVertexBindingDescriptions = &vertexBindingDescription_;
    vertexInputState_.vertexAttributeDescriptionCount = 0;
    vertexInputState_.pVertexAttributeDescriptions = vertexAttributeDescriptions_;

    vertexBindingDescription_.binding = 0;
    vertexBindingDescription_.stride = 0;
    vertexBindingDescription_.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    //////////////////////////////////

    inputAssemblyState_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState_.pNext = nullptr;
    inputAssemblyState_.flags = VK_FLAGS_NONE;
    inputAssemblyState_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState_.primitiveRestartEnable = false;

    //////////////////////////////////

    viewportState_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState_.pNext = nullptr;
    viewportState_.flags = VK_FLAGS_NONE;
    viewportState_.viewportCount = 0;
    viewportState_.pViewports = nullptr;
    viewportState_.scissorCount = 0;
    viewportState_.pScissors = nullptr;

    //////////////////////////////////

    rasterizationState_.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState_.pNext = nullptr;
    rasterizationState_.flags = VK_FLAGS_NONE;
    rasterizationState_.depthClampEnable = VK_FALSE;
    rasterizationState_.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState_.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState_.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState_.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState_.depthBiasEnable = VK_FALSE;
    rasterizationState_.depthBiasConstantFactor = 0.0f;
    rasterizationState_.depthBiasClamp = 0.0f;
    rasterizationState_.depthBiasSlopeFactor = 0.0f;
    rasterizationState_.lineWidth = 1.0f;

    //////////////////////////////////

    multisampleState_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState_.pNext = nullptr;
    multisampleState_.flags = VK_FLAGS_NONE;
    multisampleState_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState_.sampleShadingEnable = false;
    multisampleState_.minSampleShading = 0.0f;
    multisampleState_.pSampleMask = nullptr;
    multisampleState_.alphaToCoverageEnable = false;
    multisampleState_.alphaToOneEnable = false;

    //////////////////////////////////

    depthStencilState_.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState_.pNext = nullptr;
    depthStencilState_.flags = VK_FLAGS_NONE;
    depthStencilState_.depthTestEnable = VK_FALSE;
    depthStencilState_.depthWriteEnable = VK_FALSE;
    depthStencilState_.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState_.depthBoundsTestEnable = VK_FALSE;
    depthStencilState_.stencilTestEnable = VK_FALSE;
    depthStencilState_.front = {};
    depthStencilState_.back = {};
    depthStencilState_.minDepthBounds = 0.0f;
    depthStencilState_.maxDepthBounds = 1.0f;

    //////////////////////////////////

    blendState_.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState_.pNext = nullptr;
    blendState_.flags = VK_FLAGS_NONE;
    blendState_.logicOpEnable = VK_FALSE;
    blendState_.logicOp = VK_LOGIC_OP_CLEAR;
    blendState_.attachmentCount = 0;
    blendState_.pAttachments = colorBlendAttachmentStates_;
    blendState_.blendConstants[0] = 0.0f;
    blendState_.blendConstants[1] = 0.0f;
    blendState_.blendConstants[2] = 0.0f;
    blendState_.blendConstants[3] = 0.0f;

    //////////////////////////////////

    dynamicStateItems_[0] = VK_DYNAMIC_STATE_SCISSOR;
    dynamicStateItems_[1] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStateItems_[2] = VK_DYNAMIC_STATE_POLYGON_MODE_EXT;

    dynamicState_.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState_.pNext = nullptr;
    dynamicState_.flags = VK_FLAGS_NONE;
    dynamicState_.dynamicStateCount = 3;
    dynamicState_.pDynamicStates = dynamicStateItems_;
}

void Pipeline::Descriptor::SetPipelineType(PipelineType type)
{
    if (type == PIPELINE_TYPE_COMPUTE)
    {
        DRE_ASSERT(vertexAttributeCount_ == 0, "Incompatible state detected in Pipeline::Descriptor - vertexAttrCount != 0 on compute.");
        DRE_ASSERT(colorOutputCount_ == 0, "Incompatible state detected in Pipeline::Descriptor - viewportsCount != 0 on compute.");

        type_ = type;
    }
    else if (type == PIPELINE_TYPE_GRAPHIC)
    {
        type_ = type;
    }
    else
    {
        DRE_ASSERT(false, "Unknown PipelineType.");
    }
}

void Pipeline::Descriptor::SetVertexShader(ShaderModule const& vertexModule)
{
    DRE_ASSERT(type_ == PIPELINE_TYPE_GRAPHIC, "Incompatible state detected in Pipeline::Descriptor - setting vertex shader for non-graphic pipeline.");

    VkPipelineShaderStageCreateInfo& info = shaderStages_[shaderStagesCount_++];
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VK_FLAGS_NONE;
    info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    info.module = vertexModule.GetHandle();
    info.pName = vertexModule.GetEntryPoint();
    info.pSpecializationInfo = nullptr;
}

void Pipeline::Descriptor::SetFragmentShader(ShaderModule const& fragmentModule)
{
    DRE_ASSERT(type_ == PIPELINE_TYPE_GRAPHIC, "Incompatible state detected in Pipeline::Descriptor - setting fragment shader for non-graphic pipeline.");

    VkPipelineShaderStageCreateInfo& info = shaderStages_[shaderStagesCount_++];
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VK_FLAGS_NONE;
    info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    info.module = fragmentModule.GetHandle();
    info.pName = fragmentModule.GetEntryPoint();
    info.pSpecializationInfo = nullptr;
}

void Pipeline::Descriptor::SetComputeShader(ShaderModule const& computeShader)
{
    DRE_ASSERT(type_ == PIPELINE_TYPE_COMPUTE, "Incompatible state detected in Pipeline::Descriptor - setting compute shader for non-compute pipeline.");

    VkPipelineShaderStageCreateInfo& info = shaderStages_[shaderStagesCount_++];
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VK_FLAGS_NONE;
    info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.module = computeShader.GetHandle();
    info.pName = computeShader.GetEntryPoint();
    info.pSpecializationInfo = nullptr;
}

void Pipeline::Descriptor::SetLayout(PipelineLayout const* layout)
{
    pipelineLayout_ = layout;
}

void Pipeline::Descriptor::EnableDepthTest(Format depthFormat, bool write)
{
    depthTestEnabled_ = true;

    depthStencilState_.depthTestEnable = VK_TRUE;
    depthStencilState_.depthWriteEnable = write ? VK_TRUE : VK_FALSE;
    depthStencilState_.depthCompareOp = VK_COMPARE_OP_GREATER;
    depthStencilState_.depthBoundsTestEnable = VK_FALSE;

    depthAttachmentFormat_ = VKW::Format2VK(depthFormat);
}

void Pipeline::Descriptor::SetCullMode(VkCullModeFlags flags)
{
    rasterizationState_.cullMode = flags;
}

void Pipeline::Descriptor::SetPolygonMode(VkPolygonMode mode)
{
    rasterizationState_.polygonMode = mode;
}

void Pipeline::Descriptor::SetWindingOrder(WindingOrder face)
{
    rasterizationState_.frontFace = VkFrontFace(face);
}

void Pipeline::Descriptor::AddVertexAttribute(Format format)
{
    DRE_ASSERT(type_ != PIPELINE_TYPE_COMPUTE, "Incompatible state detected in Pipeline::Descritor - adding vertex to compute state.");

    VkVertexInputAttributeDescription& attr = vertexAttributeDescriptions_[vertexAttributeCount_++];
    attr.location = vertexAttributeCount_ - 1;
    attr.binding = 0; // we have 1 binding for all attributes
    attr.format = Format2VK(format);
    attr.offset = vertexBindingDescription_.stride;

    vertexInputState_.vertexBindingDescriptionCount = 1;

    vertexBindingDescription_.stride += FormatSize(format);
}

void Pipeline::Descriptor::AddColorOutput(Format format, BlendType blend)
{
    DRE_ASSERT(colorOutputCount_ < VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS, "MAX_VIEWPORTS overflow for Graphics pipeline descriptor");

    VkPipelineColorBlendAttachmentState& attachmentBlendState = colorBlendAttachmentStates_[colorOutputCount_];
    colorAttachmentFormats_[colorOutputCount_] = VKW::Format2VK(format);
    colorOutputCount_++;

    if (blend == BLEND_TYPE_NONE)
    {
        attachmentBlendState.blendEnable = VK_FALSE;
        attachmentBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        attachmentBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        attachmentBlendState.colorBlendOp = VK_BLEND_OP_ADD; // don't care
        attachmentBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachmentBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachmentBlendState.alphaBlendOp = VK_BLEND_OP_ADD; // don't care
        attachmentBlendState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    }
    else if (blend == BLEND_TYPE_ALPHA_OVER)
    {
        attachmentBlendState.blendEnable = VK_TRUE;
        attachmentBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachmentBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachmentBlendState.colorBlendOp = VK_BLEND_OP_ADD;
        attachmentBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachmentBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachmentBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
        attachmentBlendState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    }
    else
    {
        DRE_ASSERT(false, "Unknown blend type.");
    }
}

VkGraphicsPipelineCreateInfo const& Pipeline::Descriptor::CompileGraphicPipelineCreateInfo()
{
    DRE_ASSERT(type_ == PIPELINE_TYPE_GRAPHIC, "Compiling invalid pipeline type descriptor.");

    renderingCreateInfo_.colorAttachmentCount = colorOutputCount_;
    renderingCreateInfo_.pColorAttachmentFormats = colorAttachmentFormats_;
    renderingCreateInfo_.depthAttachmentFormat = depthAttachmentFormat_;
    renderingCreateInfo_.stencilAttachmentFormat = stencilAttachmentFormat_;

    graphicsCreateInfo_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsCreateInfo_.pNext = &renderingCreateInfo_;
    graphicsCreateInfo_.flags = VK_FLAGS_NONE;
    graphicsCreateInfo_.stageCount = shaderStagesCount_;
    graphicsCreateInfo_.pStages = shaderStages_;

    vertexInputState_.pVertexBindingDescriptions = &vertexBindingDescription_;
    vertexInputState_.vertexAttributeDescriptionCount = vertexAttributeCount_;
    vertexInputState_.pVertexAttributeDescriptions = vertexAttributeDescriptions_;
    graphicsCreateInfo_.pVertexInputState = &vertexInputState_;

    graphicsCreateInfo_.pInputAssemblyState = &inputAssemblyState_;
    graphicsCreateInfo_.pTessellationState = nullptr;

    viewportState_.viewportCount = colorOutputCount_ != 0 ? colorOutputCount_ : ((depthTestEnabled_ || stencilTestEnabled_) ? 1 : 0);
    viewportState_.scissorCount = colorOutputCount_ != 0 ? colorOutputCount_ : ((depthTestEnabled_ || stencilTestEnabled_) ? 1 : 0);
    graphicsCreateInfo_.pViewportState = &viewportState_;

    graphicsCreateInfo_.pRasterizationState = &rasterizationState_;
    graphicsCreateInfo_.pMultisampleState = &multisampleState_;
    graphicsCreateInfo_.pDepthStencilState = &depthStencilState_;

    blendState_.attachmentCount = colorOutputCount_;
    blendState_.pAttachments = colorBlendAttachmentStates_;
    graphicsCreateInfo_.pColorBlendState = &blendState_;

    dynamicState_.pDynamicStates = dynamicStateItems_;
    graphicsCreateInfo_.pDynamicState = &dynamicState_;

    graphicsCreateInfo_.layout = pipelineLayout_->GetHandle();
    graphicsCreateInfo_.renderPass = VK_NULL_HANDLE;
    graphicsCreateInfo_.subpass = 0;

    graphicsCreateInfo_.basePipelineHandle = VK_NULL_HANDLE;
    graphicsCreateInfo_.basePipelineIndex = 0;

    // reset this for easier reuse later
    shaderStagesCount_ = 0;

    return graphicsCreateInfo_;
}

VkComputePipelineCreateInfo const& Pipeline::Descriptor::CompileComputePipelineCreateInfo()
{
    DRE_ASSERT(type_ == PIPELINE_TYPE_COMPUTE, "Compiling invalid pipeline type descriptor.");

    computeCreateInfo_.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computeCreateInfo_.pNext = nullptr;
    computeCreateInfo_.flags = VK_FLAGS_NONE;
    computeCreateInfo_.stage = shaderStages_[0];
    computeCreateInfo_.layout = pipelineLayout_->GetHandle();
    computeCreateInfo_.basePipelineHandle = VK_NULL_HANDLE;
    computeCreateInfo_.basePipelineIndex = 0;

    shaderStagesCount_ = 0;

    return computeCreateInfo_;
}

Pipeline::Pipeline()
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , layout_{ nullptr }
{
}

Pipeline::Pipeline(ImportTable* table, LogicalDevice* device, Descriptor& descriptor)
    : table_{ table }
    , device_{ device }
    , handle_{ VK_NULL_HANDLE }
    , layout_{ nullptr }
{
    if (descriptor.GetPipelineType() == PIPELINE_TYPE_GRAPHIC)
    {
        VkGraphicsPipelineCreateInfo const& createInfo = descriptor.CompileGraphicPipelineCreateInfo();
        VK_ASSERT(table_->vkCreateGraphicsPipelines(device_->Handle(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle_));
    }
    else if (descriptor.GetPipelineType() == PIPELINE_TYPE_COMPUTE)
    {
        VkComputePipelineCreateInfo const& createInfo = descriptor.CompileComputePipelineCreateInfo();
        VK_ASSERT(table_->vkCreateComputePipelines(device_->Handle(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle_));
    }
    else
    {
        DRE_ASSERT(false, "Attempt to create unsupported pipeline type.");
    }

    layout_ = descriptor.GetLayout();
    descriptor_ = descriptor;
}

Pipeline::Pipeline(Pipeline&& rhs)
    : table_{ nullptr }
    , device_{ nullptr }
    , handle_{ VK_NULL_HANDLE }
    , layout_{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

Pipeline& Pipeline::operator=(Pipeline&& rhs)
{
    DRE_SWAP_MEMBER(table_);
    DRE_SWAP_MEMBER(device_);
    DRE_SWAP_MEMBER(handle_);
    DRE_SWAP_MEMBER(layout_);

    return *this;
}

Pipeline::~Pipeline()
{
    if (handle_ != VK_NULL_HANDLE)
    {
        table_->vkDestroyPipeline(device_->Handle(), handle_, nullptr);
    }
}

}

