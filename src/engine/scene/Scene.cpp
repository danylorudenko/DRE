#include <engine\scene\Scene.hpp>

#include <foundation\memory\Memory.hpp>
#include <gfx\GraphicsManager.hpp>


namespace WORLD
{

Scene* g_MainScene = nullptr;

Scene::Scene()
    : m_SceneEntities{ &DRE::g_MainAllocator }
{
    GFX::PipelineDB& db = GFX::g_GraphicsManager->GetPipelineDB();

    //db.

}

Entity& Scene::CreateEntity(Data::Material* material)
{
    // I guess here I have to tie Entity, RenderableObject and RenderView
    GFX::RenderView& mainView = GFX::g_GraphicsManager->GetMainRenderView();

    // form material "pipeline layout" (I guess we'll have a default one)
    // get default pipeline to use (there also will be a default one)
    // fill descriptors for the material
    // pass it all to renderable object

    //GFX::g_GraphicsManager->GetPipelineDB().
    GFX::RenderableObject* renderable = GFX::g_GraphicsManager->GetRenderablePool().Alloc(nullptr); // need pipeline here
    mainView.AddObject(renderable);


    return m_SceneEntities.EmplaceBack(glm::vec3{0,0,0}, glm::vec3{0,0,0}, glm::vec3{1,1,1}, renderable);
}

}

