#include <gfx\pass\CausticPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\scene\Scene.hpp>

namespace GFX
{

PassID CausticPass::GetID() const
{
    return PassID::Caustic;
}

void CausticPass::Initialize(RenderGraph& graph)
{
}

void CausticPass::RegisterResources(RenderGraph& graph)
{
    graph.RegisterTexture(this, RESOURCE_ID(TextureID::CausticEnvMap), VKW::FORMAT_R16G16B16A16_FLOAT, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, VKW::RESOURCE_ACCESS_SHADER_SAMPLE);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::CausticMap),
        VKW::FORMAT_R8_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT,
        0);
}


void WaterCausticDelegate(RenderableObject& obj, VKW::Context& context, VKW::DescriptorManager& descriptorManager, UniformArena& arena, RenderView const& view, VKW::PipelineLayout const* layout)
{
    /*
    auto uniformRegion = arena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(),
        sizeof(glm::mat4) * 2 +
        sizeof(glm::uvec4),
        256);
    
    VKW::DescriptorSet set = obj.GetDescriptorSet(g_GraphicsManager->GetCurrentFrameID());
    VKW::DescriptorManager::WriteDesc write;
    write.AddUniform(uniformRegion.m_Buffer, uniformRegion.m_OffsetInBuffer, uniformRegion.m_Size, 0);
    descriptorManager.WriteDescriptorSet(set, write);
    
    glm::mat4 const model = obj.GetSceneNode()->GetGlobalMatrix();
    VKW::TextureDescriptorIndex const& normalIndex = obj.GetNormalTexture()->GetShaderGlobalDescriptor();

    UniformProxy proxy{ &context, uniformRegion };
    proxy.WriteMember140(model);
    proxy.WriteMember140(model);
    proxy.WriteMember140(glm::uvec4{ normalIndex.id_, 0, 0, 0 });
    */
}

void CausticPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(Caustic);

    Texture* causticAttachment = graph.GetTexture(RESOURCE_ID(TextureID::CausticMap));
    Texture* shadowAttachment = graph.GetTexture(RESOURCE_ID(TextureID::ShadowMap));
    Texture* envMapAttachment = graph.GetTexture(RESOURCE_ID(TextureID::CausticEnvMap));

    VKW::ImageResourceView* causticView = causticAttachment->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, causticAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, shadowAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_ALL_GRAPHICS);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, envMapAttachment->GetShaderView()->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_ALL_GRAPHICS);

    context.CmdBeginRendering(1, &causticView, nullptr, nullptr);
    context.CmdSetViewport(1, 0, 0, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT);
    context.CmdSetScissor(1, 0, 0, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT);

    PipelineEntry* pipelineEntry = g_GraphicsManager->GetPipelineDB().GetEntry("water_caustics");
    ResourceBinder binder = g_GraphicsManager->CreateResourceBinder(pipelineEntry, 0);
    {
        auto uniformBuffer = g_GraphicsManager->GetUniformArena().AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), 256, 256);
        UniformProxy uniformProxy{ &context, uniformBuffer };
        uniformProxy.WriteMember140(g_GraphicsManager->GetSunShadowRenderView().GetViewProjectionM());
        uniformProxy.WriteMember140(g_GraphicsManager->GetSunShadowRenderView().GetInvViewProjectionM());

        binder.AddUniform(0, &uniformProxy);
        binder.AddSampledTexture(1, shadowAttachment);
        binder.FlushDescriptorWrites();

        context.CmdBindGraphicsDescriptorSets(pipelineEntry->GetLayout(), binder.GetTargetSetID(), 1, &binder.GetDescriptorSet());
    }

    context.CmdBindGraphicsPipeline(pipelineEntry->GetPipeline());


    // 1. take all RenderableObject's in main scene
    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };

    batcher.Batch(context, g_GraphicsManager->GetMainRenderView(), pipelineEntry->GetLayout(), RenderableObject::LAYER_WATER, GFX::WaterCausticDelegate);

    auto& draws = batcher.GetDraws();
    for (std::uint32_t i = 0, size = draws.Size(); i < size; i++)
    {
        AtomDraw const& atom = draws[i];
        // ignore default water pipeline, use caustic projection pipeline instead
        //context.CmdBindGraphicsPipeline(atom.pipeline);
        //context.CmdBindGraphicsDescriptorSets(atom.pipeline->GetLayout(), startSet, 1, &atom.descriptorSet);
        //context.CmdBindGraphicsDescriptorSets(layout, startSet, 1, &atom.descriptorSet);
        //context.CmdBindVertexBuffer(atom.vertexBuffer, atom.vertexOffset);
        //context.CmdBindIndexBuffer(atom.indexBuffer, atom.indexOffset);
        //context.CmdDrawIndexed(atom.indexCount);
    }

    // 2. feed all RenderableObject's to DrawBatcher
    // 3. DrawBatcher produces atomic draw commands
    context.CmdEndRendering();
}

}
