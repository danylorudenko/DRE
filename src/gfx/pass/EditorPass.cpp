#include <gfx\pass\EditorPass.hpp>

#include <gfx\GraphicsManager.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <editor\ViewportInputManager.hpp>

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
    float r = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_RADIUS;
    float l = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_LENGTH;

    vertexLower.pos = glm::vec3{ 0.0f, offset1 * r, -offset0 * r };
    vertexLower.color = glm::vec3{ 1.0f, 0.0f, 0.0f };
    vertexLower.normal = glm::vec3{ 0.0f, offset1, -offset0 };

    vertexUpper.pos = glm::vec3{ l, offset1 * r, -offset0 * r };
    vertexUpper.color = glm::vec3{ 1.0f, 0.0f, 0.0f };
    vertexUpper.normal = glm::vec3{ 0.0f, offset1, -offset0 };
}

template<> // Y
void AssignCylinderVertex<1>(GizmoVertex& vertexLower, GizmoVertex& vertexUpper, float offset0, float offset1)
{
    float r = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_RADIUS;
    float l = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_LENGTH;

    vertexLower.pos = glm::vec3{ offset0 * r, 0.0f, offset1 * r };
    vertexLower.color = glm::vec3{ 0.0f, 1.0f, 0.0f };
    vertexLower.normal = glm::vec3{ offset0, 0.0f, offset1 };

    vertexUpper.pos = glm::vec3{ offset0 * r, l, offset1 * r };
    vertexUpper.color = glm::vec3{ 0.0f, 1.0f, 0.0f };
    vertexUpper.normal = glm::vec3{ offset0, 0.0f, offset1 };
}

template<> // Z
void AssignCylinderVertex<2>(GizmoVertex& vertexLower, GizmoVertex& vertexUpper, float offset0, float offset1)
{
    float r = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_RADIUS;
    float l = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_LENGTH;

    vertexLower.pos = glm::vec3{ offset0 * r, -offset1 * r, 0.0f };
    vertexLower.color = glm::vec3{ 0.0f, 0.0f, 1.0f };
    vertexLower.normal = glm::vec3{ offset0, -offset1, 0.0f };

    vertexUpper.pos = glm::vec3{ offset0 * r, -offset1 * r, l };
    vertexUpper.color = glm::vec3{ 0.0f, 0.0f, 1.0f };
    vertexUpper.normal = glm::vec3{ offset0, -offset1, 0.0f };
}

template<std::uint32_t AXIS>
void GenerateCyllinder(DRE::Vector<GizmoVertex, DRE::DefaultAllocator>& vertices, std::uint32_t sections)
{
    for (std::uint32_t i = 0; i < sections; i++)
    {
        float const angle = (PI * 2) / (sections - 1) * i;
        float const nextAngle = (PI * 2) / (sections - 1) * (i + 1);

        glm::vec2 offset{ glm::cos(angle), glm::sin(angle) };
        glm::vec2 nextOffset{ glm::cos(nextAngle), glm::sin(nextAngle) };

        GizmoVertex quad[4];
        AssignCylinderVertex<AXIS>(quad[0], quad[2], offset.x, offset.y);
        AssignCylinderVertex<AXIS>(quad[1], quad[3], nextOffset.x, nextOffset.y);

        vertices.EmplaceBack(quad[0]);
        vertices.EmplaceBack(quad[2]);
        vertices.EmplaceBack(quad[1]);

        vertices.EmplaceBack(quad[1]);
        vertices.EmplaceBack(quad[2]);
        vertices.EmplaceBack(quad[3]);
    }
}

template<std::uint32_t AXIS>
void FillConeSectionVertices(DRE::Vector<GizmoVertex, DRE::DefaultAllocator>& vertices, float angle, float nextAngle);

template<> // X
void FillConeSectionVertices<0>(DRE::Vector<GizmoVertex, DRE::DefaultAllocator>& vertices, float angle, float nextAngle)
{
    float constexpr len = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_LENGTH;

    float start = len;
    float end = len + len * 0.3f;

    float h = end - start;
    float w = 2.0f * WORLD::SceneNodeManipulator::GIZMO_CYLINDER_RADIUS;

    float theta = glm::atan(h / w);
    float alpha = PI / 2 - theta;

    float x = w * glm::sin(theta) * glm::sin(alpha) * glm::cos(angle);
    float y = w * glm::sin(theta) * glm::cos(alpha);
    float z = glm::sin(angle);

    glm::vec3 normal = glm::normalize(glm::vec3{ y, z, -x });

    glm::vec2 offset0{ glm::cos(angle), glm::sin(angle) };
    glm::vec2 offset1{ glm::cos(nextAngle), glm::sin(nextAngle) };

    GizmoVertex section[2];
    section[0].pos = glm::vec3{ start, offset0.y * w, -offset0.x * w };
    section[0].color = glm::vec3{ 1.0f, 0.0f, 0.0f };
    section[1].pos = glm::vec3{ start, offset1.y * w, -offset1.x * w };
    section[1].color = glm::vec3{ 1.0f, 0.0f, 0.0f };

    GizmoVertex centers[2];
    centers[0].pos = glm::vec3{ start, 0.0f, 0.0f };
    centers[0].color = glm::vec3{ 1.0f, 0.0f, 0.0f };
    centers[1].pos = glm::vec3{ end, 0.0f, 0.0f };
    centers[1].color = glm::vec3{ 1.0f, 0.0f, 0.0f };

    vertices.EmplaceBack(section[1]).normal = normal;
    vertices.EmplaceBack(centers[1]).normal = normal;
    vertices.EmplaceBack(section[0]).normal = normal;

    glm::vec3 downNormal{ 0.0f, 1.0f, 0.0f };
    vertices.EmplaceBack(section[0]).normal = normal;
    vertices.EmplaceBack(centers[1]).normal = normal;
    vertices.EmplaceBack(section[1]).normal = normal;
}

template<> // Y
void FillConeSectionVertices<1>(DRE::Vector<GizmoVertex, DRE::DefaultAllocator>& vertices, float angle, float nextAngle)
{
    float constexpr len = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_LENGTH;

    float start = len;
    float end = len + len * 0.3f;

    float h = end - start;
    float w = 2.0f * WORLD::SceneNodeManipulator::GIZMO_CYLINDER_RADIUS;

    float theta = glm::atan(h / w);
    float alpha = PI / 2 - theta;

    float x = w * glm::sin(theta) * glm::sin(alpha) * glm::cos(angle);
    float y = w * glm::sin(theta) * glm::cos(alpha);
    float z = glm::sin(angle);

    glm::vec3 normal = glm::normalize(glm::vec3{ x, y, z });

    glm::vec2 offset0{ glm::cos(angle), glm::sin(angle) };
    glm::vec2 offset1{ glm::cos(nextAngle), glm::sin(nextAngle) };

    GizmoVertex section[2];
    section[0].pos = glm::vec3{ offset0.x * w, start, offset0.y * w };
    section[0].color = glm::vec3{ 0.0f, 1.0f, 0.0f };
    section[1].pos = glm::vec3{ offset1.x * w, start, offset1.y * w };
    section[1].color = glm::vec3{ 0.0f, 1.0f, 0.0f };

    GizmoVertex centers[2];
    centers[0].pos = glm::vec3{ 0.0f, start, 0.0f };
    centers[0].color = glm::vec3{ 0.0f, 1.0f, 0.0f };
    centers[1].pos = glm::vec3{ 0.0f, end, 0.0f };
    centers[1].color = glm::vec3{ 0.0f, 1.0f, 0.0f };

    vertices.EmplaceBack(section[0]).normal = normal;
    vertices.EmplaceBack(centers[1]).normal = normal;
    vertices.EmplaceBack(section[1]).normal = normal;

    glm::vec3 downNormal{ 0.0f, 1.0f, 0.0f };
    vertices.EmplaceBack(section[1]).normal = normal;
    vertices.EmplaceBack(centers[1]).normal = normal;
    vertices.EmplaceBack(section[0]).normal = normal;
}

template<> // Z
void FillConeSectionVertices<2>(DRE::Vector<GizmoVertex, DRE::DefaultAllocator>& vertices, float angle, float nextAngle)
{
    float constexpr len = WORLD::SceneNodeManipulator::GIZMO_CYLINDER_LENGTH;

    float start = len;
    float end = len + len * 0.3f;

    float h = end - start;
    float w = 2.0f * WORLD::SceneNodeManipulator::GIZMO_CYLINDER_RADIUS;

    float theta = glm::atan(h / w);
    float alpha = PI / 2 - theta;

    float x = w * glm::sin(theta) * glm::sin(alpha) * glm::cos(angle);
    float y = w * glm::sin(theta) * glm::cos(alpha);
    float z = glm::sin(angle);

    glm::vec3 normal = glm::normalize(glm::vec3{ x, -z, y });

    glm::vec2 offset0{ glm::cos(angle), glm::sin(angle) };
    glm::vec2 offset1{ glm::cos(nextAngle), glm::sin(nextAngle) };

    GizmoVertex section[2];
    section[0].pos = glm::vec3{ offset0.x * w, -offset0.y * w, start };
    section[0].color = glm::vec3{ 0.0f, 0.0f, 1.0f };
    section[1].pos = glm::vec3{ offset1.x * w, -offset1.y * w, start };
    section[1].color = glm::vec3{ 0.0f, 0.0f, 1.0f };

    GizmoVertex centers[2];
    centers[0].pos = glm::vec3{ 0.0f, 0.0f, start };
    centers[0].color = glm::vec3{ 0.0f, 0.0f, 1.0f };
    centers[1].pos = glm::vec3{ 0.0f, 0.0f, end };
    centers[1].color = glm::vec3{ 0.0f, 0.0f, 1.0f };

    vertices.EmplaceBack(section[0]).normal = normal;
    vertices.EmplaceBack(centers[1]).normal = normal;
    vertices.EmplaceBack(section[1]).normal = normal;

    glm::vec3 downNormal{ 0.0f, 1.0f, 0.0f };
    vertices.EmplaceBack(section[1]).normal = normal;
    vertices.EmplaceBack(centers[1]).normal = normal;
    vertices.EmplaceBack(section[0]).normal = normal;
}

void Generate3Cones(DRE::Vector<GizmoVertex, DRE::DefaultAllocator>& vertices, std::uint32_t sections)
{
    for (std::uint32_t i = 0; i < sections; i++)
    {
        float const angle = (PI * 2) / (sections - 1) * i;
        float const nextAngle = (PI * 2) / (sections - 1) * (i + 1);

        FillConeSectionVertices<0>(vertices, angle, nextAngle);
        FillConeSectionVertices<1>(vertices, angle, nextAngle);
        FillConeSectionVertices<2>(vertices, angle, nextAngle);
    }
}

EditorPass::EditorPass(EDITOR::ViewportInputManager* viewportInput)
    : m_ViewportInput{ viewportInput }
{
}

void EditorPass::Initialize(RenderGraph& graph)
{
    std::uint32_t constexpr cyllinderResolution = 20;

    // cyllinder
    DRE::Vector<GizmoVertex, DRE::DefaultAllocator> vertices{ &DRE::g_MainAllocator };
    GenerateCyllinder<0>(vertices, 20);
    GenerateCyllinder<1>(vertices, 20);
    GenerateCyllinder<2>(vertices, 20);

    Generate3Cones(vertices, 20);

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

    context.CmdBeginRendering(1, &colorBuffer, nullptr, nullptr);

    if (m_ViewportInput->ShouldRenderTranslationGizmo())
    {
        glm::vec3 const focusedObjectPosition = m_ViewportInput->GetFocusedObjectPosition();
        float const gizmoScale = glm::length(g_GraphicsManager->GetMainRenderView().GetPosition() - focusedObjectPosition);

        glm::mat4 gizmoTransform {
            { gizmoScale, 0.0f, 0.0f, 0.0f },
            { 0.0f, gizmoScale, 0.0f, 0.0f },
            { 0.0f, 0.0f, gizmoScale, 0.0f },
            { focusedObjectPosition, 1.0f }
        };

        GizmoPassBuffer uniformData;
        uniformData.m_Model = gizmoTransform;

        UniformProxy uniform = graph.GetPassUniform(GetID(), context, sizeof(GizmoPassBuffer));
        uniform.WriteMember140(uniformData);


        VKW::PipelineLayout* layout = graph.GetPassPipelineLayout(GetID());
        VKW::Pipeline* pipeline = g_GraphicsManager->GetPipelineDB().GetPipeline("gizmo_3D");
        VKW::DescriptorSet set = graph.GetPassDescriptorSet(GetID(), g_GraphicsManager->GetCurrentFrameID());
        context.CmdBindGraphicsDescriptorSets(pipeline->GetLayout(), graph.GetPassSetBinding(), 1, &set);
        context.CmdBindVertexBuffer(m_GizmoVertices, 0);
        context.CmdBindGraphicsPipeline(pipeline);
        context.CmdDraw(m_GizmoGeometry->GetVertexCount());
    }
    context.CmdEndRendering();
}

}
