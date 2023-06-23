#include <gfx\pass\FFTWaterPass.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>

namespace GFX
{

static glm::vec2 WIND_DIR = glm::vec2{ 1.0f, 1.0f };

PassID GFX::FFTButterflyGenPass::GetID() const
{
    return PassID::FFTButterflyGen;
}

void FFTButterflyGenPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(WATER_DIM)));
    graph.RegisterTexture(this, TextureID::FFTButterfly, VKW::FORMAT_R32G32B32A32_FLOAT, stagesCount, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 1);
}

void FFTButterflyGenPass::Initialize(RenderGraph& graph)
{

}

void FFTButterflyGenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_ASSERT(WATER_DIM <= 256, "Can't do dimentions more that 256 (for now)");

    std::uint32_t* bit_reversed = (std::uint32_t*)DRE::g_FrameScratchAllocator.Alloc(WATER_DIM * sizeof(std::uint32_t), alignof(std::uint32_t));
    for (std::uint32_t i = 0; i < WATER_DIM; i++)
    {
        bit_reversed[i] = DRE::BitReverse(static_cast<std::uint8_t>(i));
    }


    std::uint32_t const uniformSize = sizeof(glm::uvec4) + sizeof(std::uint32_t) * WATER_DIM;

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, uniformSize);
    uniform.WriteMember140(glm::ivec4{ WATER_DIM, 0, 0, 0 });
    uniform.WriteMember140(bit_reversed, WATER_DIM * sizeof(*bit_reversed));

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gen_butterfly");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    VKW::ImageResourceView* texture = graph.GetTexture(TextureID::FFTButterfly)->GetShaderView();
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, texture->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);


    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);
    
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(WATER_DIM)));
    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipeline);
    context.CmdDispatch(std::max(stagesCount / group_dims[0], 1u), WATER_DIM / group_dims[1], 1);
}


////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
PassID GFX::FFTWaterH0GenPass::GetID() const
{
    return PassID::FFTWaterH0Gen;
}

void FFTWaterH0GenPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterTexture(this, TextureID::FFTH0, VKW::FORMAT_R32G32B32A32_FLOAT, WATER_DIM, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 1);
}

void FFTWaterH0GenPass::Initialize(RenderGraph& graph)
{
}

void FFTWaterH0GenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    std::uint32_t const uniformSize = sizeof(glm::uvec4);
    
    VKW::TextureDescriptorIndex noiseTexture = g_GraphicsManager->GetTextureBank().FindTexture("blue_noise_256")->GetShaderReadDescriptor();
    float noiseTexID = *reinterpret_cast<float*>(&noiseTexture.id_);

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, uniformSize);
    uniform.WriteMember140(glm::vec4{ static_cast<float>(WATER_DIM), WIND_DIR[0], WIND_DIR[1], noiseTexID });

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gen_h0");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    
    VKW::ImageResourceView* texture = graph.GetTexture(TextureID::FFTH0)->GetShaderView();
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, texture->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);


    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);
    
    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipeline);
    context.CmdDispatch(WATER_DIM / group_dims[0], WATER_DIM / group_dims[1], 1);
}


////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
PassID GFX::FFTWaterHxtGenPass::GetID() const
{
    return PassID::FFTWaterHxtGen;
}

void FFTWaterHxtGenPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterTexture(this, TextureID::FFTHxt, VKW::FORMAT_R32G32_FLOAT, WATER_DIM, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterTexture(this, TextureID::FFTH0, VKW::FORMAT_R32G32B32A32_FLOAT, WATER_DIM, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 1);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 2);
}

void FFTWaterHxtGenPass::Initialize(RenderGraph& graph)
{
}

void FFTWaterHxtGenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    std::uint32_t const uniformSize = sizeof(glm::uvec4);

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, uniformSize);
    uniform.WriteMember140(glm::vec4{static_cast<float>(WATER_DIM), WIND_DIR[0], WIND_DIR[1], 0 });

    VKW::ImageResourceView* fftHxt = graph.GetTexture(TextureID::FFTHxt)->GetShaderView();
    VKW::ImageResourceView* fftH0 = graph.GetTexture(TextureID::FFTH0)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftHxt->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftH0->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gen_hxt");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);

    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipeline);
    context.CmdDispatch(WATER_DIM / group_dims[0], WATER_DIM / group_dims[1], 1);
}


////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
PassID GFX::FFTWaterHeightGenPass::GetID() const
{
    return PassID::FFTWaterHeightGen;
}

void FFTWaterHeightGenPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(WATER_DIM)));

    graph.RegisterTexture(this, TextureID::FFTButterfly, VKW::FORMAT_R32G32B32A32_FLOAT, stagesCount, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 0);

    graph.RegisterTextureSlot(this, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 1);
    graph.RegisterTextureSlot(this, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 2);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 3);

    graph.RegisterStandaloneTexture(TextureID::FFTPingPong0, VKW::FORMAT_R32G32_FLOAT, WATER_DIM, WATER_DIM, VKW::ResourceAccess(VKW::RESOURCE_ACCESS_SHADER_RW));
    graph.RegisterStandaloneTexture(TextureID::FFTPingPong1, VKW::FORMAT_R32G32_FLOAT, WATER_DIM, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_RW);
}

void FFTWaterHeightGenPass::Initialize(RenderGraph& graph)
{
    VKW::DescriptorManager* manager = g_GraphicsManager->GetMainDevice()->GetDescriptorManager();
    VKW::DescriptorSetLayout const* layout = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID()).GetLayout();
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(WATER_DIM)));
    for (std::uint32_t i = 0; i < stagesCount * 2; i++)
    {
        m_StageSets0.EmplaceBack(manager->AllocateStandaloneSet(*layout));
        m_StageSets1.EmplaceBack(manager->AllocateStandaloneSet(*layout));
    }
}

static VKW::ImageResourceView* fftOutput = nullptr;

void FFTWaterHeightGenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* fftHxt = graph.GetTexture(TextureID::FFTHxt)->GetShaderView();
    VKW::ImageResourceView* fftButterfly = graph.GetTexture(TextureID::FFTButterfly)->GetShaderView();

    VKW::ImageResourceView* pingPong0 = graph.GetTexture(TextureID::FFTPingPong0)->GetShaderView();
    VKW::ImageResourceView* pingPong1 = graph.GetTexture(TextureID::FFTPingPong1)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, pingPong0->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftHxt->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    context.CmdCopyImageToImage(pingPong0->parentResource_, fftHxt->parentResource_);


    VKW::ImageResourceView* input = pingPong0;
    VKW::ImageResourceView* output = pingPong1;

    VKW::DescriptorManager* manager = g_GraphicsManager->GetMainDevice()->GetDescriptorManager();

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gen_water_height");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();

    auto& setVector = g_GraphicsManager->GetCurrentGraphicsFrame() % 2 == 0 ? m_StageSets0 : m_StageSets1;
//    g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->WriteDescriptorSet(set, descWrite);

    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(WATER_DIM)));

    context.CmdBindComputePipeline(pipeline);

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftButterfly->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);


    // horizontal
    {
        for (std::uint32_t i = 0; i < stagesCount * 2; i++)
        {
            auto uniformAllocation = g_GraphicsManager->GetUniformArena().AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), sizeof(glm::vec4), 256);
            {
                UniformProxy uniform{ &context, uniformAllocation };
                float const isVertical = i >= stagesCount ? 1.0f : 0.0f;
                uniform.WriteMember140(glm::vec4{ isVertical, i, 0.0f, 0.0f });
            }

            VKW::DescriptorManager::WriteDesc writeDesc;
            writeDesc.AddStorageImage(fftButterfly, 0);
            writeDesc.AddStorageImage(input, 1);
            writeDesc.AddStorageImage(output, 2);
            writeDesc.AddUniform(uniformAllocation.m_Buffer, uniformAllocation.m_OffsetInBuffer, uniformAllocation.m_Size, 3);

            auto& set = setVector[i];
            manager->WriteDescriptorSet(set, writeDesc);
            
            context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);
            
            g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, input->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);
            g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, output->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

            context.CmdDispatch(WATER_DIM / 8, WATER_DIM / 8, 1);

            DRE_SWAP(input, output);
        }
    }
}



////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
PassID GFX::FFTInvPermutationPass::GetID() const
{
    return PassID::FFTWaterInvPerm;
}

void FFTInvPermutationPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterTexture(this, TextureID::FFTPingPong0, VKW::FORMAT_R32G32_FLOAT, WATER_DIM, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterTexture(this, TextureID::WaterHeight, VKW::FORMAT_R32_FLOAT, WATER_DIM, WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 1);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 2);
}

void FFTInvPermutationPass::Initialize(RenderGraph& graph)
{
}

void FFTInvPermutationPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* input = graph.GetTexture(TextureID::FFTPingPong0)->GetShaderView();
    VKW::ImageResourceView* heightMap = graph.GetTexture(TextureID::WaterHeight)->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, input->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, heightMap->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    
    std::uint32_t const uniformSize = sizeof(glm::vec4);

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, uniformSize);
    uniform.WriteMember140(glm::vec4{ static_cast<float>(WATER_DIM), 0.0f, 0.0f, 0.0f });

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("fft_inv_perm");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);

    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipeline);
    context.CmdDispatch(WATER_DIM / group_dims[0], WATER_DIM / group_dims[1], 1);
}

}
