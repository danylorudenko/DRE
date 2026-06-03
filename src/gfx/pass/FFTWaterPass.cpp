#include <gfx\pass\FFTWaterPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTButterfly), VKW::FORMAT_R32G32B32A32_FLOAT, stagesCount, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE);
}

void FFTButterflyGenPass::Initialize(RenderGraph& graph)
{

}

void FFTButterflyGenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(FFTButterflyGen);

    DRE_ASSERT(C_WATER_DIM <= 256, "Can't do dimentions more that 256 (for now)");

    std::uint32_t* bit_reversed = (std::uint32_t*)DRE::g_FrameScratchAllocator.Alloc(C_WATER_DIM * sizeof(std::uint32_t), alignof(std::uint32_t));
    for (std::uint32_t i = 0; i < C_WATER_DIM; i++)
    {
        bit_reversed[i] = DRE::BitReverse(static_cast<std::uint8_t>(i));
    }


    std::uint32_t const uniformSize = sizeof(glm::uvec4) + sizeof(std::uint32_t) * C_WATER_DIM;

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, uniformSize);
    uniform.WriteMember140(glm::ivec4{ C_WATER_DIM, 0, 0, 0 });
    uniform.WriteMember140(bit_reversed, C_WATER_DIM * sizeof(*bit_reversed));

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("gen_butterfly");

    Texture* butterfly = graph.GetTexture(RESOURCE_ID(TextureID::FFTButterfly));
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, butterfly->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);
    binder.AddStorageTexture(0, butterfly);
    binder.AddUniform(1, &uniform);
    binder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
    
    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(C_WATER_DIM)));
    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipelineEntry->GetPipeline());
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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTH0), VKW::FORMAT_R32G32B32A32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE);
}

void FFTWaterH0GenPass::Initialize(RenderGraph& graph)
{
}

void FFTWaterH0GenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(FFTWaterH0Gen);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, WATER_UNIFORM_SIZE);
    FillWaterUniform(uniform, *g_GraphicsManager->GetTextureBank().FindTexture("blue_noise_256"));

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("gen_h0");


    Texture* texture = graph.GetTexture(RESOURCE_ID(TextureID::FFTH0));
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, texture->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);
    binder.AddStorageTexture(0, texture);
    binder.AddUniform(1, &uniform);
    binder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
    
    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipelineEntry->GetPipeline());
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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTHxt), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTH0), VKW::FORMAT_R32G32B32A32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ);
}

void FFTWaterHxtGenPass::Initialize(RenderGraph& graph)
{
}

void FFTWaterHxtGenPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(FFTWaterHxtGen);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, WATER_UNIFORM_SIZE);
    FillWaterUniform(uniform, *g_GraphicsManager->GetTextureBank().FindTexture("blue_noise_256"));

    Texture* fftHxt = graph.GetTexture(RESOURCE_ID(TextureID::FFTHxt));
    Texture* fftH0 = graph.GetTexture(RESOURCE_ID(TextureID::FFTH0));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftHxt->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftH0->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("gen_hxt");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);
    binder.AddStorageTexture(0, fftHxt);
    binder.AddSampledTexture(1, fftH0);
    binder.AddUniform(2, &uniform);
    binder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipelineEntry->GetPipeline());
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

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTButterfly), VKW::FORMAT_R32G32B32A32_FLOAT, stagesCount, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ);

    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTPingPong0), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_RW);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTPingPong1), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_RW);
}

void FFTWaterFFTPass::Initialize(RenderGraph& graph)
{
}

static VKW::ImageResourceView* fftOutput = nullptr;

void FFTWaterFFTPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(FFTWaterFFT);

    Texture* fftHxt = graph.GetTexture(RESOURCE_ID(TextureID::FFTHxt));
    Texture* fftButterfly = graph.GetTexture(RESOURCE_ID(TextureID::FFTButterfly));

    Texture* pingPong0 = graph.GetTexture(RESOURCE_ID(TextureID::FFTPingPong0));
    Texture* pingPong1 = graph.GetTexture(RESOURCE_ID(TextureID::FFTPingPong1));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, pingPong0->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftHxt->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    context.CmdCopyImageToImage(pingPong0->GetShaderView()->parentResource_, fftHxt->GetShaderView()->parentResource_);

    Texture* input = pingPong0;
    Texture* output = pingPong1;

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("fft_iter");

    std::uint32_t stagesCount = std::uint32_t(glm::log2(float(C_WATER_DIM)));

    context.CmdBindComputePipeline(pipelineEntry->GetPipeline());

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, fftButterfly->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);


    // horizontal
    {
        for (std::uint32_t i = 0; i < stagesCount * 2; i++)
        {
            auto uniformAllocation = g_GraphicsManager->GetUniformArena().AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), sizeof(glm::vec4), 256);
            
            UniformProxy uniform{ &context, uniformAllocation };
            float const isVertical = i >= stagesCount ? 1.0f : 0.0f;
            uniform.WriteMember140(glm::vec4{ isVertical, i % stagesCount, 0.0f, 0.0f });
            uniform.FlushWrites();
            
            ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);
            binder.AddStorageTexture(0, fftButterfly);
            binder.AddStorageTexture(1, input);
            binder.AddStorageTexture(2, output);
            binder.AddUniform(3, &uniform);
            binder.FlushDescriptorWrites();

            context.CmdBindComputeDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

            g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, input->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);
            g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, output->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

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
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::FFTPingPong0), VKW::FORMAT_R32G32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_WRITE);
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::WaterHeight), VKW::FORMAT_R32_FLOAT, C_WATER_DIM, C_WATER_DIM, VKW::RESOURCE_ACCESS_SHADER_READ);
}

void FFTInvPermutationPass::Initialize(RenderGraph& graph)
{
}

void FFTInvPermutationPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(FFTInvPermutation);

    Texture* input = graph.GetTexture(RESOURCE_ID(TextureID::FFTPingPong0));
    Texture* heightMap = graph.GetTexture(RESOURCE_ID(TextureID::WaterHeight));

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, input->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_READ, VKW::STAGE_COMPUTE);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, heightMap->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_WRITE, VKW::STAGE_COMPUTE);

    UniformProxy uniform = graph.AllocateUniform(GetID(), context, WATER_UNIFORM_SIZE);
    FillWaterUniform(uniform, *g_GraphicsManager->GetTextureBank().FindTexture("blue_noise_256"));
    uniform.FlushWrites();

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("fft_inv_perm");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);
    binder.AddStorageTexture(0, input);
    binder.AddStorageTexture(1, heightMap);
    binder.AddUniform(2, &uniform);
    binder.FlushDescriptorWrites();

    context.CmdBindComputeDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());

    std::uint32_t group_dims[2] = { 8, 8 };
    context.CmdBindComputePipeline(pipelineEntry->GetPipeline());
    context.CmdDispatch(C_WATER_DIM / group_dims[0], C_WATER_DIM / group_dims[1], 1);
}

}
