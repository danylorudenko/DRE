#include <gfx\GraphicsManager.hpp>

#include <foundation\system\Window.hpp>


#include <vk_wrapper\Device.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <gfx\pass\ForwardOpaquePass.hpp>
#include <gfx\pass\ImGuiRenderPass.hpp>

#include <engine\io\IOManager.hpp>
#include <global_uniform.h>

namespace GFX
{

static constexpr std::uint32_t C_DEFAULT_STAGING_ARENA_SIZE     = 1024 * 1024 * 128;
static constexpr std::uint32_t C_DEFAULT_UNIFORM_ARENA_SIZE     = DRE_U16_MAX - 1;
static constexpr std::uint32_t C_DEFAULT_READBACK_ARENA_SIZE    = 1024 * 1024 * 64;
static constexpr std::uint32_t C_GLOBAL_UNIFORM_SIZE            = 256;

GraphicsManager* g_GraphicsManager = nullptr;

GraphicsManager::GraphicsManager(HINSTANCE hInstance, Window* window, IO::IOManager* ioManager, bool debug)
    : m_MainWindow{ window }
    , m_IOManager{ ioManager }
    , m_Device{ hInstance, window->NativeHandle(), debug}
    , m_MainContext{ m_Device.table_.get(), m_Device.GetMainQueue() }
    , m_GraphicsFrame{ 0 }
    , m_UploadArena{ &m_Device, C_DEFAULT_STAGING_ARENA_SIZE }
    , m_UniformArena{ &m_Device, C_DEFAULT_UNIFORM_ARENA_SIZE }
    , m_ReadbackArena{ &m_Device, C_DEFAULT_UNIFORM_ARENA_SIZE }
    , m_TextureBank{ &m_MainContext, m_Device.GetResourcesController(), m_Device.GetDescriptorAllocator() }
    , m_PipelineDB{ &m_Device, ioManager }
    , m_PersistentStorage{ &m_Device }
    , m_RenderGraph{ this }
    , m_DependencyManager{ &m_RenderGraph.ResourcesManager() }
{
    g_GraphicsManager = this;

    for (std::uint32_t i = 0; i < VKW::CONSTANTS::FRAMES_BUFFERING; i++)
    {
        m_GlobalUniforms[i] = m_Device.GetResourcesController()->CreateBuffer(C_GLOBAL_UNIFORM_SIZE, VKW::BufferUsage::UNIFORM);
    }
    m_Device.GetDescriptorAllocator()->AllocateDefaultDescriptors(VKW::CONSTANTS::FRAMES_BUFFERING, m_GlobalUniforms, m_PersistentStorage.GetStorage().GetResource());
}

void GraphicsManager::Initialize()
{
    CreateAllPasses();
}

void GraphicsManager::CreateAllPasses()
{
    m_RenderGraph.AddPass<ForwardOpaquePass>();
    m_RenderGraph.AddPass<ImGuiRenderPass>();
    m_RenderGraph.ParseGraph();
    m_RenderGraph.InitGraphResources();
}

void GraphicsManager::PrepareGlobalData(VKW::Context& context, std::uint64_t deltaTimeUS)
{
    std::uint8_t C_STD140_DATA_STRIDE = 16;
    
    VKW::BufferResource* buffer = m_GlobalUniforms[GetCurrentFrameID()];
    void* dst = buffer->memory_.GetRegionMappedPtr();

    GlobalUniform globalUniform{};
    globalUniform.viewportSize_deltaMS_0[0] = static_cast<float>(GetRenderingWidth());
    globalUniform.viewportSize_deltaMS_0[1] = static_cast<float>(GetRenderingHeight());
    globalUniform.viewportSize_deltaMS_0[2] = static_cast<float>(static_cast<double>(deltaTimeUS) / 1000.0);
    globalUniform.viewportSize_deltaMS_0[3] = 0.0f;

    std::memcpy(dst, &globalUniform, sizeof(globalUniform));

    buffer->memory_.FlushCaches(g_GraphicsManager->GetVulkanTable(), g_GraphicsManager->GetMainDevice()->GetLogicalDevice());

    context.CmdResourceDependency(buffer,
        VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST,
        VKW::RESOURCE_ACCESS_SHADER_UNIFORM, VKW::STAGE_VERTEX);
}

void GraphicsManager::RenderFrame(std::uint64_t frame, std::uint64_t deltaTimeUS)
{
    m_GraphicsFrame = frame;

    m_UniformArena.ResetAllocations(GetCurrentFrameID());
    m_UploadArena.ResetAllocations(GetCurrentFrameID());
    m_ReadbackArena.ResetAllocations(GetCurrentFrameID());

    VKW::Context& context = GetMainContext();
    PrepareGlobalData(context, deltaTimeUS);

    // globalData
    context.CmdBindGlobalDescriptorSets(*GetMainDevice()->GetDescriptorAllocator(), GetCurrentFrameID());

    // main graph
    StorageTexture& finalRT = m_RenderGraph.Render(context);

    // presentation
    m_DependencyManager.ResourceBarrier(context, finalRT.GetResource(), VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);
    TransferToSwapchainAndPresent(finalRT);
}

void GraphicsManager::TransferToSwapchainAndPresent(StorageTexture& src)
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

    GetMainContext().FlushWaitSwapchain(presentationContext);
    GetMainContext().Present(presentationContext);
}

void WriteMemorySequence(void*& memory, void const* data, std::uint32_t size)
{
    std::memcpy(memory, data, size);
    memory = DRE::PtrAdd(memory, size);
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
    default:
        DRE_ASSERT(false, "No corresponding pipeline in PipelineDB for this material type.");
        break;
    }

    VKW::DescriptorManager* descriptorManager = GetMainDevice()->GetDescriptorAllocator();

    DRE::InplaceVector<VKW::DescriptorSet, VKW::CONSTANTS::MAX_PIPELINE_LAYOUT_MEMBERS> descriptorSets;
    for (std::uint8_t i = std::uint8_t(descriptorManager->GetGlobalSetLayoutsCount()), count = std::uint8_t(layout->GetMemberCount()); i < count; i++)
    {
        descriptorSets.EmplaceBack(descriptorManager->AllocateStandaloneSet(*layout->GetMember(i)));
    }
    // ye, but sets are emtpy!!!

    // - load textures
    RenderableObject::TexturesVector textures;
    for (std::uint32_t i = 0, count = std::uint32_t(Data::Material::TextureProperty::MAX); i < count; i++)
    {
        Data::Texture2D const& texture = material->GetTexture(Data::Material::TextureProperty::Slot(i));
        if (!texture.IsInitialized())
            continue;

        textures.EmplaceBack(
            m_TextureBank.LoadTexture2DSync(
                texture.GetName(),
                texture.GetSizeX(),
                texture.GetSizeY(),
                texture.GetFormat(),
                texture.GetBuffer()
            )
        );
    }
    // - write other descriptors, texture descriptors are written by LoadTexture2DSync


    // load geometry
    GeometryGPU* geometryGPU = m_GeometryGPUMap.Find(geometry).value;
    if (geometryGPU == nullptr)
    {
        std::uint32_t const vertexMemoryRequirements = geometry->GetVertexSizeInBytes();
        std::uint32_t const indexMemoryRequirements = geometry->GetIndexSizeInBytes();

        std::uint32_t const meshMemoryRequirements = vertexMemoryRequirements + indexMemoryRequirements;


        auto meshMemory = GFX::g_GraphicsManager->GetUploadArena().AllocateTransientRegion(GFX::g_GraphicsManager->GetCurrentFrameID(), meshMemoryRequirements, 256);
        void* memorySequence = meshMemory.m_MappedRange;

        WriteMemorySequence(memorySequence, geometry->GetVertexData(), vertexMemoryRequirements);

        void* indexStart = memorySequence;

        WriteMemorySequence(memorySequence, geometry->GetIndexData(), indexMemoryRequirements);

        meshMemory.FlushCaches();

        VKW::BufferResource* vertexBuffer = GFX::g_GraphicsManager->GetMainDevice()->GetResourcesController()->CreateBuffer(vertexMemoryRequirements, VKW::BufferUsage::VERTEX_INDEX);
        VKW::BufferResource* indexBuffer = GFX::g_GraphicsManager->GetMainDevice()->GetResourcesController()->CreateBuffer(indexMemoryRequirements, VKW::BufferUsage::VERTEX_INDEX);

        context.CmdResourceDependency(meshMemory.m_Buffer, meshMemory.m_OffsetInBuffer, meshMemory.m_Size, VKW::RESOURCE_ACCESS_HOST_WRITE, VKW::STAGE_HOST, VKW::RESOURCE_ACCESS_TRANSFER_SRC, VKW::STAGE_TRANSFER);

        // schedule copy
        context.CmdResourceDependency(vertexBuffer, VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);
        context.CmdCopyBufferToBuffer(vertexBuffer, 0, meshMemory.m_Buffer, meshMemory.m_OffsetInBuffer, vertexMemoryRequirements);

        context.CmdResourceDependency(indexBuffer, VKW::RESOURCE_ACCESS_NONE, VKW::STAGE_TOP, VKW::RESOURCE_ACCESS_TRANSFER_DST, VKW::STAGE_TRANSFER);

        geometryGPU = &m_GeometryGPUMap.Emplace(geometry, vertexBuffer, indexBuffer);
    }


    //RenderableObject(VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, VKW::BufferResource* indexBuffer, TexturesVector&& textures, DescriptorSetVector&& sets);
    return m_RenderableObjectPool.Alloc(pipeline, geometryGPU->vertexBuffer, geometryGPU->indexBuffer, DRE_MOVE(textures), DRE_MOVE(descriptorSets));
}

void GraphicsManager::FreeRenderableObject(RenderableObject* obj)
{
    GetRenderablePool().Free(obj);
}

GraphicsManager::~GraphicsManager()
{
    GetMainContext().WaitIdle();
}

}

