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
    m_RootNode = CreateRootSceneNode();

    SceneNode* cameraNode = CreateSceneNode(&m_MainCamera, m_RootNode);
    cameraNode->SetName("camera");
}

Entity* Scene::CreateOpaqueEntity(VKW::Context& context, Data::Geometry* geometry, Data::Material* material, SceneNode* parent)
{
    Entity* entity = CreateEntity();
    entity->SetMaterial(material);
    entity->SetGeometry(geometry);

    SceneNode* node = CreateSceneNode(entity, parent == nullptr ? m_RootNode : parent);

    GFX::RenderableObject* renderable = GFX::g_GraphicsManager->CreateRenderableObject(node, context, geometry, material);
    entity->SetRenderableObject(renderable);

    GFX::RenderView& mainView = GFX::g_GraphicsManager->GetMainRenderView();
    GFX::RenderView& shadowView = GFX::g_GraphicsManager->GetSunShadowRenderView();

    mainView.AddObject(renderable);
    shadowView.AddObject(renderable);

    return entity;
}

SceneNode* Scene::CreateSceneNode(ISceneNodeUser* user, SceneNode* parent)
{
    NodeID const id = m_NodeCounter++; 
    SceneNode* sceneNode = &m_Nodes.Emplace(id, parent, user);

    if (user != nullptr)
        user->SetSceneNode(sceneNode);

    if (parent != nullptr)
        parent->AddChild(sceneNode);
    else
        m_RootNode->AddChild(sceneNode);

    return sceneNode;
}

Light* Scene::CreateSunLight(VKW::Context& context, SceneNode* parent)
{
    return CreateDirectionalLightInternal(context, parent, DRE_LIGHT_TYPE_SUN);
}

Light* Scene::CreateDirectionalLight(VKW::Context& context, SceneNode* parent)
{
    return CreateDirectionalLightInternal(context, parent, DRE_LIGHT_TYPE_DIRECTIONAL);
}

Light* Scene::CreateDirectionalLightInternal(VKW::Context& context, SceneNode* parent, std::uint32_t type)
{
    LightID const id = m_LightsCounter++;
    Light* light = &m_SceneLights.Emplace(id, &GFX::g_GraphicsManager->GetLightsManager(), static_cast<std::uint32_t>(type));

    SceneNode* node = CreateSceneNode(light, parent == nullptr ? m_RootNode : parent);
    node->SetName("Directional Light");
    light->ScheduleUpdateGPUData();

    return light;
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

