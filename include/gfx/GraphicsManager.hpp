#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\system\Window.hpp>
#include <foundation\container\ObjectPool.hpp>
#include <foundation\container\InplaceHashTable.hpp>
#include <foundation\container\HashTable.hpp>

#include <vk_wrapper\Device.hpp>

#include <gfx\FrameID.hpp>
#include <gfx\buffer\TransientArena.hpp>
#include <gfx\buffer\PersistentStorage.hpp>
#include <gfx\texture\TextureBank.hpp>
#include <gfx\scheduling\RenderGraph.hpp>
#include <gfx\scheduling\DependencyManager.hpp>
#include <gfx\pipeline\PipelineDB.hpp>
#include <gfx\renderer\RenderableObject.hpp>
#include <gfx\view\RenderView.hpp>

#include <engine\data\Geometry.hpp>
#include <engine\data\Material.hpp>


namespace VKW
{
struct LoaderDesc;
}

namespace IO
{
class IOManager;
}

namespace Data
{
class Geometry;
}

namespace GFX
{

class GraphicsManager final
    : public NonCopyable
    , public NonMovable
{
public:
    GraphicsManager(HINSTANCE hInstance, Window* window, IO::IOManager* ioManager, bool debug = false);
    ~GraphicsManager();

    inline Window*                      GetMainWindow() { return m_MainWindow; }

    inline std::uint32_t                GetRenderingWidth() { return m_MainWindow->Width(); }
    inline std::uint32_t                GetRenderingHeight() { return m_MainWindow->Height(); }

    inline VKW::Device*                 GetMainDevice() { return &m_Device; }

    inline VKW::ImportTable*            GetVulkanTable() const { return m_Device.GetFuncTable(); }
    inline VKW::Swapchain*              GetSwapchain() const{ return m_Device.GetSwapchain(); }
    inline VKW::PresentationController* GetPresentationController() const { return m_Device.GetPresentationController(); }

    inline VKW::Queue*                  GetMainQueue() const { return m_Device.GetMainQueue(); }
    inline VKW::Queue*                  GetLoadingQueue() const { return m_Device.GetMainQueue(); }
    inline VKW::Queue*                  GetPresentationQueue() const { return m_Device.GetMainQueue(); }

    inline VKW::Context&                GetMainContext() { return m_MainContext; }

    inline std::uint64_t                GetCurrentGraphicsFrame() const { return m_GraphicsFrame; }
    inline FrameID                      GetCurrentFrameID() const { return FrameID{ std::uint8_t(m_GraphicsFrame % VKW::CONSTANTS::FRAMES_BUFFERING) }; }
    inline FrameID                      GetPrevFrameID() const { return FrameID{ std::uint8_t((m_GraphicsFrame - 1) % VKW::CONSTANTS::FRAMES_BUFFERING) }; }
    inline FrameID                      GetNextFrameID() const { return FrameID{ std::uint8_t((m_GraphicsFrame + 1) % VKW::CONSTANTS::FRAMES_BUFFERING) }; }

    inline UploadArena&                 GetUploadArena() { return m_UploadArena; }
    inline UniformArena&                GetUniformArena() { return m_UniformArena; }
    inline ReadbackArena&               GetReadbackArena() { return m_ReadbackArena; }
    inline TextureBank&                 GetTextureBank() { return m_TextureBank; }
    inline PipelineDB&                  GetPipelineDB() { return m_PipelineDB; }
    inline PersistentStorage&           GetPersistentStorage() { return m_PersistentStorage; }
    inline DependencyManager&           GetDependencyManager() { return m_DependencyManager; }
    inline RenderGraph&                 GetMainRenderGraph() { return m_RenderGraph; }

    inline RenderView&                  GetMainRenderView() { return m_MainView; }
    inline auto&                        GetRenderablePool() { return m_RenderableObjectPool; }


public:
    void    Initialize();
    void    RenderFrame(std::uint64_t frame, std::uint64_t deltaTimeUS);

    RenderableObject*                   CreateRenderableObject(VKW::Context& context, Data::Geometry* geometry, Data::Material* material);
    void                                FreeRenderableObject(RenderableObject* obj);

private:
    void            CreateAllPasses();

    struct GeometryGPU
    {
        VKW::BufferResource* vertexBuffer;
        VKW::BufferResource* indexBuffer;
    };
    using GeometryGPUMap = DRE::InplaceHashTable<Data::Geometry*, GeometryGPU>;
    GeometryGPU* LoadGPUGeometry(VKW::Context& context, Data::Geometry* geometry);

    void            PrepareGlobalData(VKW::Context& context, std::uint64_t deltaTimeUS);
    VKW::QueueExecutionPoint TransferToSwapchainAndPresent(StorageTexture& src);


private:
    Window*                     m_MainWindow;
    IO::IOManager*              m_IOManager;

    VKW::Device                 m_Device;

    VKW::Context                m_MainContext;

    std::uint64_t               m_GraphicsFrame;
    VKW::QueueExecutionPoint    m_FrameProcessingCompletePoint[VKW::CONSTANTS::FRAMES_BUFFERING];

    UploadArena                 m_UploadArena;
    UniformArena                m_UniformArena;
    ReadbackArena               m_ReadbackArena;

    TextureBank                 m_TextureBank;
    PipelineDB                  m_PipelineDB;


    VKW::BufferResource*        m_GlobalUniforms[VKW::CONSTANTS::FRAMES_BUFFERING];
    PersistentStorage           m_PersistentStorage;

    RenderGraph                 m_RenderGraph;
    DependencyManager           m_DependencyManager;


    RenderView                  m_MainView;

    using RenderablePool        = DRE::InplaceObjectAllocator<RenderableObject, 2048>;
    RenderablePool              m_RenderableObjectPool;

    using MaterialLayoutMap     = DRE::InplaceHashTable<Data::Material::RenderingProperties::MaterialType, VKW::DescriptorSetLayout>;
    MaterialLayoutMap           m_MaterialLayoutMap; // store signature for similar materials

    GeometryGPUMap              m_GeometryGPUMap;
};

extern GraphicsManager* g_GraphicsManager;

}

