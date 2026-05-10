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

enum ShaderStageSlot : DRE::U8
{
    SHADER_STAGE_SLOT_VERTEX   = 0,
    SHADER_STAGE_SLOT_FRAGMENT = 1,
    SHADER_STAGE_SLOT_COMPUTE  = 2,
    SHADER_STAGE_SLOT_MAX      = 3
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
    static DRE::U32 constexpr MAX_SHADER_STAGES = SHADER_STAGE_SLOT_MAX;
    static DRE::U32 constexpr MAX_VERTEX_ATTRIBUTES = 6;

public:
    class Descriptor
    {
    public:
        Descriptor();

        inline PipelineType GetPipelineType() const { return type_; }
        inline PipelineLayout const* GetLayout() const { return pipelineLayout_; }

        inline bool IsShaderStagePresent(ShaderStageSlot slot) const { return (shaderStageMask_ >> slot) & 1; }
        inline DRE::String64 const& GetShaderStageName(ShaderStageSlot slot) const { return shaderStages_[slot].shaderName; }
        inline DRE::String64 const& GetShaderStageEntryPoint(ShaderStageSlot slot) const { return shaderStages_[slot].entryPoint; }
 

        //inline void ClearShaderStages() { shaderStagesCount_ = 0; DRE::MemZero(shaderStages_, sizeof(shaderStages_); DRE::MemZero(shaderStageNames_, sizeof(shaderStageNames_)); }

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

        // mirrors VkColorComponentFlagBits
        enum ColorComponent : DRE::U8
        {
            R_BIT = 0x00000001,
            G_BIT = 0x00000002,
            B_BIT = 0x00000004,
            A_BIT = 0x00000008,
        };
        void AddColorOutput         (Format format, BlendType blend = BLEND_TYPE_NONE, DRE::U8 writeMask = 0xF);

        VkGraphicsPipelineCreateInfo const& CompileGraphicPipelineCreateInfo();
        VkComputePipelineCreateInfo const& CompileComputePipelineCreateInfo();


    private:
        PipelineType                                type_;
        bool                                        depthTestEnabled_ : 1;
        bool                                        stencilTestEnabled_ : 1;

        VkPipelineRenderingCreateInfo               renderingCreateInfo_;

        VkGraphicsPipelineCreateInfo                graphicsCreateInfo_;
        VkComputePipelineCreateInfo                 computeCreateInfo_;

        DRE::U8                                     shaderStageMask_;

        struct ShaderStageInfo
        {
            DRE::String64                   shaderName;
            DRE::String64                   entryPoint;
            VkPipelineShaderStageCreateInfo createInfo;
        };
        ShaderStageInfo                             shaderStages_[MAX_SHADER_STAGES];
        VkPipelineShaderStageCreateInfo             compiledShaderStages_[MAX_SHADER_STAGES];

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