#pragma once

#include <vulkan\vulkan.h>
#include <cstdint>

#include <class_features\NonMovable.hpp>
#include <class_features\NonCopyable.hpp>

#include <vk_interface\descriptor\DescriptorLayout.hpp>
#include <vk_interface\pipeline\RenderPass.hpp>

namespace VKW
{

class ImportTable;
class LogicalDevice;
class PipelineLayout;
class RenderPass;
class ShaderModule;

enum PipelineType
{
    PIPELINE_TYPE_GRAPHIC,
    PIPELINE_TYPE_COMPUTE
};

enum BlendType
{
    BLEND_TYPE_NONE,
    BLEND_TYPE_ALPHA_OVER,
    BLEND_TYPE_MAX
};

class Pipeline
    : public NonCopyable
{
public:
    static std::uint32_t constexpr MAX_SHADER_STAGES = 6;
    static std::uint32_t constexpr MAX_VERTEX_ATTRIBUTES = 6;

public:
    class Descriptor : public NonMovable, public NonCopyable
    {
    public:
        Descriptor();

        inline PipelineType GetPipelineType() const { return type_; }
        inline PipelineLayout const* GetLayout() const { return pipelineLayout_; }

        void SetPipelineType        (PipelineType type);
        void SetVertexShader        (ShaderModule const& vertexModule);
        void SetFragmentShader      (ShaderModule const& fragmentModule);
        void SetComputeShader       (ShaderModule const& computeShader);

        void SetLayout              (PipelineLayout const* layout);

        void EnableDepthStencilTest (Format depthFormat);
        void SetCullMode            (VkCullModeFlags mode);

        void AddVertexAttribute     (Format format);
        void AddOutputViewport      (Format format, BlendType blend);

        VkGraphicsPipelineCreateInfo const& CompileGraphicPipelineCreateInfo();
        VkComputePipelineCreateInfo const& CompileComputePipelineCreateInfo();


    private:
        PipelineType                                type_;

        VkPipelineRenderingCreateInfo               renderingCreateInfo_;

        VkGraphicsPipelineCreateInfo                graphicsCreateInfo_;
        VkComputePipelineCreateInfo                 computeCreateInfo_;

        std::uint8_t                                shaderStagesCount_;
        VkPipelineShaderStageCreateInfo             shaderStages_[MAX_SHADER_STAGES];

        VkPipelineVertexInputStateCreateInfo        vertexInputState_;
        VkVertexInputBindingDescription             vertexBindingDescription_;
        std::uint8_t                                vertexAttributeCount_;
        VkVertexInputAttributeDescription           vertexAttributeDescriptions_[MAX_VERTEX_ATTRIBUTES];

        VkPipelineInputAssemblyStateCreateInfo      inputAssemblyState_;

        VkPipelineViewportStateCreateInfo           viewportState_;
        std::uint8_t                                viewportsCount_;
        //VkViewport                                  viewports_[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
        //VkRect2D                                    scissors_[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];

        VkPipelineRasterizationStateCreateInfo      rasterizationState_;
        VkPipelineMultisampleStateCreateInfo        multisampleState_;
        VkPipelineDepthStencilStateCreateInfo       depthStencilState_;

        VkPipelineColorBlendStateCreateInfo         blendState_;
        VkPipelineColorBlendAttachmentState         colorBlendAttachmentStates_[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
        VkFormat                                    colorAttachmentFormats_[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
        VkFormat                                    depthAttachmentFormat_;
        VkFormat                                    stencilAttachmentFormat_;

        VkDynamicState                              dynamicStateItems_[2];
        VkPipelineDynamicStateCreateInfo            dynamicState_;

        PipelineLayout const*                       pipelineLayout_;
    };

public:
    Pipeline();
    Pipeline(ImportTable* table, LogicalDevice* device, Descriptor& descriptor);

    Pipeline(Pipeline&& rhs);
    Pipeline& operator=(Pipeline&& rhs);

    ~Pipeline();

    inline VkPipeline               GetHandle() const { return handle_; }
    inline PipelineLayout const*    GetLayout() const { return layout_; }


private:
    ImportTable*            table_;
    LogicalDevice*                 device_;

    VkPipeline              handle_;
    PipelineLayout const*   layout_;
};

}