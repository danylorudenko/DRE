#include <engine\ApplicationContext.hpp>
#include <engine\data\GeometryLibrary.hpp>
#include <engine\data\MaterialLibrary.hpp>
#include <engine\scene\Scene.hpp>

#include <gfx\GraphicsManager.hpp>

namespace DRE
{

ApplicationContext g_AppContext;

WORLD::Entity* CreateDefaultBox(WORLD::Scene* targetScene)
{
    Data::Geometry* boxGeometry = g_AppContext.m_GeometryLibrary->GetGeometry(Data::GeometryLibrary::NAME_DEFAULT_BOX);
    Data::Material* diffuseMaterial = g_AppContext.m_MaterialLibrary->GetMaterial(Data::MaterialLibrary::NAME_DEFAULT_DIFFUSE_WHITE);

    WORLD::Entity* box = g_AppContext.m_MainScene->CreateOpaqueEntity(GFX::g_GraphicsManager->GetMainContext(), boxGeometry, diffuseMaterial);
    GFX::g_GraphicsManager->GetMainContext().FlushAll();

    return box;
}

WORLD::Entity* CreateDefaultSphere(WORLD::Scene* targetScene)
{
    Data::Geometry* sphereGeometry = g_AppContext.m_GeometryLibrary->GetGeometry(Data::GeometryLibrary::NAME_DEFAULT_SPHERE);
    Data::Material* diffuseMaterial = g_AppContext.m_MaterialLibrary->GetMaterial(Data::MaterialLibrary::NAME_DEFAULT_DIFFUSE_WHITE);

    WORLD::Entity* sphere = g_AppContext.m_MainScene->CreateOpaqueEntity(GFX::g_GraphicsManager->GetMainContext(), sphereGeometry, diffuseMaterial);
    GFX::g_GraphicsManager->GetMainContext().FlushAll();

    return sphere;
}

WORLD::Entity* CreateLocalLight(WORLD::Scene* targetScene)
{
    return nullptr;
}

}
