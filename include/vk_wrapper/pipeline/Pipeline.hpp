#pragma once

#include <vulkan\vulkan.h>
#include <foundation\Common.hpp>

#include <foundation\class_features\NonMovable.hpp>
#include <foundation\class_features\NonCopyable.hpp>

#include <vk_wrapper\descriptor\DescriptorLayout.hpp>
#include <vk_wrapper\pipeline\RenderPass.hpp>

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
    PIPELINE_TYPE_COMPUTE,
    PIEPLINE_TYPE_INVALID
};

enum BlendType
{
    BLEND_TYPE_NONE,
    BLEND_TYPE_ALPHA_OVER,
    BLEND_TYPE_MAX
};

enum WindingOrder
{
    WINDING_ORDER_CLOCKWIZE         = VK_FRONT_FACE_CLOCKWISE,
    WINDING_RODER_COUNTER_CLOCKWIZE = VK_FRONT_FACE_COUNTER_CLOCKWISE
};

class Pipeline
    : public NonCopyable
{
public:
    static DRE::U32 constexpr MAX_SHADER_STAGES = 6;
    static DRE::U32 constexpr MAX_VERTEX_ATTRIBUTES = 6;

public:
    class Descriptor
    {
    public:
        Descriptor();

        inline PipelineType GetPipelineType() const { return type_; }
        inline PipelineLayout const* GetLayout() const { return pipelineLayout_; }

        inline DRE::U8 GetShaderStageCount() const { return shaderStagesCount_; }
        inline DRE::String64 const& GetShaderStageName(DRE::U8 i) const { return shaderStageNames_[i]; }

        void SetPipelineType        (PipelineType type);
        void SetVertexShader        (ShaderModule const& vertexModule);
        void SetFragmentShader      (ShaderModule const& fragmentModule);
        void SetComputeShader       (ShaderModule const& computeShader);

        void SetLayout              (PipelineLayout const* layout);

        void EnableDepthTest        (Format depthFormat, bool write = true);
        void SetCullMode            (VkCullModeFlags mode);
        void SetPolygonMode         (VkPolygonMode mode);
        void SetWindingOrder        (WindingOrder order);

        void AddVertexAttribute     (Format format);
        void AddColorOutput         (Format format, BlendType blend = BLEND_TYPE_NONE);

        VkGraphicsPipelineCreateInfo const& CompileGraphicPipelineCreateInfo();
        VkComputePipelineCreateInfo const& CompileComputePipelineCreateInfo();


    private:
        PipelineType                                type_;
        bool                                        depthTestEnabled_ : 1;
        bool                                        stencilTestEnabled_ : 1;

        VkPipelineRenderingCreateInfo               renderingCreateInfo_;

        VkGraphicsPipelineCreateInfo                graphicsCreateInfo_;
        VkComputePipelineCreateInfo                 computeCreateInfo_;

        DRE::U8                                     shaderStagesCount_;
        DRE::String64                               shaderStageNames_[MAX_SHADER_STAGES];
        VkPipelineShaderStageCreateInfo             shaderStages_[MAX_SHADER_STAGES];

        VkPipelineVertexInputStateCreateInfo        vertexInputState_;
        VkVertexInputBindingDescription             vertexBindingDescription_;
        DRE::U8                                     vertexAttributeCount_;
        VkVertexInputAttributeDescription           vertexAttributeDescriptions_[MAX_VERTEX_ATTRIBUTES];

        VkPipelineInputAssemblyStateCreateInfo      inputAssemblyState_;

        VkPipelineViewportStateCreateInfo           viewportState_;
        DRE::U8                                     colorOutputCount_;

        VkPipelineRasterizationStateCreateInfo      rasterizationState_;
        VkPipelineMultisampleStateCreateInfo        multisampleState_;
        VkPipelineDepthStencilStateCreateInfo       depthStencilState_;

        VkPipelineColorBlendStateCreateInfo         blendState_;
        VkPipelineColorBlendAttachmentState         colorBlendAttachmentStates_[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
        VkFormat                                    colorAttachmentFormats_[VKW::CONSTANTS::MAX_COLOR_ATTACHMENTS];
        VkFormat                                    depthAttachmentFormat_;
        VkFormat                                    stencilAttachmentFormat_;

        VkDynamicState                              dynamicStateItems_[3];
        VkPipelineDynamicStateCreateInfo            dynamicState_;

        PipelineLayout const*                       pipelineLayout_;
    };

public:
    Pipeline();
    Pipeline(ImportTable* table, LogicalDevice* device, Descriptor& descriptor, char const* name);

    Pipeline(Pipeline&& rhs);
    Pipeline& operator=(Pipeline&& rhs);

    ~Pipeline();

    inline VkPipeline               GetHandle() const { return handle_; }
    inline PipelineLayout const*    GetLayout() const { return layout_; }

    inline Descriptor&              GetDescriptor() { return descriptor_; }


private:
    ImportTable*            table_;
    LogicalDevice*          device_;

    VkPipeline              handle_;
    PipelineLayout const*   layout_;

    Descriptor              descriptor_;
    DRE::String128          name_;
};

}