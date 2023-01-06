#pragma once

#include <imgui\imgui.h>

#include <foundation\class_features\NonCopyable.hpp>

#include <vk_wrapper\pipeline\RenderPass.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>
#include <vk_wrapper\resources\Framebuffer.hpp>

#include <gfx\texture\ReadOnlyTexture.hpp>

class Window;
class InputSystem;

namespace VKW
{
class Context;
}

namespace GFX
{
class Device;
}

class ImGuiHelper : public NonCopyable
{
public:
    ImGuiHelper();
    ImGuiHelper(Window* window, InputSystem* input);
    ImGuiHelper(ImGuiHelper&& rhs);

    ImGuiHelper& operator=(ImGuiHelper&& rhs);

    ~ImGuiHelper();

    void BeginFrame();
    void EndFrame();

private:
    Window*         m_TargetWindow;
    InputSystem*    m_InputSystem;
    
};