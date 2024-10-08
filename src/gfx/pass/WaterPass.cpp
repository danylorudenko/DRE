#include <gfx\pass\WaterPass.hpp>

#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\renderer\DrawBatcher.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>

namespace GFX
{

PassID GFX::WaterPass::GetID() const
{
    return PassID::Water;
}

void WaterPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::ShadowMap),
        VKW::FORMAT_D16_UNORM, C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT, 0);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::ForwardColor),
        g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT, 1);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT, 2);

    graph.RegisterTexture(this,
        RESOURCE_ID(TextureID::WaterHeight),
        VKW::FORMAT_R32_FLOAT, C_WATER_DIM, C_WATER_DIM,
        VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_VERTEX, 3);

    graph.RegisterUniformBuffer(this, VKW::STAGE_FRAGMENT, 4);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::WaterColor),
        g_GraphicsManager->GetMainColorFormat(), renderWidth, renderHeight,
        0);

    graph.RegisterRenderTarget(this,
        RESOURCE_ID(TextureID::Velocity),
        VKW::FORMAT_R16G16_FLOAT, renderWidth, renderHeight,
        1);

    graph.RegisterDepthOnlyTarget(this,
        RESOURCE_ID(TextureID::MainDepth),
        g_GraphicsManager->GetMainDepthFormat(), renderWidth, renderHeight);
}

void WaterPass::Initialize(RenderGraph& graph)
{
}

void WaterObjectDelegate(RenderableObject& obj, VKW::Context& context, VKW::DescriptorManager& descriptorManager, UniformArena& arena, RenderView const& view)
{
    //std::uint32_t constexpr uniformSize =
    //    sizeof(glm::mat4) * 2 + sizeof(std::uint32_t) * 4;
    //
    //auto uniformAllocation = arena.AllocateTransientRegion(g_GraphicsManager->GetCurrentFrameID(), uniformSize, 256);
    //VKW::DescriptorManager::WriteDesc writeDesc;
    //writeDesc.AddUniform(uniformAllocation.m_Buffer, uniformAllocation.m_OffsetInBuffer, uniformAllocation.m_Size, 0);
    //descriptorManager.WriteDescriptorSet(obj.GetDescriptorSet(g_GraphicsManager->GetCurrentFrameID()), writeDesc);
    //
    //VKW::TextureDescriptorIndex const& normalIndex = obj.GetNormalTexture()->GetShaderGlobalReadDescriptor();
    //
    //UniformProxy uniformProxy{ &context, uniformAllocation };
    //uniformProxy.WriteMember140(obj.GetModelM());
    //uniformProxy.WriteMember140(obj.GetModelM()); // prev world matrix is same, geometry is static
    //uniformProxy.WriteMember140(normalIndex.id_);

}

void WaterPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(Water);

    VKW::ImageResourceView* waterAttachment = graph.GetTexture(RESOURCE_ID(TextureID::WaterColor))->GetShaderView();
    VKW::ImageResourceView* velocityAttachment = graph.GetTexture(RESOURCE_ID(TextureID::Velocity))->GetShaderView();
    VKW::ImageResourceView* depthAttachment = graph.GetTexture(RESOURCE_ID(TextureID::MainDepth))->GetShaderView();
    VKW::ImageResourceView* shadowMap       = graph.GetTexture(RESOURCE_ID(TextureID::ShadowMap))->GetShaderView();
    VKW::ImageResourceView* heightMap       = graph.GetTexture(RESOURCE_ID(TextureID::WaterHeight))->GetShaderView();
    VKW::ImageResourceView* color           = graph.GetTexture(RESOURCE_ID(TextureID::ForwardColor))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, waterAttachment->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, color->parentResource_, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    context.CmdCopyImageToImage(waterAttachment->parentResource_, color->parentResource_);

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, waterAttachment->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, velocityAttachment->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, depthAttachment->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_ALL_GRAPHICS); // but also used as depth readonly attachment
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, shadowMap->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, heightMap->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_VERTEX);
    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, color->parentResource_, VKW::RESOURCE_ACCESS_SHADER_SAMPLE, VKW::STAGE_FRAGMENT);

    VKW::ImageResourceView* attachments[2] = { waterAttachment, velocityAttachment };

    DrawBatcher batcher{ &DRE::g_FrameScratchAllocator, g_GraphicsManager->GetMainDevice()->GetDescriptorManager(), &g_GraphicsManager->GetUniformArena() };
    batcher.Batch(context, g_GraphicsManager->GetMainRenderView(), RenderableObject::LAYER_WATER_BIT, WaterObjectDelegate);

    context.CmdBeginRendering(2, attachments, depthAttachment, nullptr);

    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;
    context.CmdSetViewport(2, 0, 0, renderWidth, renderHeight);
    context.CmdSetScissor(2, 0, 0, renderWidth, renderHeight);
#ifndef DRE_COMPILE_FOR_RENDERDOC
    context.CmdSetPolygonMode(g_GraphicsManager->GetGraphicsSettings().m_WaterWireframe ? VKW::POLYGON_WIREFRAME : VKW::POLYGON_FILL);
#endif // DRE_COMPILE_FOR_RENDERDOC

    {
        glm::mat4 const shadow_ViewProj = g_GraphicsManager->GetSunShadowRenderView().GetViewProjectionM();
        glm::vec4 const shadow_Size = glm::vec4{ C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, 0.0f, 0.0f };
        glm::vec4 const useFFT = glm::vec4{ g_GraphicsManager->GetGraphicsSettings().m_UseFFTWater ? 1.0f : 0.0f, C_WATER_VERTEX_X, C_WATER_VERTEX_Z, g_GraphicsManager->GetGraphicsSettings().m_WindDirectionX };

        std::uint32_t constexpr passUniformSize = sizeof(shadow_ViewProj) + sizeof(shadow_Size) + sizeof(useFFT);

        UniformProxy passUniformProxy = graph.GetPassUniform(GetID(), context, passUniformSize);
        passUniformProxy.WriteMember140(shadow_ViewProj);
        passUniformProxy.WriteMember140(shadow_Size);
        passUniformProxy.WriteMember140(useFFT);
    }

    VKW::DescriptorSet passSet = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
    std::uint32_t const passSetBinding = g_GraphicsManager->GetMainDevice()->GetDescriptorManager()->GetGlobalSetLayoutsCount();

    VKW::PipelineLayout* passLayout = graph.GetPassPipelineLayout(GetID());
    context.CmdBindGraphicsDescriptorSets(passLayout, passSetBinding, 1, &passSet);

    std::uint32_t const startSet = graph.GetUserSetBinding(GetID());

    auto& draws = batcher.GetDraws();
    for (std::uint32_t i = 0, size = draws.Size(); i < size; i++)
    {
        AtomDraw const& atom = draws[i];
        context.CmdBindGraphicsPipeline(atom.pipeline);
        context.CmdBindGraphicsDescriptorSets(atom.pipeline->GetLayout(), startSet, 1, &atom.descriptorSet);
        context.CmdBindVertexBuffer(atom.vertexBuffer, atom.vertexOffset);
        context.CmdBindIndexBuffer(atom.indexBuffer, atom.indexOffset);
        context.CmdDrawIndexed(atom.indexCount);
    }

    context.CmdEndRendering();
}

}
