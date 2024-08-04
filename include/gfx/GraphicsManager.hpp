#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\system\Window.hpp>
#include <foundation\container\ObjectPool.hpp>
#include <foundation\container\InplaceHashTable.hpp>
#include <foundation\container\HashTable.hpp>
#include <foundation\container\Vector.hpp>

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
#include <gfx\renderer\LightsManager.hpp>

#include <engine\data\Geometry.hpp>
#include <engine\data\Material.hpp>

constexpr std::uint32_t C_SHADOW_MAP_WIDTH = 2048;
constexpr std::uint32_t C_SHADOW_MAP_HEIGHT = 2048;
constexpr float C_SHADOW_MAP_WORLD_EXTENT = 20.0f;
constexpr std::uint32_t C_WATER_DIM = 256;
constexpr std::uint32_t C_WATER_VERTEX_X = 100;
constexpr std::uint32_t C_WATER_VERTEX_Z = 200;

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

namespace WORLD
{
class Scene;
class SceneNode;
}

namespace GFX
{

struct GraphicsSettings
{
    bool            m_UseACESEncoding       = true;
    float           m_ExposureEV            = 0.0f;
    float           m_AlphaTAA              =0;//= 0.9f;
    float           m_VarianceGammaTAA      = 1.0f;
    float           m_JitterScale           =0;//= 0.15f;
    bool            m_WaterWireframe        = false;
    bool            m_UseFFTWater           = true;
    float           m_WaterSpeed            = 1.0f;
    float           m_WaterSizeMeters       = 10.0f;
    float           m_WaterAmplitude        = 1000.0f;
    float           m_WindDirectionX        = 0.0f;
    float           m_WindSpeed             = 1.0f;
    float           m_WindDirFactor         = 2.0f;
    float           m_GenericScalar         = 1.0f;

    std::uint32_t   m_ShadowMapWidth        = 1024;
    std::uint32_t   m_ShadowMapHeight       = 1024;

    std::uint32_t   m_RenderingWidth        = 0;
    std::uint32_t   m_RenderingHeight       = 0;
};

class GraphicsManager final
    : public NonCopyable
    , public NonMovable
{
public:
    using ImGuiSyncQueue = DRE::Vector<Texture*, DRE::DefaultAllocator>;

    GraphicsManager(HINSTANCE hInstance, Window* window, IO::IOManager* ioManager, bool debug = false);
    ~GraphicsManager();

    inline Window*                      GetMainWindow() { return m_MainWindow; }

    inline VKW::Device*                 GetMainDevice() { return &m_Device; }

    inline VKW::ImportTable*            GetVulkanTable() const { return m_Device.GetFuncTable(); }
    inline VKW::Instance*               GetInstance() const { return m_Device.GetInstance(); }
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
    inline LightsManager&               GetLightsManager() { return m_LightsManager; }
    inline DependencyManager&           GetDependencyManager() { return m_DependencyManager; }
    inline RenderGraph&                 GetMainRenderGraph() { return m_RenderGraph; }

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    inline ImGuiSyncQueue&              GetImGuiSyncQueue() { return m_ImGuiSyncQueue; }
#endif

    inline RenderView&                  GetMainRenderView() { return m_MainView; }
    inline RenderView&                  GetSunShadowRenderView() { return m_SunShadowView; }

    inline GraphicsSettings&            GetGraphicsSettings() { return m_Settings; }
    inline GraphicsSettings const&      GetGraphicsSettings() const { return m_Settings; }

    static constexpr VKW::Format        GetMainColorFormat() { return VKW::FORMAT_B8G8R8A8_UNORM; }
    static constexpr VKW::Format        GetFinalImageFormat() { return VKW::FORMAT_B8G8R8A8_UNORM; }
    static constexpr VKW::Format        GetMainDepthFormat() { return VKW::FORMAT_D32_FLOAT; }
    static constexpr VKW::Format        GetObjectIDBufferFormat() { return VKW::FORMAT_B8G8R8A8_UNORM; }


public:
    void                                LoadDefaultData();
    void                                ReloadShaders();
    void                                RenderFrame(std::uint64_t frame, std::uint64_t deltaTimeUS, float globalTimeS);
    void                                WaitIdle();

    RenderableObject*                   CreateRenderableObject(WORLD::SceneNode* sceneNode, VKW::Context& context, Data::Geometry* geometry, Data::Material* material);
    void                                FreeRenderableObject(RenderableObject* obj);

    struct GeometryGPU
    {
        VKW::BufferResource* vertexBuffer;
        VKW::BufferResource* indexBuffer;
    };
    GeometryGPU*                        LoadGPUGeometry(VKW::Context& context, Data::Geometry* geometry);

private:
    void                                CreateAllPasses();

    void            PrepareGlobalData(VKW::Context& context, WORLD::Scene& scene, std::uint64_t deltaTimeUS, float globalTimeS);
    VKW::QueueExecutionPoint TransferToSwapchainAndPresent(Texture& src);


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

#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    ImGuiSyncQueue              m_ImGuiSyncQueue;
#endif


    VKW::BufferResource*        m_GlobalUniforms[VKW::CONSTANTS::FRAMES_BUFFERING];
    PersistentStorage           m_PersistentStorage;

    LightsManager               m_LightsManager;

    RenderGraph                 m_RenderGraph;
    DependencyManager           m_DependencyManager;


    RenderView                  m_MainView;
    RenderView                  m_SunShadowView;

    using RenderablePool        = DRE::InplaceObjectAllocator<RenderableObject, 2048>;
    RenderablePool              m_RenderableObjectPool;

    using GeometryGPUMap        = DRE::InplaceHashTable<Data::Geometry*, GeometryGPU>;
    GeometryGPUMap              m_GeometryGPUMap;

    GraphicsSettings            m_Settings;
};

extern GraphicsManager* g_GraphicsManager;

}

