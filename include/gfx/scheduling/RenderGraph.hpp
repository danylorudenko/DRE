#pragma once

#include <foundation\Container\InplaceVector.hpp>

#include <gfx\scheduling\GraphResourcesManager.hpp>
#include <gfx\buffer\UniformProxy.hpp>
#include <gfx\pass\PassID.hpp>

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

    void RegisterRenderTarget       (BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, DRE::U32 binding, GraphResourceFlags flags = GraphResourceFlags::NONE);
    void RegisterDepthStencilTarget (BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, GraphResourceFlags flags = GraphResourceFlags::NONE);
    void RegisterDepthOnlyTarget    (BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, GraphResourceFlags flags = GraphResourceFlags::NONE);

    void RegisterTexture            (BasePass* pass, char const* id, VKW::Format format, DRE::U32 width, DRE::U32 height, VKW::ResourceAccess access, GraphResourceFlags flags = GraphResourceFlags::NONE);

    void RegisterStorageBuffer      (BasePass* pass, char const* id, DRE::U32 size, VKW::ResourceAccess access, GraphResourceFlags flags = GraphResourceFlags::NONE);

    Texture*                        GetTexture(char const* id);
    StorageBuffer*                  GetBuffer(char const* id);

    Texture*                        GetTemporalTextureCurrent(char const* id);
    Texture*                        GetTemporalTextureHistory(char const* id);

    StorageBuffer*                  GetTemporalBufferCurrent(char const* id);
    StorageBuffer*                  GetTemporalBufferHistory(char const* id);

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


    DRE::InplaceVector<BasePass*, 64>  m_Passes;
};

}
