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


    void RegisterRenderTarget       (BasePass* pass, char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height, std::uint32_t binding);
    void RegisterDepthStencilTarget (BasePass* pass, char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height);
    void RegisterDepthOnlyTarget (BasePass* pass, char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height);

    void RegisterTexture            (BasePass* pass, char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding);
    void RegisterStandaloneTexture  (char const* id, VKW::Format format, std::uint32_t width, std::uint32_t height, VKW::ResourceAccess access);
    void RegisterTextureSlot        (BasePass* pass, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding);

    void RegisterStorageBuffer      (BasePass* pass, char const* id, std::uint32_t size, VKW::ResourceAccess access, VKW::Stages stage, std::uint32_t binding);
    void RegisterUniformBuffer      (BasePass* pass, VKW::Stages stage, std::uint32_t binding);

    void RegisterPushConstant       (BasePass* pass, std::uint32_t size, VKW::Stages stage);

    VKW::DescriptorSet              GetPassDescriptorSet(PassID pass, FrameID frameID);
    VKW::PipelineLayout*            GetPassPipelineLayout(PassID pass);

    std::uint32_t                   GetPassSetBinding();
    std::uint32_t                   GetUserSetBinding(PassID pass);

    Texture*                        GetTexture(char const* id);
    StorageBuffer*                  GetBuffer(char const* id);
    UniformProxy                    GetPassUniform(PassID pass, VKW::Context& context, std::uint32_t size);

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
    GraphDescriptorManager  m_DescriptorManager;


    DRE::InplaceVector<BasePass*, 20>  m_Passes;
};

}
