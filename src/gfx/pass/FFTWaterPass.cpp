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

static std::uint32_t constexpr WATER_UNIFORM_SIZE = sizeof(glm::vec4) * 3;

void FillWaterUniform(UniformProxy& uniform, Texture const& noiseTexture)
{
    GraphicsSettings const& s = g_GraphicsManager->GetGraphicsSettings();

    WIND_DIR[0] = s.m_WindDirectionX;
    WIND_DIR = glm::normalize(WIND_DIR);

    VKW::TextureDescriptorIndex id = noiseTexture.GetShaderGlobalDescriptor();
    float noiseTexID = *reinterpret_cast<float*>(&id.id_);

    glm::vec4 _0{ C_WATER_DIM, WIND_DIR[0], WIND_DIR[1], noiseTexID };
    glm::vec4 _1{ s.m_WindSpeed, s.m_WaterSpeed, s.m_WaterSizeMeters, s.m_WaterAmplitude };
    glm::vec4 _2{ s.m_WindDirFactor, 0.0f, 0.0f, 0.0f };

    uniform.WriteMember140(_0);
    uniform.WriteMember140(_1);
    uniform.WriteMember140(_2);
}

PassID GFX::FFTButterflyGenPass::GetID() const
{
    return PassID::FFTButterflyGen;
}

void FFTButterflyGenPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(C_WATER_DIM)));
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTButterfly), VKW::FORMAT_R32G32B32A32_FLOAT, stagesCount, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 1);
}

void FFTButterflyGenPass::Initialize(RenderGraph& graph)
{

}

void FFTButterflyGenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_ASSERT(C_WATER_DIM <= 256, "Can't do dimentions more that 256 (for now)");

    std::uint32_t* bit_reversed = (std::uint32_t*)DRE::g_FrameScratchAllocator.Alloc(C_WATER_DIM * sizeof(std::uint32_t), alignof(std::uint32_t));
    for (std::uint32_t i = 0; i < C_WATER_DIM; i++)
    {
        bit_reversed[i] = DRE::BitReverse(static_cast<std::uint8_t>(i));
    }


    std::uint32_t const uniformSize = sizeof(glm::uvec4) + sizeof(std::uint32_t) * C_WATER_DIM;

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, uniformSize);
    uniform.WriteMember140(glm::ivec4{ C_WATER_DIM, 0, 0, 0 });
    uniform.WriteMember140(bit_reversed, C_WATER_DIM * sizeof(*bit_reversed));

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gen_butterfly");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    VKW::ImageResourceView* texture = graph.GetTexture(RESOURCE_ID(TextureID::FFTButterfly))->GetShaderView();
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, texture->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);


    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);
    
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(C_WATER_DIM)));
    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipeline);
    context.CmdDispatch(std::max(stagesCount / group_dims[0], 1u), C_WATER_DIM / group_dims[1], 1);
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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTH0), VKW::FORMAT_R32G32B32A32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 1);
}

void FFTWaterH0GenPass::Initialize(RenderGraph& graph)
{
}

void FFTWaterH0GenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    UniformProxy uniform = graph.GetPassUniform(GetID(), context, WATER_UNIFORM_SIZE);
    FillWaterUniform(uniform, *g_GraphicsManager->GetTextureBank().FindTexture("blue_noise_256"));

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gen_h0");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    
    VKW::ImageResourceView* texture = graph.GetTexture(RESOURCE_ID(TextureID::FFTH0))->GetShaderView();
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, texture->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);


    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);
    
    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipeline);
    context.CmdDispatch(C_WATER_DIM / group_dims[0], C_WATER_DIM / group_dims[1], 1);
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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTHxt), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTH0), VKW::FORMAT_R32G32B32A32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 1);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 2);
}

void FFTWaterHxtGenPass::Initialize(RenderGraph& graph)
{
}

void FFTWaterHxtGenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    UniformProxy uniform = graph.GetPassUniform(GetID(), context, WATER_UNIFORM_SIZE);
    FillWaterUniform(uniform, *g_GraphicsManager->GetTextureBank().FindTexture("blue_noise_256"));

    VKW::ImageResourceView* fftHxt = graph.GetTexture(RESOURCE_ID(TextureID::FFTHxt))->GetShaderView();
    VKW::ImageResourceView* fftH0 = graph.GetTexture(RESOURCE_ID(TextureID::FFTH0))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftHxt->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftH0->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gen_hxt");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);

    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipeline);
    context.CmdDispatch(C_WATER_DIM / group_dims[0], C_WATER_DIM / group_dims[1], 1);
}


////////////////////////////
////////////////////////////
////////////////////////////
////////////////////////////
PassID GFX::FFTWaterFFTPass::GetID() const
{
    return PassID::FFTWaterHeightGen;
}

void FFTWaterFFTPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(C_WATER_DIM)));

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTButterfly), VKW::FORMAT_R32G32B32A32_FLOAT, stagesCount, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 0);

    graph.RegisterTextureSlot(this, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 1);
    graph.RegisterTextureSlot(this, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 2);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 3);

    graph.RegisterStandaloneTexture(RESOURCE_ID(TextureID::FFTPingPong0), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::ResourceAccess(VKW::RESOURCE_ACCESS_SHADER_RW));
    graph.RegisterStandaloneTexture(RESOURCE_ID(TextureID::FFTPingPong1), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_RW);
}

void FFTWaterFFTPass::Initialize(RenderGraph& graph)
{
    VKW::DescriptorManager* manager = g_GraphicsManager->GetMainDevice()->GetDescriptorManager();
    VKW::DescriptorSetLayout const* layout = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID()).GetLayout();
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(C_WATER_DIM)));
    for (std::uint32_t i = 0; i < stagesCount * 2; i++)
    {
        m_StageSets0.EmplaceBack(manager->AllocateStandaloneSet(*layout));
        m_StageSets1.EmplaceBack(manager->AllocateStandaloneSet(*layout));
    }
}

static VKW::ImageResourceView* fftOutput = nullptr;

void FFTWaterFFTPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* fftHxt = graph.GetTexture(RESOURCE_ID(TextureID::FFTHxt))->GetShaderView();
    VKW::ImageResourceView* fftButterfly = graph.GetTexture(RESOURCE_ID(TextureID::FFTButterfly))->GetShaderView();

    VKW::ImageResourceView* pingPong0 = graph.GetTexture(RESOURCE_ID(TextureID::FFTPingPong0))->GetShaderView();
    VKW::ImageResourceView* pingPong1 = graph.GetTexture(RESOURCE_ID(TextureID::FFTPingPong1))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, pingPong0->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftHxt->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    context.CmdCopyImageToImage(pingPong0->parentResource_, fftHxt->parentResource_);


    VKW::ImageResourceView* input = pingPong0;
    VKW::ImageResourceView* output = pingPong1;

    VKW::DescriptorManager* manager = g_GraphicsManager->GetMainDevice()->GetDescriptorManager();

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("fft_iter");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();

    auto& setVector = g_GraphicsManager->GetCurrentGraphicsFrame() % 2 == 0 ? m_StageSets0 : m_StageSets1;

    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(C_WATER_DIM)));

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
                uniform.WriteMember140(glm::vec4{ isVertical, i % stagesCount, 0.0f, 0.0f });
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

            context.CmdDispatch(C_WATER_DIM / 8, C_WATER_DIM / 8, 1);

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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTPingPong0), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE, 0);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::WaterHeight), VKW::FORMAT_R32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE, 1);

    graph.RegisterUniformBuffer(this, VKW::STAGE_COMPUTE, 2);
}

void FFTInvPermutationPass::Initialize(RenderGraph& graph)
{
}

void FFTInvPermutationPass::Render(RenderGraph& graph, VKW::Context& context)
{
    VKW::ImageResourceView* input = graph.GetTexture(RESOURCE_ID(TextureID::FFTPingPong0))->GetShaderView();
    VKW::ImageResourceView* heightMap = graph.GetTexture(RESOURCE_ID(TextureID::WaterHeight))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, input->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, heightMap->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.GetPassUniform(GetID(), context, WATER_UNIFORM_SIZE);
    FillWaterUniform(uniform, *g_GraphicsManager->GetTextureBank().FindTexture("blue_noise_256"));

    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("fft_inv_perm");
    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());

    std::uint32_t const firstSet = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();
    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    context.CmdBindComputeDescriptorSets(layout, firstSet, 1, &set);

    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipeline);
    context.CmdDispatch(C_WATER_DIM / group_dims[0], C_WATER_DIM / group_dims[1], 1);
}

}
