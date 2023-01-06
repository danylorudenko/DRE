#include "VulkanApplicationDelegate.hpp"

#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\resources\Framebuffer.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <transform\TansformComponent.hpp>

#include <utility>
#include <cstdio>

#include <glm/geometric.hpp>

#include <imgui\imgui.h>

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
    , m_MaterialLibrary{}
    , m_IOManager{ &m_MaterialLibrary }
    , m_GraphicsManager{ instance, &m_MainWindow, vkDebug }
    , m_ImGuiEnabled{ imguiEnabled }
    , m_PrevFrameDeltaMicroseconds{ 0 }
    , m_AppStartTimeMicroseconds{ 0 }
    , m_EngineFrame{ 0 }
    , m_BulletsEveryFrame{ false }
{
    WORLD::g_MainScene = &m_MainScene;
    auto startPoint = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now());
    m_AppStartTimeMicroseconds = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(startPoint.time_since_epoch()).count());

    Assimp::Importer* testAssimp = new Assimp::Importer();
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

    m_GraphicsManager.Initialize();

    m_IOManager.ParseModelFile("LFS\\Sponza\\Sponza.gltf", m_MainScene);
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

    if (m_ImGuiEnabled)
    {
        m_ImGuiHelper->BeginFrame();
        ImGuiUser();
        m_ImGuiHelper->EndFrame();
    }


    m_InputSystem.Update();
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

        ImGui::SetNextWindowContentWidth(150.0f);
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

        float const cameraMod = (static_cast<float>(m_PrevFrameDeltaMicroseconds) / 10000);
        glm::vec3 cam_front{
                glm::sin(cameraEuler.y) * glm::cos(cameraEuler.x),
                glm::sin(cameraEuler.x),
                glm::cos(cameraEuler.y) * glm::cos(cameraEuler.x)
        };

        glm::vec3 cam_right{
                glm::sin(cameraEuler.y - 1.571f) * glm::cos(cameraEuler.x),
                0.0f,
                glm::cos(cameraEuler.y - 1.571f) * glm::cos(cameraEuler.x)
        };

        if (ImGui::Begin("Camera Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            if (ImGui::Button("Reset", ImVec2(50.0, 20.0f)))
            {
                camera.SetPosition(glm::vec3{ 0.0f, 0.0f, 0.0f });
                camera.SetEulerOrientation(glm::vec3{ 0.0f, 0.0f, 0.0f });
            }

            ImGui::PushButtonRepeat(true);

            if (ImGui::ArrowButton("fwd", ImGuiDir_Up))
                camera.Move(cam_front * 0.05f * cameraMod);
            ImGui::SameLine();
            if (ImGui::ArrowButton("left", ImGuiDir_Left))
                camera.Move(-cam_right * 0.05f * cameraMod);
            ImGui::SameLine(0.0f, 20.0f);
            if (ImGui::ArrowButton("upr", ImGuiDir_Up))
                camera.Rotate(glm::vec3{ -0.005f * cameraMod, 0.0f, 0.0f });
            ImGui::SameLine();
            if (ImGui::ArrowButton("leftr", ImGuiDir_Left))
                camera.Rotate(glm::vec3{ 0.0f, 0.005f * cameraMod, 0.0f });
            ImGui::SameLine();
            if(ImGui::Button("UP"))
                camera.Move(glm::vec3{ 0.0f, 0.05f * cameraMod, 0.0f });

            // new line
            if (ImGui::ArrowButton("back", ImGuiDir_Down))
                camera.Move(-cam_front * 0.05f * cameraMod);
            ImGui::SameLine();
            if (ImGui::ArrowButton("right", ImGuiDir_Right))
                camera.Move(cam_right * 0.05f * cameraMod);
            ImGui::SameLine(0.0f, 20.0f);
            if (ImGui::ArrowButton("downr", ImGuiDir_Down))
                camera.Rotate(glm::vec3{ 0.005f * cameraMod, 0.0f, 0.0f });
            ImGui::SameLine();
            if (ImGui::ArrowButton("rightr", ImGuiDir_Right))
                camera.Rotate(glm::vec3{ 0.0f, -0.005f * cameraMod, 0.0f });
            ImGui::SameLine();
            if (ImGui::Button("DOWN"))
                camera.Move(glm::vec3{ 0.0f, -0.05f * cameraMod, 0.0f });

            ImGui::PopButtonRepeat();

            ImGui::Text("Camera pos: %.2f, %.2f, %.2f", camera.GetPosition()[0], camera.GetPosition(), camera.GetPosition()[2]);
            ImGui::Text("Camera rot: %.2f, %.2f, %.2f", camera.GetEulerOrientation()[0], camera.GetEulerOrientation()[1], camera.GetEulerOrientation()[2]);
            ImGui::End();
        }
    }
}
