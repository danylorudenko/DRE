#pragma once

#include <memory>
#include <chrono>
#include <future>

#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorBuddy.hpp>

#include <DemoApp\Application.hpp>
#include <DemoApp\ImGuiHelper.hpp>

#include <vk_wrapper\Context.hpp>

#include <gfx\GraphicsManager.hpp>

#include <foundation\system\Window.hpp>
#include <foundation\system\DynamicLibrary.hpp>
#include <foundation\input\InputSystem.hpp>

#include <engine\transform\TransformationSystem.hpp>

#include <engine\scene\Scene.hpp>
#include <engine\data\MaterialLibrary.hpp>

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
    IO::IOManager                       m_IOManager;

    GFX::GraphicsManager                m_GraphicsManager;

    std::unique_ptr<ImGuiHelper>        m_ImGuiHelper;
    bool m_ImGuiEnabled;

    std::chrono::high_resolution_clock::time_point m_PrevFrameTimePoint;
    std::uint64_t m_PrevFrameDeltaMicroseconds;
    std::uint64_t m_AppStartTimeMicroseconds;
    std::uint64_t m_EngineFrame;


    WORLD::Scene                        m_MainScene;


    // TESTS SECTION
    static std::uint8_t constexpr       MAX_TASKS = 4;
    DRE::InplaceVector<std::future<void>, MAX_TASKS> m_BulletAddTasks;

    bool                                m_BulletsEveryFrame;
};
