#pragma once

#include <memory>
#include <future>

#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorBuddy.hpp>
#include <foundation\system\Time.hpp>

#include <demo_app\Application.hpp>
#include <demo_app\ImGuiHelper.hpp>

#include <vk_wrapper\Context.hpp>

#include <gfx\GraphicsManager.hpp>

#include <foundation\system\Window.hpp>
#include <foundation\system\DynamicLibrary.hpp>
#include <foundation\input\InputSystem.hpp>

#include <engine\scene\Scene.hpp>
#include <engine\data\MaterialLibrary.hpp>
#include <engine\data\GeometryLibrary.hpp>

#include <engine\io\IOManager.hpp>

class VulkanApplicationDelegate
    : public Application::ApplicationDelegate
    , public NonMovable
{
public:
    VulkanApplicationDelegate(HINSTANCE instance, char const* title, std::uint32_t windowWidth, std::uint32_t windowHeight, std::uint32_t buffering, bool vkDebug, bool imguiEnabled);
    
    virtual void start() override;
    virtual void update() override;
    virtual void shutdown() override;

    virtual ~VulkanApplicationDelegate();

    static LRESULT CALLBACK WinProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);

    InputSystem& GetInputSystem();

    WORLD::Scene& GetMainScene();

private:
    void InitImGui();
    void ImGuiUser();

private:
    Window                              m_MainWindow;
    InputSystem                         m_InputSystem;
    Data::MaterialLibrary               m_MaterialLibrary;
    Data::GeometryLibrary               m_GeometryLibrary;
    IO::IOManager                       m_IOManager;

    Data::Geometry                      m_WaterGeometry;
    Data::Material                      m_WaterMaterial;
    Data::Material                      m_BeachMaterial;

    GFX::GraphicsManager                m_GraphicsManager;

    std::unique_ptr<ImGuiHelper>        m_ImGuiHelper;
    bool m_ImGuiEnabled;

    DRE::Stopwatch  m_FrameStopwatch;
    DRE::Stopwatch  m_GlobalStopwatch;
    std::uint64_t m_DeltaMicroseconds;
    std::uint64_t m_EngineFrame;


    WORLD::Scene                        m_MainScene;
    bool                                m_RotateSun;
    float                               m_SunElevation;
    bool                                m_RotateCamera;
    bool                                m_PauseTime;
    float                               m_TimeOffset;
};
