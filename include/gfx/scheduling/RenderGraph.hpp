#pragma once

#include <foundation\Container\InplaceVector.hpp>

#include <vk_wrapper\descriptor\Descriptor.hpp>

#include <gfx\scheduling\GraphResourcesManager.hpp>
#include <gfx\scheduling\GraphDescriptorManager.hpp>

namespace VKW
{
class Context;
}

namespace GFX
{

class BasePass;
class GraphicsManager;
class Texture;

class RenderGraph
    : public NonMovable
    , public NonCopyable
{
public:
    RenderGraph(GraphicsManager* graphicsManager);

    ~RenderGraph();

    template<typename TPass, typename... TArgs>
    void AddPass(TArgs&&... args)
    {
        m_Passes.EmplaceBack(new TPass{ std::forward<TArgs>(args)... });
    }

    void RegisterRenderTarget       (BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, DRE::U32 binding);
    void RegisterDepthStencilTarget (BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height);
    void RegisterDepthOnlyTarget    (BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height);

    void RegisterTexture            (BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, VKW::ResourceAccess access);

    void RegisterStorageBuffer      (BasePass* pass, char const* id, DRE::U32 size, VKW::ResourceAccess access);

    Texture*                        GetTexture(char const* id);
    StorageBuffer*                  GetBuffer(char const* id);
    UniformProxy                    AllocateUniform(PassID pass, VKW::Context& context, DRE::U32 size);

    GraphResourcesManager&          GetResourcesManager();

public:
    void ParseGraph();
    void InitGraphResources();
    void UnloadGraphResources();

    // last access to texture should be VKW::RESOURCE_ACCESS_COLOR_ATTACHMENT
    Texture& Render(VKW::Context& context);


private:
    GraphicsManager*        m_GraphicsManager;
    GraphResourcesManager   m_ResourcesManager;


    DRE::InplaceVector<BasePass*, 20>  m_Passes;
};

}
