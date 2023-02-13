#include "VulkanApplicationDelegate.hpp"

#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\resources\Framebuffer.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <engine\transform\TransformComponent.hpp>

#include <utility>
#include <cstdio>

#include <glm/geometric.hpp>

#include <imgui.h>

#include <assimp\Importer.hpp>

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
    , m_PrevFrameDeltaMicroseconds{ 0 }
    , m_AppStartTimeMicroseconds{ 0 }
    , m_EngineFrame{ 0 }
    , m_MainScene{ &DRE::g_MainAllocator }
    , m_BulletsEveryFrame{ false }
{
    WORLD::g_MainScene = &m_MainScene;

    auto startPoint = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now());
    m_AppStartTimeMicroseconds = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(startPoint.time_since_epoch()).count());
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

    m_IOManager.LoadShaderFiles();

    //m_GraphicsManager.GetMainRenderView() = GFX::RenderView{
    //    &DRE::g_FrameScratchAllocator,
    //    glm::uvec2{ 0, 0 }, glm::uvec2{ m_GraphicsManager.GetMainDevice()->GetSwapchain()->GetWidth(), m_GraphicsManager.GetMainDevice()->GetSwapchain()->GetHeight() },
    //    m_MainScene.GetMainCamera().GetPosition(), glm::vec3{1.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f}, m_MainScene.GetMainCamera().GetFOV()};

    m_MainScene.GetMainCamera().SetAspect(float(m_GraphicsManager.GetMainDevice()->GetSwapchain()->GetWidth()) / float(m_GraphicsManager.GetMainDevice()->GetSwapchain()->GetHeight()));
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
    auto currTime = std::chrono::high_resolution_clock::now();
    auto frameTime = currTime - m_PrevFrameTimePoint;
    m_PrevFrameTimePoint = currTime;
    m_PrevFrameDeltaMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(frameTime).count();
    ////////////////////////////////////////////////////

    DRE::g_FrameScratchAllocator.Reset();
    m_GraphicsManager.GetMainRenderView().Reset();

    m_InputSystem.Update();

    if (m_ImGuiEnabled)
    {
        m_ImGuiHelper->BeginFrame();
        ImGuiUser();
        m_ImGuiHelper->EndFrame();
    }

    m_GraphicsManager.RenderFrame(m_EngineFrame, m_PrevFrameDeltaMicroseconds);

    m_EngineFrame++;
}

//////////////////////////////////////////
void VulkanApplicationDelegate::shutdown()
{
    if (m_BulletAddTasks.Size() > 0)
    {
        for (std::uint8_t i = 0; i < m_BulletAddTasks.Size(); i++)
        {
            m_BulletAddTasks[i].wait();
        }
        m_BulletAddTasks.Clear();
    }
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
            ImGui::Text("DT: %f ms", static_cast<double>(m_PrevFrameDeltaMicroseconds) / 1000);
            ImGui::Text("FPS: %f", 1.0 / (static_cast<double>(m_PrevFrameDeltaMicroseconds) / 1000000));
            ImGui::End();
        }

        ImGui::SetNextWindowPos(ImVec2(0.0f, 100.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);

        WORLD::Camera& camera = m_MainScene.GetMainCamera();
        glm::vec3 const& cameraEuler = camera.GetEulerOrientation();
        glm::vec3 const& cameraForward = camera.GetForward();
        glm::vec3 const& cameraRight = camera.GetRight();

        float const cameraMod = (static_cast<float>(m_PrevFrameDeltaMicroseconds) / 10000);
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
