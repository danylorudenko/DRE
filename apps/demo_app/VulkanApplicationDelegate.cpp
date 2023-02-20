#include "VulkanApplicationDelegate.hpp"

#include <assimp\Importer.hpp>
#include <glm\geometric.hpp>
#include <imgui.h>

#include <utility>
#include <cstdio>

#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\resources\Framebuffer.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>


////////////////
constexpr bool C_COMPILE_GLSL_SOURCES_ON_START = true;
////////////////

//////////////////////////////////////////
VulkanApplicationDelegate::VulkanApplicationDelegate(HINSTANCE instance, char const* title, std::uint32_t windowWidth, std::uint32_t windowHeight, std::uint32_t buffering, bool vkDebug, bool imguiEnabled)
    : m_MainWindow {
        instance,
        title,
        windowWidth,
        windowHeight,
        "VulkanRenderWindow",
        VulkanApplicationDelegate::WinProc,
        this }
    , m_InputSystem{ m_MainWindow.NativeHandle() }
    , m_MaterialLibrary{ &DRE::g_MainAllocator }
    , m_GeometryLibrary{ &DRE::g_MainAllocator }
    , m_IOManager{ &DRE::g_MainAllocator, &m_MaterialLibrary, &m_GeometryLibrary }
    , m_GraphicsManager{ instance, &m_MainWindow, &m_IOManager, vkDebug }
    , m_ImGuiEnabled{ imguiEnabled }
    , m_DeltaMicroseconds{ 0 }
    , m_EngineFrame{ 0 }
    , m_MainScene{ &DRE::g_MainAllocator }
{
    WORLD::g_MainScene = &m_MainScene;
}

//////////////////////////////////////////
VulkanApplicationDelegate::~VulkanApplicationDelegate()
{

}

//////////////////////////////////////////
LRESULT VulkanApplicationDelegate::WinProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* appDelegate = reinterpret_cast<VulkanApplicationDelegate*>(::GetWindowLongPtr(handle, GWLP_USERDATA));
    
    switch (message)
    {
    case WM_INPUT:
    {
        UINT code = GET_RAWINPUT_CODE_WPARAM(wparam);
        LRESULT result{};
        if (code == RIM_INPUTSINK || code == RIM_INPUT)
            result = ::DefWindowProc(handle, message, wparam, lparam);

        appDelegate->GetInputSystem().ProcessSystemInput(handle, wparam, lparam);
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
InputSystem& VulkanApplicationDelegate::GetInputSystem()
{
    return m_InputSystem;
}

WORLD::Scene& VulkanApplicationDelegate::GetMainScene()
{
    return m_MainScene;
}

void VulkanApplicationDelegate::start()
{
    if (m_ImGuiEnabled)
        InitImGui();

    if (C_COMPILE_GLSL_SOURCES_ON_START)
    {
        m_IOManager.CompileGLSLSources();
    }
    m_IOManager.LoadShaderBinaries();

    m_GraphicsManager.GetMainRenderView() = GFX::RenderView{ &DRE::g_MainAllocator };

    m_MainScene.GetMainCamera().SetFOV(60.0f);
    m_MainScene.GetMainCamera().SetPosition(glm::vec3{ 7.28f, 5.57f, -1.07f });
    m_MainScene.GetMainCamera().SetEulerOrientation(glm::vec3{ -17.26f, 107.37f, 0.0f });

    m_GraphicsManager.Initialize();

    m_IOManager.ParseModelFile("data\\Sponza\\glTF\\Sponza.gltf", m_MainScene);
}

//////////////////////////////////////////
void VulkanApplicationDelegate::update()
{
    ////////////////////////////////////////////////////
    // Frame time measurement
    m_DeltaMicroseconds = m_FrameStopwatch.CurrentMicroseconds();
    m_FrameStopwatch.Reset();
    ////////////////////////////////////////////////////

    DRE::g_FrameScratchAllocator.Reset();

    m_InputSystem.Update();

    if (m_ImGuiEnabled)
    {
        m_ImGuiHelper->BeginFrame();
        ImGuiUser();
        m_ImGuiHelper->EndFrame();
    }

    if (m_InputSystem.GetKeyboardButtonJustReleased(Keys::R))
    {
        if (m_IOManager.NewShadersPending())
        {
            m_GraphicsManager.WaitIdle();
            m_GraphicsManager.ReloadShaders();
        }
    }
    m_GraphicsManager.RenderFrame(m_EngineFrame, m_DeltaMicroseconds);

    m_EngineFrame++;
}

//////////////////////////////////////////
void VulkanApplicationDelegate::shutdown()
{
}

//////////////////////////////////////////
void VulkanApplicationDelegate::InitImGui()
{
    m_ImGuiHelper = std::make_unique<ImGuiHelper>(&m_MainWindow, &m_InputSystem);
}

//////////////////////////////////////////
void VulkanApplicationDelegate::ImGuiUser()
{
    if (m_ImGuiEnabled) {
        IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!");

        ImGui::SetNextWindowContentSize(ImVec2{ 150.0f, 0.0f});
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);

        ImGuiWindowFlags frameDataWindowFlags = 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoCollapse;
        static bool frameDataOpened = false;
        if (ImGui::Begin("Frame Stats", nullptr, frameDataWindowFlags))
        {
            ImGui::Text("DT: %f ms", static_cast<double>(m_DeltaMicroseconds) / 1000);
            ImGui::Text("FPS: %f", 1.0 / (static_cast<double>(m_DeltaMicroseconds) / 1000000));
            ImGui::End();
        }

        ImGui::SetNextWindowPos(ImVec2(0.0f, 100.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);

        WORLD::Camera& camera = m_MainScene.GetMainCamera();
        glm::vec3 const& cameraEuler = camera.GetEulerOrientation();
        glm::vec3 const& cameraForward = camera.GetForward();
        glm::vec3 const& cameraRight = camera.GetRight();

        float const cameraMod = (static_cast<float>(m_DeltaMicroseconds) / 10000);
        float const moveMul = 0.1f;
        float const rotMul = 1.0f;

        if (ImGui::Begin("Camera Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            if (ImGui::Button("Reset", ImVec2(50.0, 20.0f)))
            {
                camera.SetPosition(glm::vec3{ 0.0f, 0.0f, 0.0f });
                camera.SetEulerOrientation(glm::vec3{ 0.0f, 0.0f, 0.0f });
            }

            ImGui::PushButtonRepeat(true);

            if (ImGui::ArrowButton("fwd", ImGuiDir_Up))
                camera.Move(cameraForward * moveMul * cameraMod);
            ImGui::SameLine();
            if (ImGui::ArrowButton("left", ImGuiDir_Left))
                camera.Move(-cameraRight * moveMul * cameraMod);
            ImGui::SameLine(0.0f, 20.0f);
            if (ImGui::ArrowButton("upr", ImGuiDir_Up))
                camera.Rotate(glm::vec3{ rotMul * cameraMod, 0.0f, 0.0f });
            ImGui::SameLine();
            if (ImGui::ArrowButton("leftr", ImGuiDir_Left))
                camera.Rotate(glm::vec3{ 0.0f, rotMul * cameraMod, 0.0f });
            ImGui::SameLine();
            if(ImGui::Button("UP"))
                camera.Move(glm::vec3{ 0.0f, moveMul * cameraMod, 0.0f });

            // new line
            if (ImGui::ArrowButton("back", ImGuiDir_Down))
                camera.Move(-cameraForward * moveMul * cameraMod);
            ImGui::SameLine();
            if (ImGui::ArrowButton("right", ImGuiDir_Right))
                camera.Move(cameraRight * moveMul * cameraMod);
            ImGui::SameLine(0.0f, 20.0f);
            if (ImGui::ArrowButton("downr", ImGuiDir_Down))
                camera.Rotate(glm::vec3{ -rotMul * cameraMod, 0.0f, 0.0f });
            ImGui::SameLine();
            if (ImGui::ArrowButton("rightr", ImGuiDir_Right))
                camera.Rotate(glm::vec3{ 0.0f, -rotMul * cameraMod, 0.0f });
            ImGui::SameLine();
            if (ImGui::Button("DOWN"))
                camera.Move(glm::vec3{ 0.0f, -moveMul * cameraMod, 0.0f });

            ImGui::PopButtonRepeat();

            ImGui::Text("Camera pos: %.2f, %.2f, %.2f", camera.GetPosition()[0], camera.GetPosition()[1], camera.GetPosition()[2]);
            ImGui::Text("Camera rot: %.2f, %.2f, %.2f", camera.GetEulerOrientation()[0], camera.GetEulerOrientation()[1], camera.GetEulerOrientation()[2]);
            ImGui::End();
        }
    }
}
