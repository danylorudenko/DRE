#include <gfx\pass\EditorPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>

#include <gizmo_3D.h>

namespace GFX
{

struct GizmoVertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
};

PassID EditorPass::GetID() const
{
    return PassID::Editor;
}

template<std::uint32_t AXIS>
void AssignCylinderVertex(GizmoVertex& vertexLower, GizmoVertex& vertexUpper, float offset0, float offset1);

template<> // X
void AssignCylinderVertex<0>(GizmoVertex& vertexLower, GizmoVertex& vertexUpper, float offset0, float offset1)
{
    vertexLower.pos = glm::vec3{ 0.0f, offset1, offset0 };
    vertexLower.color = glm::vec3{ 1.0f, 0.0f, 0.0f };
    vertexLower.normal = glm::vec3{ 0.0f, offset1, offset0 };

    vertexUpper.pos = glm::vec3{ 10.0f, offset1, offset0 };
    vertexUpper.color = glm::vec3{ 1.0f, 0.0f, 0.0f };
    vertexUpper.normal = glm::vec3{ 10.0f, offset1, offset0 };
}

template<> // Y
void AssignCylinderVertex<1>(GizmoVertex& vertexLower, GizmoVertex& vertexUpper, float offset0, float offset1)
{
    vertexLower.pos = glm::vec3{ offset0, 0.0f, offset1 };
    vertexLower.color = glm::vec3{ 0.0f, 1.0f, 0.0f };
    vertexLower.normal = glm::vec3{ offset0, 0.0f, offset1 };

    vertexUpper.pos = glm::vec3{ offset0, 10.0f, offset1 };
    vertexUpper.color = glm::vec3{ 0.0f, 1.0f, 0.0f };
    vertexUpper.normal = glm::vec3{ offset0, 10.0f, offset1 };
}

template<> // Z
void AssignCylinderVertex<2>(GizmoVertex& vertexLower, GizmoVertex& vertexUpper, float offset0, float offset1)
{
    vertexLower.pos = glm::vec3{ offset0, offset1, 0.0f };
    vertexLower.color = glm::vec3{ 0.0f, 0.0f, 1.0f };
    vertexLower.normal = glm::vec3{ offset0, offset1, 0.0f };

    vertexUpper.pos = glm::vec3{ offset0, offset1, 10.0f };
    vertexUpper.color = glm::vec3{ 0.0f, 0.0f, 1.0f };
    vertexUpper.normal = glm::vec3{ offset0, offset1, 10.0f };
}

template<std::uint32_t AXIS>
void GenerateCyllinder(DRE::Vector<GizmoVertex, DRE::DefaultAllocator>& vertices, std::uint32_t sections)
{
    for (std::uint32_t i = 0; i < sections; i++)
    {
        float const angle = (PI * 2) / (sections - 1) * i;
        float const nextAngle = (PI * 2) / (sections - 1) * (i + 1);

        glm::vec3 offset{ glm::cos(angle), 0.0f, glm::sin(angle) };
        glm::vec3 nextOffset{ glm::cos(nextAngle), 0.0f, glm::sin(nextAngle) };

        GizmoVertex quad[4];
        AssignCylinderVertex<AXIS>(quad[0], quad[2], offset.x, offset.z);
        AssignCylinderVertex<AXIS>(quad[1], quad[3], nextOffset.x, nextOffset.z);

        vertices.EmplaceBack(quad[0]);
        vertices.EmplaceBack(quad[1]);
        vertices.EmplaceBack(quad[2]);

        vertices.EmplaceBack(quad[1]);
        vertices.EmplaceBack(quad[3]);
        vertices.EmplaceBack(quad[2]);
    }
}

void EditorPass::Initialize(RenderGraph& graph)
{
    std::uint32_t constexpr cyllinderResolution = 20;

    // cyllinder
    DRE::Vector<GizmoVertex, DRE::DefaultAllocator> vertices{ &DRE::g_MainAllocator };
    GenerateCyllinder<0>(vertices, 20);
    GenerateCyllinder<1>(vertices, 20);
    GenerateCyllinder<2>(vertices, 20);

    m_GizmoGeometry = DRE::g_PersistentDataAllocator.Alloc<Data::Geometry>(std::uint16_t(sizeof(GizmoVertex)), std::uint16_t(0));
    m_GizmoGeometry->SetVertexData(DRE::ByteBuffer{ vertices.Data(), vertices.SizeInBytes() });

    GraphicsManager::GeometryGPU* gpuGeometry = g_GraphicsManager->LoadGPUGeometry(g_GraphicsManager->GetMainContext(), m_GizmoGeometry);
    m_GizmoVertices = gpuGeometry->vertexBuffer;
}

void EditorPass::RegisterResources(RenderGraph& graph)
{
    std::uint32_t renderWidth = g_GraphicsManager->GetGraphicsSettings().m_RenderingWidth, renderHeight = g_GraphicsManager->GetGraphicsSettings().m_RenderingHeight;

    graph.RegisterRenderTarget(this, RESOURCE_ID(TextureID::DisplayEncodedImage),
        g_GraphicsManager->GetFinalImageFormat(), renderWidth, renderHeight,
        0);

    graph.RegisterUniformBuffer(this, VKW::STAGE_VERTEX | VKW::STAGE_FRAGMENT, 0);
}

void EditorPass::Render(RenderGraph& graph, VKW::Context& context)
{
    DRE_GPU_SCOPE(EditorPass);

    VKW::ImageResourceView* colorBuffer = graph.GetTexture(RESOURCE_ID(TextureID::DisplayEncodedImage))->GetShaderView();
    VKW::ImageResourceView* objectIDBuffer = graph.GetTexture(RESOURCE_ID(TextureID::ObjectIDBuffer))->GetShaderView();

    g_GraphicsManager->GetDependencyManager().ResourceBarrier(context, colorBuffer->parentResource_, VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT, VKW::STAGE_COLOR_OUTPUT);

    VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());

    {
        GizmoPassBuffer uniformData;
        uniformData.m_Model = glm::identity<glm::mat4>();
        uniformData.m_CameraDistance = glm::vec4{ glm::length(g_GraphicsManager->GetMainRenderView().GetPosition()), 0.0f, 0.0f, 0.0f }; // TEMPORARY, NO OBJECT IN FOCUS

        UniformProxy uniform = graph.GetPassUniform(GetID(), context, sizeof(GizmoPassBuffer));
        uniform.WriteMember140(uniformData);
    }

    VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
    VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gizmo_3D");

    context.CmdBeginRendering(1, &colorBuffer, nullptr, nullptr);

    context.CmdBindGraphicsDescriptorSets(pipeline->GetLayout(), graph.GetPassSetBinding(), 1, &set);
    context.CmdBindVertexBuffer(m_GizmoVertices, 0);
    context.CmdBindGraphicsPipeline(pipeline);
    context.CmdDraw(m_GizmoGeometry->GetVertexCount());

    context.CmdEndRendering();
}

}
