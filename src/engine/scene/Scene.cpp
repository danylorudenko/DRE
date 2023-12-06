#include <engine\scene\Scene.hpp>

#include <foundation\memory\Memory.hpp>
#include <gfx\GraphicsManager.hpp>
#include <engine\data\Material.hpp>


namespace WORLD
{

Scene* g_MainScene = nullptr;

Scene::Scene(DRE::DefaultAllocator* allocator)
    : m_SceneEntities{}
    , m_EntityCounter{ 0u }
    , m_Nodes{}
    , m_NodeCounter{ 0u }
    , m_RootNode{ nullptr }
{
    m_RootNode = CreateEmptySceneNode();

    SceneNode* cameraNode = CreateSceneNode(&m_MainCamera, m_RootNode);
    cameraNode->SetName("camera");

    SceneNode* sunNode = CreateSceneNode(&m_MainSunLight, cameraNode);
    sunNode->SetName("sun");
}

Entity* Scene::CreateOpaqueEntity(VKW::Context& context, Data::Geometry* geometry, Data::Material* material, SceneNode* parent)
{
    GFX::RenderableObject* renderable = GFX::g_GraphicsManager->CreateRenderableObject(context, geometry, material);

    GFX::RenderView& mainView = GFX::g_GraphicsManager->GetMainRenderView();
    GFX::RenderView& shadowView = GFX::g_GraphicsManager->GetSunShadowRenderView();

    mainView.AddObject(renderable);
    shadowView.AddObject(renderable);

    Entity* entity = CreateEntity(renderable);
    entity->SetMaterial(material);
    entity->SetGeometry(geometry);

    SceneNode* node = CreateSceneNode(entity, parent == nullptr ? m_RootNode : parent);

    return entity;
}

SceneNode* Scene::CreateSceneNode(ISceneNodeUser* user, SceneNode* parent)
{
    NodeID const id = m_NodeCounter++; 
    SceneNode* sceneNode = &m_Nodes.Emplace(id, parent, user);
    user->SetSceneNode(sceneNode);

    parent->AddChild(sceneNode);

    return sceneNode;
}

Scene::~Scene()
{
    m_SceneEntities.ForEach([](auto& pair)
    {
        Entity* entity = pair.value;;
        if (entity->GetRenderableObject() != nullptr)
            GFX::g_GraphicsManager->FreeRenderableObject(entity->GetRenderableObject());
    });
}

}

