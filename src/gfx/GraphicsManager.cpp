#include <gfx\GraphicsManager.hpp>

#include <foundation\system\Window.hpp>

#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\pass\ForwardOpaquePass.hpp>
#include <gfx\pass\AntiAliasingPass.hpp>
#include <gfx\pass\ImGuiRenderPass.hpp>
#include <gfx\pass\ShadowPass.hpp>
#include <gfx\pass\ColorEncodingPass.hpp>

#include <engine\io\IOManager.hpp>
#include <engine\scene\Scene.hpp>
#include <global_uniform.h>

namespace GFX
{

static constexpr std::uint32_t C_DEFAULT_STAGING_ARENA_SIZE     = 1024 * 1024 * 128;
static constexpr std::uint32_t C_DEFAULT_UNIFORM_ARENA_SIZE     = DRE_U16_MAX - 1;
static constexpr std::uint32_t C_DEFAULT_READBACK_ARENA_SIZE    = DRE_U16_MAX - 1;

GraphicsManager* g_GraphicsManager = nullptr;

GraphicsManager::GraphicsManager(HINSTANCE hInstance, Window* window, IO::IOManager* ioManager, bool debug)
    : m_MainWindow{ window }
    , m_IOManager{ ioManager }
    , m_Device{ hInstance, window->NativeHandle(), debug}
    , m_MainContext{ m_Device.GetFuncTable(), m_Device.GetMainQueue()}
    , m_GraphicsFrame{ 0 }
    , m_UploadArena{ &m_Device, C_DEFAULT_STAGING_ARENA_SIZE }
    , m_UniformArena{ &m_Device, C_DEFAULT_UNIFORM_ARENA_SIZE }
    , m_ReadbackArena{ &m_Device, C_DEFAULT_READBACK_ARENA_SIZE }
    , m_TextureBank{ &m_MainContext, m_Device.GetResourcesController(), m_Device.GetDescriptorManager() }
    , m_PipelineDB{ &m_Device, ioManager }
    , m_PersistentStorage{ &m_Device }
    , m_RenderGraph{ this }
    , m_DependencyManager{}
    , m_MainView{ &DRE::g_MainAllocator }
    , m_SunShadowView{ &DRE::g_MainAllocator }
    , m_Settings{}
{
    g_GraphicsManager = this;

    for (std::uint32_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
    {
        m_GlobalUniforms[i] = m_Device.GetResourcesController()->CreateBuffer(sizeof(GlobalUniforms), VKW::BufferUsage::UNIFORM);
    }
    m_Device.GetDescriptorManager()->AllocateDefaultDescriptors(VKW::CONSTANTS::FRAMES_BUFFERING, m_GlobalUniforms, m_PersistentStorage.GetStorage().GetResource());
}

void GraphicsManager::Initialize()
{
    m_PipelineDB.CreateDefaultPipelines();
    m_TextureBank.LoadDefaultTextures();
    CreateAllPasses();
}

void GraphicsManager::CreateAllPasses()
{
    m_RenderGraph.AddPass<ShadowPass>();
    m_RenderGraph.AddPass<ForwardOpaquePass>();
    m_RenderGraph.AddPass<AntiAliasingPass>();
    m_RenderGraph.AddPass<ColorEncodingPass>();
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

void GraphicsManager::PrepareGlobalData(VKW::Context& context, WORLD::Scene& scene, std::uint64_t deltaTimeUS)
{
    m_MainView.UpdatePreviosFrame();
    m_SunShadowView.UpdatePreviosFrame();
    
    VKW::BufferResource* buffer = m_GlobalUniforms[GetCurrentFrameID()];
    void* dst = buffer->memory_.GetRegionMappedPtr();

    glm::vec2 const halton = s_HaltonSequence[GetCurrentGraphicsFrame() % (sizeof(s_HaltonSequence) / sizeof(glm::vec2))];
    glm::vec2 const taaJitter = ((halton - 0.5f) / glm::vec2(m_MainView.GetSize())) * 2.0f * glm::vec2{ GetGraphicsSettings().m_JitterScale };


    WORLD::Camera const& camera = scene.GetMainCamera();
    m_MainView.UpdatePlacement(camera.GetPosition(), camera.GetForward(), camera.GetUp());
    m_MainView.UpdateViewport(glm::uvec2{ 0, 0 }, glm::uvec2{ GetRenderingWidth(), GetRenderingHeight() });
    m_MainView.UpdateProjection(camera.GetFOV(), camera.GetRange()[0], camera.GetRange()[1], taaJitter.x, taaJitter.y);

    WORLD::DirectionalLight const& sunLight = scene.GetMainSunLight();
    m_SunShadowView.UpdatePlacement(sunLight.GetPosition(), sunLight.GetForward(), sunLight.GetUp());
    m_SunShadowView.UpdateViewport(glm::uvec2{ 0, 0 }, glm::uvec2{ C_SHADOW_MAP_WIDTH, C_SHADOW_MAP_HEIGHT });
    m_SunShadowView.UpdateProjection(
        -C_SHADOW_MAP_WORLD_EXTENT, C_SHADOW_MAP_WORLD_EXTENT,
        -C_SHADOW_MAP_WORLD_EXTENT, C_SHADOW_MAP_WORLD_EXTENT,
        -C_SHADOW_MAP_WORLD_EXTENT, C_SHADOW_MAP_WORLD_EXTENT);


    GlobalUniforms globalUniform{};
    globalUniform.viewportSize_deltaMS_0[0] = static_cast<float>(GetRenderingWidth());
    globalUniform.viewportSize_deltaMS_0[1] = static_cast<float>(GetRenderingHeight());
    globalUniform.viewportSize_deltaMS_0[2] = static_cast<float>(static_cast<double>(deltaTimeUS) / 1000.0);
    globalUniform.viewportSize_deltaMS_0[3] = 0.0f;

    globalUniform.main_CameraPos        = glm::vec4{ scene.GetMainCamera().GetPosition(), 1.0f };
    globalUniform.main_CameraDir        = glm::vec4{ scene.GetMainCamera().GetForward(), 0.0f };
    globalUniform.main_Jitter           = glm::vec4{ taaJitter, 0.0f, 0.0f };

    globalUniform.main_ViewM            = m_MainView.GetViewM();
    globalUniform.main_iViewM           = m_MainView.GetInvViewM();
    globalUniform.main_ProjM            = m_MainView.GetProjectionM();
    globalUniform.main_ProjJittM        = m_MainView.GetProjectionJitteredM();
    globalUniform.main_iProjJittM       = m_MainView.GetInvProjectionJitteredM();
    globalUniform.main_ViewProjM        = m_MainView.GetViewProjectionM();
    globalUniform.main_ViewProjJittM    = m_MainView.GetViewProjectionJitteredM();
    globalUniform.main_iViewProjM       = m_MainView.GetInvViewProjectionM();
    globalUniform.main_iViewProjJittM   = m_MainView.GetInvViewProjectionJitteredM();

    globalUniform.main_PrevViewM          = m_MainView.GetPrevViewM();
    globalUniform.main_PreviViewM         = m_MainView.GetPrevInvViewM();
    globalUniform.main_PrevProjM          = m_MainView.GetPrevProjectionM();
    globalUniform.main_PrevProjJittM      = m_MainView.GetPrevProjectionJitteredM();
    globalUniform.main_PreviProjJittM     = m_MainView.GetPrevInvProjectionJitteredM();
    globalUniform.main_PrevViewProjM      = m_MainView.GetPrevViewProjectionM();
    globalUniform.main_PrevViewProjJittM  = m_MainView.GetPrevViewProjectionJitteredM();
    globalUniform.main_PreviViewProjM     = m_MainView.GetPrevInvViewProjectionM();
    globalUniform.main_PreviViewProjJittM = m_MainView.GetPrevInvViewProjectionJitteredM();

    globalUniform.main_LightDir         = glm::vec4{ scene.GetMainSunLight().GetForward(), 0.0f };
    globalUniform.main_LightRadiance    = glm::vec4{ scene.GetMainSunLight().GetRadiance(), 1.0f };

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

void GraphicsManager::RenderFrame(std::uint64_t frame, std::uint64_t deltaTimeUS)
{
    m_GraphicsFrame = frame;

    // need to wait for currentFrame - 2 to complete
    if (m_FrameProcessingCompletePoint[GetCurrentFrameID()].GetQueue() != nullptr)
        m_FrameProcessingCompletePoint[GetCurrentFrameID()].Wait();

    m_UniformArena.ResetAllocations(GetCurrentFrameID());
    m_UploadArena.ResetAllocations(GetCurrentFrameID());
    m_ReadbackArena.ResetAllocations(GetCurrentFrameID());

    VKW::Context& context = GetMainContext();
    PrepareGlobalData(context,  *WORLD::g_MainScene, deltaTimeUS);

    // globalData
    context.CmdBindGlobalDescriptorSets(*GetMainDevice()->GetDescriptorManager(), GetCurrentFrameID());

    // main graph
    StorageTexture& finalRT = m_RenderGraph.Render(context);

    // presentation
    m_DependencyManager.ResourceBarrier(context, finalRT.GetResource(), VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);
    m_FrameProcessingCompletePoint[GetCurrentFrameID()] = TransferToSwapchainAndPresent(finalRT);
}

VKW::QueueExecutionPoint GraphicsManager::TransferToSwapchainAndPresent(StorageTexture& src)
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

    VKW::BufferResource* vertexBuffer = GetMainDevice()->GetResourcesController()->CreateBuffer(vertexMemoryRequirements, VKW::BufferUsage::VERTEX_INDEX);
    VKW::BufferResource* indexBuffer = GetMainDevice()->GetResourcesController()->CreateBuffer(indexMemoryRequirements, VKW::BufferUsage::VERTEX_INDEX);

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
        ReadOnlyTexture* gfxTexture = textureBank.FindTexture(texture.GetName());

        result.EmplaceBack(gfxTexture == nullptr 
            ? textureBank.LoadTexture2DSync(texture.GetName(), texture.GetSizeX(), texture.GetSizeY(), texture.GetFormat(), texture.GetBuffer()) 
            : gfxTexture);
    }
}

RenderableObject* GraphicsManager::CreateRenderableObject(VKW::Context& context, Data::Geometry* geometry, Data::Material* material)
{
    VKW::Pipeline* pipeline = nullptr;
    VKW::PipelineLayout* layout = nullptr;
    switch (material->GetRenderingProperties().GetMaterialType())
    {
    case Data::Material::RenderingProperties::MATERIAL_TYPE_DEFAULT_LIT:
        pipeline = GetPipelineDB().GetPipeline("default_lit");
        layout = GetPipelineDB().GetLayout("default_lit_layout");
        break;
    case Data::Material::RenderingProperties::MATERIAL_TYPE_COOK_TORRANCE:
        pipeline = GetPipelineDB().GetPipeline("cook_torrance");
        layout = GetPipelineDB().GetLayout("cook_torrance_layout");
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

    VKW::DescriptorManager* descriptorManager = GetMainDevice()->GetDescriptorManager();
    std::uint8_t const mainRenderingPassSetCount = m_RenderGraph.GetPassDescriptorSet(PassID::ForwardOpaque, GetCurrentFrameID()).IsValid() ? 1 : 0;
    std::uint8_t const itemDescriptorId = std::uint8_t(descriptorManager->GetGlobalSetLayoutsCount() + mainRenderingPassSetCount); // globals + pass set
    DRE_ASSERT(layout->GetMemberCount() == itemDescriptorId + 1, "All renderable items should currently contain everything in one set.");
    for (std::uint8_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
    {
        descriptors.EmplaceBack(descriptorManager->AllocateStandaloneSet(*layout->GetMember(itemDescriptorId)));
    }
    return m_RenderableObjectPool.Alloc(pipeline, geometryGPU->vertexBuffer, geometry->GetVertexCount(), geometryGPU->indexBuffer, geometry->GetIndexCount(), DRE_MOVE(textures), DRE_MOVE(descriptors));
}

RenderableObject* GraphicsManager::CreateShadowRenderableObject(VKW::Context& context, Data::Geometry* geometry)
{
    VKW::Pipeline* pipeline = GetPipelineDB().GetPipeline("forward_shadow");
    VKW::PipelineLayout* layout = GetPipelineDB().GetLayout("forward_shadow_layout");

    // load geometry
    GeometryGPU* geometryGPU = m_GeometryGPUMap.Find(geometry).value;
    if (geometryGPU == nullptr)
        geometryGPU = LoadGPUGeometry(context, geometry);

    RenderableObject::DescriptorSetVector descriptors;

    VKW::DescriptorManager* descriptorManager = GetMainDevice()->GetDescriptorManager();
    std::uint8_t const mainRenderingPassSetCount = m_RenderGraph.GetPassDescriptorSet(PassID::Shadow, GetCurrentFrameID()).IsValid() ? 1 : 0;
    std::uint8_t const itemDescriptorId = std::uint8_t(descriptorManager->GetGlobalSetLayoutsCount() + mainRenderingPassSetCount); // globals + pass set
    DRE_ASSERT(layout->GetMemberCount() == itemDescriptorId + 1, "All renderable items should currently contain everything in one set.");
    for (std::uint8_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
    {
        descriptors.EmplaceBack(descriptorManager->AllocateStandaloneSet(*layout->GetMember(itemDescriptorId)));
    }
    return m_RenderableObjectPool.Alloc(pipeline, geometryGPU->vertexBuffer, geometry->GetVertexCount(), geometryGPU->indexBuffer, geometry->GetIndexCount(), DRE_MOVE(descriptors));
}

void GraphicsManager::FreeRenderableObject(RenderableObject* obj)
{
    GetRenderablePool().Free(obj);
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

