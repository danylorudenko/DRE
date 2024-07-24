#include <gfx\GraphicsManager.hpp>

#include <foundation\system\Window.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\pass\ForwardOpaquePass.hpp>
#include <gfx\pass\WaterPass.hpp>
#include <gfx\pass\FFTWaterPass.hpp>
#include <gfx\pass\AntiAliasingPass.hpp>
#include <gfx\pass\CausticPass.hpp>
#include <gfx\pass\ColorEncodingPass.hpp>
#include <gfx\pass\ImGuiRenderPass.hpp>
#include <gfx\pass\ShadowPass.hpp>
#include <gfx\pass\DebugPass.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>
#include <global_uniform.h>

namespace GFX
{

static constexpr std::uint32_t C_STAGING_ARENA_SIZE         = 1024 * 1024 * 128;
static constexpr std::uint32_t C_UNIFORM_ARENA_SIZE         = DRE_U16_MAX * 2 - 1;
static constexpr std::uint32_t C_READBACK_ARENA_SIZE        = 1024 * 1024 * 64;
static constexpr std::uint32_t C_PERSISTENT_STORAGE_SIZE    = 1024 * 1024 * 16;

GraphicsManager* g_GraphicsManager = nullptr;

GraphicsManager::GraphicsManager(HINSTANCE hInstance, Window* window, IO::IOManager* ioManager, bool debug)
    : m_MainWindow{ window }
    , m_IOManager{ ioManager }
    , m_Device{ hInstance, window->NativeHandle(), debug}
    , m_MainContext{ m_Device.GetFuncTable(), m_Device.GetMainQueue(), &DRE::g_FrameScratchAllocator }
    , m_GraphicsFrame{ 0 }
    , m_UploadArena{ &m_Device, C_STAGING_ARENA_SIZE }
    , m_UniformArena{ &m_Device, C_UNIFORM_ARENA_SIZE }
    , m_ReadbackArena{ &m_Device, C_READBACK_ARENA_SIZE }
    , m_TextureBank{ &m_MainContext, m_Device.GetResourcesController(), m_Device.GetDescriptorManager() }
    , m_PipelineDB{ &m_Device, ioManager }
#ifdef DRE_IMGUI_CUSTOM_TEXTURE
    , m_ImGuiSyncQueue{ &DRE::g_MainAllocator }
#endif
    , m_PersistentStorage{ &m_Device, &m_UploadArena, &m_Device, C_PERSISTENT_STORAGE_SIZE }
    , m_LightsManager{ &m_PersistentStorage }
    , m_RenderGraph{ this }
    , m_DependencyManager{}
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

void GraphicsManager::LoadDefaultData()
{
    m_PipelineDB.CreateDefaultPipelines();
    m_TextureBank.LoadDefaultTextures();
    CreateAllPasses();
}

void GraphicsManager::CreateAllPasses()
{
    m_RenderGraph.AddPass<ShadowPass>();
    m_RenderGraph.AddPass<CausticPass>();
    m_RenderGraph.AddPass<ForwardOpaquePass>();
    m_RenderGraph.AddPass<FFTButterflyGenPass>();
    m_RenderGraph.AddPass<FFTWaterH0GenPass>();
    m_RenderGraph.AddPass<FFTWaterHxtGenPass>();
    m_RenderGraph.AddPass<FFTWaterFFTPass>();
    m_RenderGraph.AddPass<FFTInvPermutationPass>();
    m_RenderGraph.AddPass<WaterPass>();
    m_RenderGraph.AddPass<AntiAliasingPass>();
    m_RenderGraph.AddPass<ColorEncodingPass>();
    m_RenderGraph.AddPass<DebugPass>();
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

    globalUniform.main_CameraPos_GenericScalar = glm::vec4{ scene.GetMainCamera().GetPosition(), GetGraphicsSettings().m_GenericScalar };
    globalUniform.main_CameraDir        = glm::vec4{ scene.GetMainCamera().GetForward(), 0.0f };
    globalUniform.main_Jitter           = glm::vec4{ taaJitter, 0.0f, 0.0f };

    globalUniform.main_ViewM            = m_MainView.GetViewM();
    globalUniform.main_iViewM           = m_MainView.GetInvViewM();
    globalUniform.main_ProjM            = m_MainView.GetProjectionM();
    globalUniform.main_ViewProjM        = m_MainView.GetViewProjectionM();
    globalUniform.main_iViewProjM       = m_MainView.GetInvViewProjectionM();

    globalUniform.main_PrevViewM        = m_MainView.GetPrevViewM();
    globalUniform.main_PreviViewM       = m_MainView.GetPrevInvViewM();
    globalUniform.main_PrevProjM        = m_MainView.GetPrevProjectionM();
    globalUniform.main_PrevViewProjM    = m_MainView.GetPrevViewProjectionM();
    globalUniform.main_PreviViewProjM   = m_MainView.GetPrevInvViewProjectionM();

    globalUniform.main_ShadowVP         = m_SunShadowView.GetViewProjectionM();
    globalUniform.main_ShadowSize       = glm::vec4{ C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT, 0.0f, 0.0f };

    globalUniform.main_SunLightDir      = glm::vec4{ sunLight.GetForward(), 0.0f };

    globalUniform.lightsCount           = glm::uvec4{ m_LightsManager.GetLightsCount(), 0u, 0u, 0u };
    globalUniform.LightBuffer           = m_LightsManager.GetBufferAddress();

    std::memcpy(dst, &globalUniform, sizeof(globalUniform));

    buffer->memory_.FlushCaches(g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice());

    context.CmdResourceDependency(buffer,
        VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST,
        VKW::RESOURCE_ACCESS_SHADER_UNIFORM, VKW::STAGE_VERTEX);
}

void GraphicsManager::ReloadShaders()
{
    auto names = m_IOManager->GetPendingShaders();
    for (std::uint32_t i = 0; i < names.Size(); i++)
    {
        m_PipelineDB.ReloadPipeline(names[i].GetData());
    }
}

void GraphicsManager::RenderFrame(std::uint64_t frame, std::uint64_t deltaTimeUS, float globalTimeS)
{
    m_GraphicsFrame = frame;

    // need to wait for currentFrame - 2 to complete
    if (m_FrameProcessingCompletePoint[GetCurrentFrameID()].GetQueue() != nullptr)
        m_FrameProcessingCompletePoint[GetCurrentFrameID()].Wait();

    m_UniformArena.ResetAllocations(GetCurrentFrameID());
    m_UploadArena.ResetAllocations(GetCurrentFrameID());
    m_ReadbackArena.ResetAllocations(GetCurrentFrameID());

    VKW::Context& context = GetMainContext();

    DRE_GPU_SCOPE(FRAME);

    context.ResetDependenciesVectors(&DRE::g_FrameScratchAllocator);
    PrepareGlobalData(context,  *WORLD::g_MainScene, deltaTimeUS, globalTimeS);
    m_LightsManager.UpdateGPULights(context);

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

void WriteMemorySequence(void*& memory, void const* data, std::uint32_t size)
{
    std::memcpy(memory, data, size);
    memory = DRE::PtrAdd(memory, size);
}

GraphicsManager::GeometryGPU* GraphicsManager::LoadGPUGeometry(VKW::Context& context, Data::Geometry* geometry)
{
    std::uint32_t const vertexMemoryRequirements = geometry->GetVertexSizeInBytes();
    std::uint32_t const indexMemoryRequirements = geometry->GetIndexSizeInBytes();
    std::uint32_t const meshMemoryRequirements = vertexMemoryRequirements + indexMemoryRequirements;

    auto meshMemory = GetUploadArena().AllocateTransientRegion(GetCurrentFrameID(), meshMemoryRequirements, 256);

    void* memorySequence = meshMemory.m_MappedRange;
    WriteMemorySequence(memorySequence, geometry->GetVertexData(), vertexMemoryRequirements);
    void* indexStart = memorySequence;
    WriteMemorySequence(memorySequence, geometry->GetIndexData(), indexMemoryRequirements);

    meshMemory.FlushCaches();

    VKW::BufferResource* vertexBuffer = GetMainDevice()->GetResourcesController()->CreateBuffer(vertexMemoryRequirements, VKW::BufferUsage::VERTEX_INDEX, "data_vtx");
    VKW::BufferResource* indexBuffer = GetMainDevice()->GetResourcesController()->CreateBuffer(indexMemoryRequirements, VKW::BufferUsage::VERTEX_INDEX, "data_idx");

    context.CmdResourceDependency(meshMemory.m_Buffer, meshMemory.m_OffsetInBuffer, meshMemory.m_Size, VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

    // schedule copy
    context.CmdResourceDependency(vertexBuffer, VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
    context.CmdCopyBufferToBuffer(vertexBuffer, 0, meshMemory.m_Buffer, meshMemory.m_OffsetInBuffer, vertexMemoryRequirements);
    context.CmdResourceDependency(vertexBuffer, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER, VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_INPUT_ASSEMBLER);

    context.CmdResourceDependency(indexBuffer, VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
    context.CmdCopyBufferToBuffer(indexBuffer, 0, meshMemory.m_Buffer, meshMemory.m_OffsetInBuffer + vertexMemoryRequirements, indexMemoryRequirements);
    context.CmdResourceDependency(indexBuffer, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER, VKW::RESOURCE_ACCESS_GENERIC_READ, VKW::STAGE_INPUT_ASSEMBLER);

    return &m_GeometryGPUMap.Emplace(geometry, vertexBuffer, indexBuffer);
}

void EmplaceRenderableObjectTexture(Data::Material* material, Data::Material::TextureProperty::Slot slot, TextureBank& textureBank, char const* defaultName, RenderableObject::TexturesVector& result)
{
    Data::Texture2D const& texture = material->GetTexture(slot);
    if (!texture.IsInitialized())
    {
        result.EmplaceBack(textureBank.FindTexture(defaultName));
    }
    else
    {
        Texture* gfxTexture = textureBank.FindTexture(texture.GetName());

        result.EmplaceBack(gfxTexture == nullptr 
            ? textureBank.LoadTexture2DSync(texture.GetName(), texture.GetSizeX(), texture.GetSizeY(), texture.GetFormat(), texture.GetBuffer()) 
            : gfxTexture);
    }
}

RenderableObject* GraphicsManager::CreateRenderableObject(WORLD::SceneNode* sceneNode, VKW::Context& context, Data::Geometry* geometry, Data::Material* material)
{
    DRE::String64 name = material->GetRenderingProperties().GetShader();
    VKW::Pipeline* pipeline = m_PipelineDB.GetPipeline(name.GetData());
    VKW::Pipeline* shadowPipeline = m_PipelineDB.GetPipeline("forward_shadow");

    name.Append("_layout");
    VKW::PipelineLayout* layout = m_PipelineDB.GetLayout(name.GetData());
    VKW::PipelineLayout* shadowLayout = m_PipelineDB.GetLayout("forward_shadow_layout");

    RenderableObject::LayerBits layers = RenderableObject::LAYER_NONE;
    switch (material->GetRenderingProperties().GetMaterialType())
    {
    case Data::Material::RenderingProperties::MATERIAL_TYPE_OPAQUE:
        layers = RenderableObject::LAYER_OPAQUE_BIT;
        break;
    case Data::Material::RenderingProperties::MATERIAL_TYPE_WATER:
        layers = RenderableObject::LAYER_WATER_BIT;
        break;
    default:
        DRE_ASSERT(false, "No corresponding pipeline in PipelineDB for this material type.");
        break;
    }

    // - load textures
    RenderableObject::TexturesVector textures;
    EmplaceRenderableObjectTexture(material, Data::Material::TextureProperty::DIFFUSE, m_TextureBank, "default_color", textures);
    EmplaceRenderableObjectTexture(material, Data::Material::TextureProperty::NORMAL, m_TextureBank, "default_normal", textures);
    EmplaceRenderableObjectTexture(material, Data::Material::TextureProperty::METALNESS, m_TextureBank, "zero_r", textures);
    EmplaceRenderableObjectTexture(material, Data::Material::TextureProperty::ROUGHNESS, m_TextureBank, "one_r", textures);

    // load geometry
    GeometryGPU* geometryGPU = m_GeometryGPUMap.Find(geometry).value;
    if (geometryGPU == nullptr)
        geometryGPU = LoadGPUGeometry(context, geometry);

    RenderableObject::DescriptorSetVector descriptors;
    RenderableObject::DescriptorSetVector shadowDescriptors;

    VKW::DescriptorManager* descriptorManager = GetMainDevice()->GetDescriptorManager();

    std::uint8_t const mainRenderingPassSetCount = m_RenderGraph.GetPassDescriptorSet(PassID::ForwardOpaque, GetCurrentFrameID()).IsValid() ? 1 : 0;
    std::uint8_t const shadowPassSetCount = m_RenderGraph.GetPassDescriptorSet(PassID::Shadow, GetCurrentFrameID()).IsValid() ? 1 : 0;

    std::uint8_t const layoutMemberId = std::uint8_t(descriptorManager->GetGlobalSetLayoutsCount() + mainRenderingPassSetCount); // globals + pass set
    std::uint8_t const shadowLayoutMemberId = std::uint8_t(descriptorManager->GetGlobalSetLayoutsCount() + shadowPassSetCount); // globals + pass set

    DRE_ASSERT(layout->GetMemberCount() == layoutMemberId + 1, "All renderable items should currently contain everything in one set.");
    for (std::uint8_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
    {
        descriptors.EmplaceBack(descriptorManager->AllocateStandaloneSet(*layout->GetMember(layoutMemberId)));
    }

    for (std::uint8_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
    {
        shadowDescriptors.EmplaceBack(descriptorManager->AllocateStandaloneSet(*shadowLayout->GetMember(shadowLayoutMemberId)));
    }

    return m_RenderableObjectPool.Alloc(sceneNode, layers, pipeline, geometryGPU->vertexBuffer, geometry->GetVertexCount(),
        geometryGPU->indexBuffer, geometry->GetIndexCount(),
        DRE_MOVE(textures), DRE_MOVE(descriptors), DRE_MOVE(shadowDescriptors));
}

void GraphicsManager::FreeRenderableObject(RenderableObject* obj)
{
    m_RenderableObjectPool.Free(obj);
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

