#include <gfx\GraphicsManager.hpp>

#include <foundation\math\Geometry.hpp>
#include <foundation\system\Window.hpp>
#include <foundation\input\InputSystem.hpp>

#include <vk_wrapper\Device.hpp>

#include <gfx\pass\GBufferPass.hpp>
#include <gfx\pass\LightingPass.hpp>
#include <gfx\pass\WaterPass.hpp>
#include <gfx\pass\FFTWaterPass.hpp>
#include <gfx\pass\AntiAliasingPass.hpp>
#include <gfx\pass\AmbientOcclusionPass.hpp>
#include <gfx\pass\ColorEncodingPass.hpp>
#include <gfx\pass\ImGuiRenderPass.hpp>
#include <gfx\pass\DebugPassTextureView.hpp>
#include <gfx\pass\EditorPass.hpp>
#include <gfx\pass\DDGI.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>
#include <engine\data\GeometryLibrary.hpp>

#include <common\global_uniform.slang>


namespace GFX
{

static constexpr std::uint32_t C_STAGING_ARENA_SIZE         = 1024 * 1024 * 256;
static constexpr std::uint32_t C_UNIFORM_ARENA_SIZE         = DRE_U16_MAX * 2 - 1;
static constexpr std::uint32_t C_READBACK_ARENA_SIZE        = 1024 * 1024 * 64;
static constexpr std::uint32_t C_PERSISTENT_STORAGE_SIZE    = 1024 * 1024 * 16;

GraphicsManager* g_GraphicsManager = nullptr;

GraphicsManager::GraphicsManager(HINSTANCE hInstance, SYS::Window* window, IO::IOManager* ioManager, IO::ShaderDB* shaderDB, bool debug)
    : m_MainWindow{ window }
    , m_Device{ hInstance, window->NativeHandle(), debug }
    , m_MainContext{ m_Device.GetFuncTable(), m_Device.GetMainQueue(), &DRE::g_FrameScratchAllocator }
    , m_GraphicsFrame{ 0 }
    , m_RenderGraph{ this }
    , m_DependencyManager{}
    , m_UploadArena{ &m_Device, C_STAGING_ARENA_SIZE }
    , m_UniformArena{ &m_Device, C_UNIFORM_ARENA_SIZE }
    , m_ReadbackArena{ &m_Device, C_READBACK_ARENA_SIZE }
    , m_PersistentStorage{ &m_Device, &m_UploadArena, &m_Device, C_PERSISTENT_STORAGE_SIZE }
    , m_MaterialsManager{ &m_PersistentStorage }
    , m_InstanceDataManager{ &m_PersistentStorage }
    , m_LightsManager{ &m_PersistentStorage }
    , m_TextureBank{ &m_MainContext, m_Device.GetResourcesController(), m_Device.GetDescriptorManager() }
    , m_PipelineDB{ &m_Device, shaderDB }
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    , m_ImGuiSyncQueue{ &DRE::g_PersistentDataAllocator }
#endif
    , m_GlobalGeometryManager{ &m_MainContext, &m_Device, &m_UploadArena }
    , m_RayTracingManager{ &m_Device, &m_GlobalGeometryManager }
    , m_DDGI{ }
    , m_MainView{ &DRE::g_MainAllocator }
    , m_SunShadowView{ &DRE::g_MainAllocator }
    , m_Settings{}
{
    g_GraphicsManager = this;

    m_Settings.m_RenderingWidth = m_MainWindow->Width();
    m_Settings.m_RenderingHeight = m_MainWindow->Height();

    for (std::uint32_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
    {
        char name[16];
        std::sprintf(name, "g_uniform_%u", i);
        m_GlobalUniforms[i] = m_Device.GetResourcesController()->CreateBuffer(sizeof(GlobalUniforms), VKW::BufferUsage::UNIFORM, name);
    }
    m_Device.GetDescriptorManager()->AllocateDefaultDescriptors(VKW::CONSTANTS::FRAMES_BUFFERING, m_GlobalUniforms, m_PersistentStorage.GetStorage()->GetResource());
}

void GraphicsManager::PrecacheAllData(EDITOR::ViewportInputManager* viewportInput, Data::GeometryLibrary* geometryLibrary)
{
    m_PipelineDB.CreateDefaultPipelines();
    m_TextureBank.LoadDefaultTextures();
    m_DDGI.Initialize(geometryLibrary);
    CreateAllPasses(viewportInput, geometryLibrary);
}

void GraphicsManager::CreateAllPasses(EDITOR::ViewportInputManager* viewportInput, Data::GeometryLibrary* geometryLibrary)
{
    //m_RenderGraph.AddPass<ShadowPass>();
    //m_RenderGraph.AddPass<CausticPass>();
    m_RenderGraph.AddPass<GBufferPass>();

    m_RenderGraph.AddPass<DDGIProbeScatterPass>();
    m_RenderGraph.AddPass<DDGIProbeTracePass>();
    m_RenderGraph.AddPass<DDGIProbeLightingPass>();
    m_RenderGraph.AddPass<DDGIProbeBlendPass>();

    m_RenderGraph.AddPass<LightingPass>();

    m_RenderGraph.AddPass<FFTButterflyGenPass>();
    m_RenderGraph.AddPass<FFTWaterH0GenPass>();
    m_RenderGraph.AddPass<FFTWaterHxtGenPass>();
    m_RenderGraph.AddPass<FFTWaterFFTPass>();
    m_RenderGraph.AddPass<FFTInvPermutationPass>();
    m_RenderGraph.AddPass<WaterPass>();

    m_RenderGraph.AddPass<AntiAliasingPass>();
    m_RenderGraph.AddPass<AmbientOcclusionPass>();
    m_RenderGraph.AddPass<ColorEncodingPass>();

    m_RenderGraph.AddPass<EditorPass>(viewportInput);

    m_RenderGraph.AddPass<DebugPassDDGIProbeDisplay>();
    m_RenderGraph.AddPass<DebugPassTextureView>();

    m_RenderGraph.AddPass<ImGuiRenderPass>();

    m_RenderGraph.ParseGraph();
    m_RenderGraph.InitGraphResources();
}

glm::vec2 constexpr s_HaltonSequence[16] = {
    glm::vec2{ 0.500000, 0.333333 },
    glm::vec2{ 0.250000, 0.666667 },
    glm::vec2{ 0.750000, 0.111111 },
    glm::vec2{ 0.125000, 0.444444 },
    glm::vec2{ 0.625000, 0.777778 },
    glm::vec2{ 0.375000, 0.222222 },
    glm::vec2{ 0.875000, 0.555556 },
    glm::vec2{ 0.062500, 0.888889 },
    glm::vec2{ 0.562500, 0.037037 },
    glm::vec2{ 0.312500, 0.370370 },
    glm::vec2{ 0.812500, 0.703704 },
    glm::vec2{ 0.187500, 0.148148 },
    glm::vec2{ 0.687500, 0.481481 },
    glm::vec2{ 0.437500, 0.814815 },
    glm::vec2{ 0.937500, 0.259259 },
    glm::vec2{ 0.031250, 0.592593 }
};

void GraphicsManager::PrepareGlobalData(VKW::Context& context, WORLD::Scene& scene, std::uint64_t deltaTimeUS, float timeS)
{
    m_MainView.UpdatePreviosFrame();
    m_SunShadowView.UpdatePreviosFrame();

    VKW::BufferResource* buffer = m_GlobalUniforms[GetCurrentFrameID()];
    void* dst = buffer->memory_.GetRegionMappedPtr();

    glm::vec2 const halton = s_HaltonSequence[GetCurrentGraphicsFrame() % (sizeof(s_HaltonSequence) / sizeof(glm::vec2))];
    glm::vec2 const taaJitter = ((halton - 0.5f) / glm::vec2(m_MainView.GetSize())) * 2.0f * glm::vec2{ GetGraphicsSettings().m_JitterScale };


    WORLD::Camera const& camera = scene.GetMainCamera();
    m_MainView.UpdatePlacement(camera.GetPosition(), camera.GetForward(), camera.GetUp());
    m_MainView.UpdateViewport(glm::uvec2{ 0, 0 }, glm::uvec2{ m_Settings.m_RenderingWidth, m_Settings.m_RenderingHeight });
    m_MainView.UpdateProjection(camera.GetFOV(), camera.GetRange()[0], camera.GetRange()[1]);
    m_MainView.UpdateJitter(taaJitter.x, taaJitter.y);

    WORLD::Light const& sunLight = *scene.GetMainSunLight();
    m_SunShadowView.UpdatePlacement(sunLight.GetPosition(), sunLight.GetForward(), sunLight.GetUp());
    m_SunShadowView.UpdateViewport(glm::uvec2{ 0, 0 }, glm::uvec2{ C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT });
    m_SunShadowView.UpdateProjection(
        -C_SHADOW_MAP_WORLD_EXTENT, C_SHADOW_MAP_WORLD_EXTENT,
        -C_SHADOW_MAP_WORLD_EXTENT, C_SHADOW_MAP_WORLD_EXTENT,
        -C_SHADOW_MAP_WORLD_EXTENT, C_SHADOW_MAP_WORLD_EXTENT);


    GlobalUniforms globalUniform{};
    globalUniform.viewportSize_deltaMS_timeS[0] = static_cast<float>(m_Settings.m_RenderingWidth);
    globalUniform.viewportSize_deltaMS_timeS[1] = static_cast<float>(m_Settings.m_RenderingHeight);
    globalUniform.viewportSize_deltaMS_timeS[2] = static_cast<float>(static_cast<double>(deltaTimeUS) / 1000.0);
    globalUniform.viewportSize_deltaMS_timeS[3] = timeS;

    globalUniform.CameraPos_GenericScalar = glm::vec4{ scene.GetMainCamera().GetPosition(), GetGraphicsSettings().m_GenericScalar };
    globalUniform.CameraDir        = glm::vec3{ scene.GetMainCamera().GetForward() };
    globalUniform.FrameNumber      = DRE::U32(GetCurrentGraphicsFrame());
    globalUniform.Jitter           = glm::vec4{ taaJitter, 0.0f, 0.0f };

    globalUniform.ViewM            = m_MainView.GetViewM();
    globalUniform.iViewM           = m_MainView.GetInvViewM();
    globalUniform.ProjM            = m_MainView.GetProjectionM();
    globalUniform.iProjM           = m_MainView.GetInvProjectionM();
    globalUniform.ViewProjM        = m_MainView.GetViewProjectionM();
    globalUniform.iViewProjM       = m_MainView.GetInvViewProjectionM();

    globalUniform.PrevViewM        = m_MainView.GetPrevViewM();
    globalUniform.PreviViewM       = m_MainView.GetPrevInvViewM();
    globalUniform.PrevProjM        = m_MainView.GetPrevProjectionM();
    globalUniform.PreviProjM       = m_MainView.GetPrevInvProjectionM();
    globalUniform.PrevViewProjM    = m_MainView.GetPrevViewProjectionM();
    globalUniform.PreviViewProjM   = m_MainView.GetPrevInvViewProjectionM();

    globalUniform.blueNoiseTextureID    = m_TextureBank.FindTexture("blue_noise_256")->GetShaderGlobalDescriptor().id_;
    globalUniform.whiteNoiseTextureID   = m_TextureBank.FindTexture("white_noise_256")->GetShaderGlobalDescriptor().id_;

    globalUniform.SunLightDir      = glm::vec4{ sunLight.GetForward(), 0.0f };

    globalUniform.lightsCount           = glm::uvec4{ m_LightsManager.GetCount(), 0u, 0u, 0u};
    globalUniform.LightBuffer           = reinterpret_cast<S_LIGHT*>(m_LightsManager.GetBufferAddress());
    globalUniform.InstanceBuffer        = reinterpret_cast<S_INSTANCE*>(m_InstanceDataManager.GetBufferAddress());
    globalUniform.GlobalGeometryBuffer  = m_GlobalGeometryManager.GetMainBufferAddress();

    std::memcpy(dst, &globalUniform, sizeof(globalUniform));

    buffer->memory_.FlushCaches(g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice());

    context.CmdResourceDependency(buffer,
        VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST,
        VKW::RESOURCE_ACCESS_SHADER_UNIFORM, VKW::STAGE_VERTEX);
}

void GraphicsManager::BuildMainSceneTLAS()
{
    m_InstanceDataManager.FlushUpdates(GetMainContext());
    m_RayTracingManager.BuildSceneAccelerationStructure(m_MainView, GetMainContext());
    m_Device.GetDescriptorManager()->WriteTLASDescriptor(m_RayTracingManager.GetMainSceneTLAS()->m_LogicalHandle);
}

void GraphicsManager::RenderFrame(std::uint64_t frame, std::uint64_t deltaTimeUS, float globalTimeS)
{
    m_GraphicsFrame = frame;

    // need to wait for currentFrame - 2 to complete
    if (m_FrameProcessingCompletePoint[GetCurrentFrameID()].GetQueue() != nullptr)
        m_FrameProcessingCompletePoint[GetCurrentFrameID()].Wait();

    m_Device.GetDescriptorManager()->ResetPerFramePool(GetCurrentFrameID());

    m_UniformArena.ResetAllocations(GetCurrentFrameID());
    m_UploadArena.ResetAllocations(GetCurrentFrameID());
    m_ReadbackArena.ResetAllocations(GetCurrentFrameID());

    VKW::Context& context = GetMainContext();

    DRE_GPU_SCOPE(FRAME);

    context.ResetDependenciesVectors(&DRE::g_FrameScratchAllocator);
    PrepareGlobalData(context, *WORLD::g_MainScene, deltaTimeUS, globalTimeS);

    // maybe I should do these earlier?
    m_GlobalGeometryManager.UpdateGPUGeometry(context);
    m_InstanceDataManager.FlushUpdates(context);
    m_MaterialsManager.FlushUpdates(context);
    m_LightsManager.FlushUpdates(context);

#ifdef DRE_DEBUG
    if (SYS::g_InputSystem->GetKeyboardButtonJustPressed(Keys::B))
        DebugBreak();
#endif

    // globalData
    context.CmdBindGlobalDescriptorSets(*GetMainDevice()->GetDescriptorManager(), GetCurrentFrameID());

    // main graph
    Texture& finalRT = m_RenderGraph.Render(context);

    // presentation
    m_DependencyManager.ResourceBarrier(context, finalRT.GetResource(), VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    GetMainContext().FlushAll();

    VKW::QueueExecutionPoint srcTransferComplete = TransferToSwapchainAndPresent(finalRT);
    m_FrameProcessingCompletePoint[GetCurrentFrameID()] = srcTransferComplete;
}

VKW::QueueExecutionPoint GraphicsManager::TransferToSwapchainAndPresent(Texture& src)
{
    VKW::PresentationController* presentController = GetPresentationController();
    VKW::PresentationContext presentationContext = presentController->AcquireNewPresentContext();


    GetMainContext().CmdResourceDependency(presentController->GetSwapchainResource(presentationContext),
        VKW::RESOURCE_ACCESS_UNDEFINED, VKW::STAGE_PRESENT,
        VKW::RESOURCE_ACCESS_CLEAR,     VKW::STAGE_TRANSFER);

    GetMainContext().CmdCopyImageToImage(presentController->GetSwapchainResource(presentationContext), src.GetResource());

    GetMainContext().CmdResourceDependency(presentController->GetSwapchainResource(presentationContext),
        VKW::RESOURCE_ACCESS_CLEAR,   VKW::STAGE_TRANSFER,
        VKW::RESOURCE_ACCESS_PRESENT, VKW::STAGE_PRESENT);

    VKW::QueueExecutionPoint transferCompletePoint = GetMainContext().SyncPoint();

    GetMainContext().FlushWaitSwapchain(presentationContext);
    GetMainContext().Present(presentationContext);

    return transferCompletePoint;
}

RenderableObject* GraphicsManager::CreateRenderableObject(WORLD::SceneNode* sceneNode, VKW::Context& context, Data::Geometry* geometry, GFX::Material* material)
{
    // load geometry
    GlobalGeometry::GeometryGPU* geometryGPU = m_GlobalGeometryManager.FindOrUploadGeometry(geometry);
    if (m_RayTracingManager.GetGeometryBLAS(geometry) == nullptr)
    {
        m_RayTracingManager.RegisterGeometry(geometry, context);
    }

    InstanceDataManager::InstanceGPU instanceGPU = m_InstanceDataManager.AllocateInstance(material->GetMaterialGPU());
    instanceGPU.ScheduleUpdate(
        sceneNode->GetGlobalMatrix(),
        glm::inverse(sceneNode->GetGlobalMatrix()),
        sceneNode->GetGlobalID(),
        InstanceFlags{ 0 },
        geometryGPU->GetIndexOffset(),
        geometryGPU->GetVertexOffset()
    );

    RenderableObject* renderable = m_RenderableObjectPool.Alloc(sceneNode, instanceGPU, *geometryGPU, m_RayTracingManager.GetGeometryBLAS(geometry)->m_LogicalHandle);

    switch (material->GetType())
    {
    case Material::Type::MATERIAL_TYPE_OPAQUE:
    case Material::Type::MATERIAL_TYPE_ALPHA_MASKED:
        renderable->AddLayerPipeline(RenderableObject::LAYER_FORWARD, m_PipelineDB.GetEntry("forward_pbr")->GetPipeline());
        renderable->AddLayerPipeline(RenderableObject::LAYER_GBUFFER, m_PipelineDB.GetEntry("gbuffer_pbr")->GetPipeline());
        break;
    default:
        DRE_ASSERT(false, "No corresponding pipeline in PipelineDB for this material type.");
        break;
    }

    return renderable;
}

void GraphicsManager::FreeRenderableObject(RenderableObject* obj)
{
    m_RenderableObjectPool.Free(obj);
}

GFX::Material* GraphicsManager::CreateMaterial(char const* name, GFX::Material::Type type)
{
    MaterialsManager::MaterialGPU materialGPU = m_MaterialsManager.AllocateMaterial();
    return &m_Materials.Emplace(name, type, materialGPU);
}

GFX::ResourceBinder GraphicsManager::CreateResourceBinder(PipelineEntry* pipelineEntry, DRE::U32 setIDAfterGlobalSets)
{
    return ResourceBinder(&m_Device, pipelineEntry, setIDAfterGlobalSets, GetCurrentFrameID());
}

void GraphicsManager::WaitIdle()
{
    GetMainContext().WaitIdle();
}

GraphicsManager::~GraphicsManager()
{
    WaitIdle();
}

}

