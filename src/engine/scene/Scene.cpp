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
    // I guess here I have to tie Entity, RenderableObject and RenderView
    //GFX::RenderView& mainView = GFX::g_GraphicsManager->GetMainRenderView();

    // form material "pipeline layout" (I guess we'll have a default one)
    // get default pipeline to use (there also will be a default one)
    // fill descriptors for the material
    // pass it all to renderable object

    GFX::RenderableObject* renderable = GFX::g_GraphicsManager->CreateRenderableObject(context, geometry, material);
    renderable->Transform(transform.model);
    //mainView.AddObject(renderable);

    Entity& entity = m_SceneEntities.EmplaceBack(transform, renderable);
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

