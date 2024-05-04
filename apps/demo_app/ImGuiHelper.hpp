#pragma once

#include <imgui.h>

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\container\InplaceVector.hpp>

#include <vk_wrapper\Surface.hpp>

class Window;
class InputSystem;

namespace VKW
{
class Instance;
class Device;
class Context;
class Swapchain;
}

namespace GFX
{
class Device;
}

class ImGuiHelper : public NonCopyable
{
public:
    ImGuiHelper();
    ImGuiHelper(Window* window, InputSystem* input, VKW::Instance& instance, VKW::Swapchain& swapchain, VKW::Device& device, VKW::Context& context);
    ImGuiHelper(ImGuiHelper&& rhs);

    ImGuiHelper& operator=(ImGuiHelper&& rhs);

    ~ImGuiHelper();

    void BeginFrame();
    void DrawFrame(VKW::Context& context);
    void EndFrame();

    VKW::Surface* CreateCustomSurface(void* hwnd);
    void DestroyCustomSurface();

private:
    Window*         m_TargetWindow;
    InputSystem*    m_InputSystem;
    
    DRE::InplaceVector<VKW::Surface, 8> m_ImGuiSurfaces;
};
