#include <engine\scene\Scene.hpp>

#include <foundation\memory\Memory.hpp>
#include <gfx\GraphicsManager.hpp>
#include <engine\data\Material.hpp>


namespace WORLD
{

Scene* g_MainScene = nullptr;

Scene::Scene(DRE::DefaultAllocator* allocator)
    : m_SceneEntities{ allocator }
{
}

Entity& Scene::CreateRenderableEntity(VKW::Context& context, Entity::TransformData const& transform, Data::Geometry* geometry, Data::Material* material)
{
    GFX::RenderableObject* renderable = GFX::g_GraphicsManager->CreateRenderableObject(context, geometry, material);
    GFX::RenderableObject* renderableShadow = GFX::g_GraphicsManager->CreateShadowRenderableObject(context, geometry);

    GFX::RenderView& mainView = GFX::g_GraphicsManager->GetMainRenderView();
    GFX::RenderView& shadowView = GFX::g_GraphicsManager->GetSunShadowRenderView();

    mainView.AddObject(renderable);
    shadowView.AddObject(renderableShadow);

    Entity& entity = m_SceneEntities.EmplaceBack(transform, renderable, renderableShadow);
    entity.SetMaterial(material);
    entity.SetGeometry(geometry);

    return entity;
}

Scene::~Scene()
{
    for (std::uint32_t i = 0, size = m_SceneEntities.Size(); i < size; i++)
    {
        Entity& entity = m_SceneEntities[i];
        if (entity.GetRenderableObject() != nullptr)
            GFX::g_GraphicsManager->FreeRenderableObject(entity.GetRenderableObject());
    }
}

}

