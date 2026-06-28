#include "DREApplicationDelegate.hpp"

#include "AppUtils.hpp"

#include <glm\geometric.hpp>
#include <glm\ext\matrix_transform.hpp>
#include <glm\glm.hpp>
#include <imgui.h>

#include <algorithm>

#include <vk_wrapper\Tools.hpp>
#include <engine\ApplicationContext.hpp>



////////////////
constexpr bool C_COMPILE_HLSL_SOURCES_ON_START = true;
constexpr bool C_COMPILE_HLSL_PARALLEL = false; // don't turn on while HLSL infrastructure is finished
////////////////

//////////////////////////////////////////
DREApplicationDelegate::DREApplicationDelegate(HINSTANCE instance, char const* title, std::uint32_t windowWidth, std::uint32_t windowHeight, std::uint32_t buffering, bool vkDebug, bool imguiEnabled)
    : m_MainWindow {
        instance,
        title,
        windowWidth,
        windowHeight,
        "VulkanRenderWindow",
        DREApplicationDelegate::WinProc,
        this }
    , m_InputSystem{ m_MainWindow.NativeHandle() }
    , m_MaterialLibrary{ &DRE::g_MainAllocator }
    , m_GeometryLibrary{ &DRE::g_MainAllocator }
    , m_IOManager{ &m_MaterialLibrary, &m_GeometryLibrary }
    , m_ShaderModuleDB{ &m_IOManager }
    , m_GraphicsManager{ instance, &m_MainWindow, &m_IOManager, &m_ShaderModuleDB, vkDebug }
    , m_ImGuiEnabled{ imguiEnabled }
    , m_MainScene{ &DRE::g_MainAllocator }
    , m_RootEditor{ &m_MainScene }
    , m_ViewportInput{ &m_MainScene }
    , m_CameraMoveSpeed{ 25.0f }
{
    WORLD::g_MainScene = &m_MainScene;
}

//////////////////////////////////////////
DREApplicationDelegate::~DREApplicationDelegate()
{
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//////////////////////////////////////////
LRESULT DREApplicationDelegate::WinProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(handle, message, wparam, lparam))
        return true;

    auto* appDelegate = reinterpret_cast<DREApplicationDelegate*>(::GetWindowLongPtr(handle, GWLP_USERDATA));
    
    switch (message)
    {
    case WM_INPUT:
    {
        UINT code = GET_RAWINPUT_CODE_WPARAM(wparam);
        appDelegate->GetInputSystem().ProcessSystemInput(handle, wparam, lparam);

        LRESULT result{};
        if (code == RIM_INPUTSINK || code == RIM_INPUT)
            result = ::DefWindowProc(handle, message, wparam, lparam);

        return result;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        break;
    }
    
    return ::DefWindowProc(handle, message, wparam, lparam);
}

//////////////////////////////////////////
SYS::InputSystem& DREApplicationDelegate::GetInputSystem()
{
    return m_InputSystem;
}

WORLD::Scene& DREApplicationDelegate::GetMainScene()
{
    return m_MainScene;
}

IO::IOManager& DREApplicationDelegate::GetIOManager()
{
    return m_IOManager;
}

void DREApplicationDelegate::start()
{
    if (C_COMPILE_HLSL_SOURCES_ON_START)
    {  
        m_ShaderModuleDB.CompileSources(C_COMPILE_HLSL_PARALLEL);
    }
    
    m_GeometryLibrary.LoadDefaultGeometry();

    m_MainScene.GetMainCamera().SetFOV(60.0f);
    //m_MainScene.GetMainCamera().SetPosition(glm::vec3{ 7.28f, 5.57f, -1.07f });
    //m_MainScene.GetMainCamera().SetPosition(glm::vec3{ 0.0f, 0.0f, 11.0f });
    //m_MainScene.GetMainCamera().SetEulerOrientation(glm::vec3{ -17.26f, 107.37f, 0.0f });

    m_MainScene.GetMainCamera().SetPosition(glm::vec3{ -0.23f, 10.41f, 14.70f });
    m_MainScene.GetMainCamera().SetCameraEuler(glm::vec3{ -13.32f, -43.83f, 0.0f });

    WORLD::Light* sunLight = m_MainScene.CreateSunLight(m_GraphicsManager.GetMainContext());
    m_MainScene.SetMainSunLight(sunLight);

    sunLight->SetEulerOrientation(glm::vec3{ -70.0f, 110.0f, 0.0f });
    //sunLight->SetEulerOrientation(glm::vec3{ 0.0f, 0.0f, 0.0f });
    sunLight->ScheduleUpdateGPUData();


    if (m_ImGuiEnabled)
        InitImGui();

    m_GraphicsManager.PrecacheAllData(&m_ViewportInput, &m_GeometryLibrary);



    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    // WARNING: ParseModelFile will invoke LoadTexture2DSync which will reset the UploadArena
#if 1
    WORLD::SceneNode* sponzaNode = m_IOManager.ParseModelFile("data\\gltf_samples\\Sponza\\glTF\\Sponza.gltf", m_MainScene);
    sponzaNode->SetScale(5.0f);
#endif

#if 0
    WORLD::SceneNode* sponzaNode = m_IOManager.ParseModelFile("data\\downloadable\\main_sponza\\NewSponza_Main_glTF_003.gltf", m_MainScene);
    sponzaNode->SetScale(5.0f);
#endif

#if 0
    WORLD::SceneNode* palaceNode = m_IOManager.ParseModelFile("data\\downloadable\\mcguire_cornell\\CornellBox-Original.obj", m_MainScene);
    palaceNode->SetScale(2.0f);
#endif

#if 0
    WORLD::SceneNode* sponzaNode = m_IOManager.ParseModelFile("data\\downloadable\\mcguire_sponza\\sponza.obj", m_MainScene);
    sponzaNode->SetScale(0.1f);
#endif

#if 0
    WORLD::SceneNode* galleryNode = m_IOManager.ParseModelFile("data\\downloadable\\gallery\\gallery.obj", m_MainScene); // texture too big
    galleryNode->SetScale(0.1f);
#endif

#if 0
    WORLD::SceneNode* palaceNode = m_IOManager.ParseModelFile("data\\downloadable\\palace\\sibenik.obj", m_MainScene);
    palaceNode->SetScale(2.0f);
#endif


    glm::mat spheresTransform = glm::rotate(glm::identity<glm::mat4>(), glm::radians(180.0f), glm::vec3{ 1.0f, 0.0, 0.0f });
    WORLD::SceneNode* spheresNode = m_IOManager.ParseModelFile("data\\gltf_samples\\MetalRoughSpheres\\glTF\\MetalRoughSpheres.gltf", m_MainScene, spheresTransform);

    m_GraphicsManager.GetMainContext().FlushAll();

    ////////////
    Data::Texture2D blueNoise256 = m_IOManager.ReadTexture2D("textures\\blue_noise_rgba.png", Data::TEXTURE_CHANNELS_RGBA);
    m_GraphicsManager.GetTextureBank().LoadTexture2DSync("blue_noise_256", 256, 256, VKW::FORMAT_R8G8B8A8_UNORM, blueNoise256.GetBuffer());

    Data::Texture2D whiteNoise256 = m_IOManager.ReadTexture2D("textures\\white_noise.png", Data::TEXTURE_CHANNELS_GRAY);
    m_GraphicsManager.GetTextureBank().LoadTexture2DSync("white_noise_256", 256, 256, VKW::FORMAT_R8_UNORM, whiteNoise256.GetBuffer());

    ////////////
    m_GraphicsManager.BuildMainSceneTLAS();

    ////////////
    m_GraphicsManager.GetMainContext().FlushAll();
}

//////////////////////////////////////////
void DREApplicationDelegate::update()
{
    ////////////////////////////////////////////////////
    // Frame time
    DRE::g_AppContext.m_DeltaTimeUS = m_FrameStopwatch.CurrentMicroseconds();
    DRE::g_AppContext.m_TimeSinceStartUS = m_GlobalStopwatch.CurrentMicroseconds();
    DRE::g_AppContext.m_SystemTimeUS = DRE::Stopwatch::GlobalTimeMicroseconds();
    m_FrameStopwatch.Reset();
    ////////////////////////////////////////////////////

    DRE::g_FrameScratchAllocator.Reset();

    // Input maintenance
    m_InputSystem.Update();
    DRE::g_AppContext.m_CursorX = m_InputSystem.GetMouseState().mousePosX_;
    DRE::g_AppContext.m_CursorY = m_InputSystem.GetMouseState().mousePosY_;

    // ImGui
    if (m_ImGuiEnabled)
    {
        m_ImGuiHelper->BeginFrame();
        ImGuiUser();
        m_ImGuiHelper->EndFrame();
    }

    ProcessViewportInput();

    // Global stopwatch
    if (DRE::g_AppContext.m_PauseTime != m_GlobalStopwatch.IsPaused())
    {
        if (DRE::g_AppContext.m_PauseTime)
        {
            m_GlobalStopwatch.Pause();
        }
        else
        {
            m_GlobalStopwatch.Unpause();
        }
    }

    // Rendering
    m_GraphicsManager.RenderFrame(DRE::g_AppContext.m_EngineFrame, DRE::g_AppContext.m_DeltaTimeUS, m_GlobalStopwatch.CurrentSeconds());

    DRE::g_AppContext.m_EngineFrame++;
}

void DREApplicationDelegate::ProcessCameraInput()
{
    if (!m_InputSystem.GetRightMouseButtonPressed())
        return;

    auto const& mouseState = m_InputSystem.GetMouseState();

    if (mouseState.mouseWheelDelta_ != 0.0f)
    {
        constexpr float CAMERA_SPEED_STEP = 0.5f;
        m_CameraMoveSpeed = std::max(0.1f, m_CameraMoveSpeed + mouseState.mouseWheelDelta_ * CAMERA_SPEED_STEP);
    }

    WORLD::Camera& camera = m_MainScene.GetMainCamera();
    float const deltaSeconds = static_cast<double>(DRE::g_AppContext.m_DeltaTimeUS) / 1'000'000.0;

    // Camera rotation
    {
        constexpr float CAMERA_ROTATION_SPEED = 100.0f;
        glm::vec3 rotationDelta{ -mouseState.yDelta_ * CAMERA_ROTATION_SPEED, -mouseState.xDelta_ * CAMERA_ROTATION_SPEED, 0.0f };
        rotationDelta *= deltaSeconds;
        if (rotationDelta.x != 0.0f || rotationDelta.y != 0.0f)
        {
            camera.RotateCamera(rotationDelta);
        }
    }

    // Camera movement
    {
        glm::vec3 movement{ 0.0f, 0.0f, 0.0f };

        if (m_InputSystem.GetKeyboardButtonDown(Keys::W))
            movement += camera.GetForward();
        if (m_InputSystem.GetKeyboardButtonDown(Keys::S))
            movement -= camera.GetForward();
        if (m_InputSystem.GetKeyboardButtonDown(Keys::D))
            movement += camera.GetRight();
        if (m_InputSystem.GetKeyboardButtonDown(Keys::A))
            movement -= camera.GetRight();

        if (glm::length(movement) > 0.0f)
        {
            movement = glm::normalize(movement);
            camera.Move(movement * m_CameraMoveSpeed * deltaSeconds);
        }
    }
}

void DREApplicationDelegate::ProcessInputShortcuts()
{
    if (m_InputSystem.GetKeyboardButtonJustReleased(Keys::R))
    {
        if (m_InputSystem.GetKeyboardButtonDown(Keys::Shift))
        {
            ForceReloadAllShaders();
        }
        else if (m_ShaderModuleDB.AreNewShadersPending())
        {
            ReloadPendingShaders();
        }
    }

    if (m_InputSystem.GetKeyboardButtonJustPressed(Keys::Space))
    {
        DebugBreak();
    }
}

void DREApplicationDelegate::ReloadPendingShaders()
{
    std::cout << "Start reloading pending shaders." << std::endl;

    m_GraphicsManager.WaitIdle();

    auto fileNames = m_ShaderModuleDB.GetPendingShaderFilesCopy();
    for (DRE::U32 i = 0; i < fileNames.Size(); i++)
    {
        m_GraphicsManager.GetPipelineDB().ReloadShaderFilePipelines(fileNames[i].GetData());
    }
}

void DREApplicationDelegate::ForceReloadAllShaders()
{
    std::cout << "Start force reloading all shaders." << std::endl;

    m_GraphicsManager.WaitIdle();

    m_GraphicsManager.GetPipelineDB().ReloadAllPipelines();
    m_ShaderModuleDB.ClearPendingShaders();
}

//////////////////////////////////////////
void DREApplicationDelegate::shutdown()
{
    m_GraphicsManager.WaitIdle();
    m_GraphicsManager.GetTextureBank().UnloadAllTextures();
    m_GraphicsManager.GetMainRenderGraph().UnloadGraphResources();
    DestroyImGui();
}

//////////////////////////////////////////
void DREApplicationDelegate::InitImGui()
{
    // Window* window, InputSystem* input, VKW::Instance& instance, VKW::Swapchain& swapchain, VKW::Device& device, VKW::Context& context
    m_ImGuiHelper = std::make_unique<ImGuiHelper>(&m_MainWindow, &m_InputSystem, *m_GraphicsManager.GetInstance(), *m_GraphicsManager.GetSwapchain(), *m_GraphicsManager.GetMainDevice(), m_GraphicsManager.GetMainContext());
}

void DREApplicationDelegate::DestroyImGui()
{
    m_ImGuiHelper.reset();
}

//////////////////////////////////////////
void DREApplicationDelegate::ImGuiUser()
{
    IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!");
    m_RootEditor.Render();
}

void DREApplicationDelegate::ProcessViewportInput()
{
    ImGuiIO const& imguiIO = ImGui::GetIO();

    bool const allowMouseInput = !imguiIO.WantCaptureMouse;
    bool const allowKeyboardInput = !imguiIO.WantCaptureKeyboard;

    if (allowMouseInput)
    {
        m_ViewportInput.ProcessInput(m_InputSystem, m_GraphicsManager.GetMainRenderView());
    }

    if (allowMouseInput && allowKeyboardInput)
    {
        ProcessCameraInput();
    }

    if (allowKeyboardInput)
    {
        ProcessInputShortcuts();
    }
}

