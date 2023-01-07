#include <DemoApp\ImGuiHelper.hpp>

#include <foundation\input\InputSystem.hpp>
#include <foundation\system\Window.hpp>
#include <foundation\memory\ByteBuffer.hpp>
#include <foundation\memory\Pointer.hpp>
#include <foundation\Common.hpp>

#include <engine\io\IOManager.hpp>

#include <glm\vec2.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <Windows.h>

#include <vk_wrapper\Context.hpp>

//#include <gfx\GraphicsManager.hpp>

ImGuiHelper::ImGuiHelper()
    : m_TargetWindow{ nullptr }
    , m_InputSystem{ nullptr }
{
}

ImGuiHelper::ImGuiHelper(Window* window, InputSystem* input)
    : m_TargetWindow{ window }
    , m_InputSystem{ input }
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = static_cast<float>(window->Width());
    io.DisplaySize.y = static_cast<float>(window->Height());

    io.ConfigFlags = ImGuiConfigFlags_None;
    //io.FontGlobalScale = 2.5f;
    io.IniFilename = nullptr;

    io.KeyMap[ImGuiKey_Tab] = (std::int32_t)Keys::Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = (std::int32_t)Keys::Left;
    io.KeyMap[ImGuiKey_RightArrow] = (std::int32_t)Keys::Right;
    io.KeyMap[ImGuiKey_UpArrow] = (std::int32_t)Keys::Up;
    io.KeyMap[ImGuiKey_DownArrow] = (std::int32_t)Keys::Down;
    io.KeyMap[ImGuiKey_PageUp] = (std::int32_t)Keys::PageUp;
    io.KeyMap[ImGuiKey_PageDown] = (std::int32_t)Keys::PageDown;
    io.KeyMap[ImGuiKey_Home] = (std::int32_t)Keys::Home;
    io.KeyMap[ImGuiKey_End] = (std::int32_t)Keys::End;
    io.KeyMap[ImGuiKey_Insert] = (std::int32_t)Keys::Insert;
    io.KeyMap[ImGuiKey_Delete] = (std::int32_t)Keys::Delete;
    io.KeyMap[ImGuiKey_Backspace] = (std::int32_t)Keys::Backspace;
    io.KeyMap[ImGuiKey_Space] = (std::int32_t)Keys::Space;
    io.KeyMap[ImGuiKey_Enter] = (std::int32_t)Keys::Enter;
    io.KeyMap[ImGuiKey_Escape] = (std::int32_t)Keys::Escape;
    io.KeyMap[ImGuiKey_A] = (std::int32_t)Keys::A;         // for text edit CTRL+A: select all
    io.KeyMap[ImGuiKey_C] = (std::int32_t)Keys::C;         // for text edit CTRL+C: copy
    io.KeyMap[ImGuiKey_V] = (std::int32_t)Keys::V;         // for text edit CTRL+V: paste
    io.KeyMap[ImGuiKey_X] = (std::int32_t)Keys::X;         // for text edit CTRL+X: cut
    io.KeyMap[ImGuiKey_Y] = (std::int32_t)Keys::Y;         // for text edit CTRL+Y: redo
    io.KeyMap[ImGuiKey_Z] = (std::int32_t)Keys::Z;
}

ImGuiHelper::ImGuiHelper(ImGuiHelper&& rhs)
    : m_TargetWindow{ nullptr }
    , m_InputSystem{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

ImGuiHelper& ImGuiHelper::operator=(ImGuiHelper&& rhs)
{
    DRE_SWAP_MEMBER(m_TargetWindow);
    DRE_SWAP_MEMBER(m_InputSystem);
    return *this;
}

ImGuiHelper::~ImGuiHelper()
{
    // hmmmmmmmm
}

void ImGuiHelper::BeginFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    HWND activeWindow = ::GetForegroundWindow();
    if (activeWindow == m_TargetWindow->NativeHandle()) {
        POINT pos;
        if (::GetCursorPos(&pos) && ::ScreenToClient(activeWindow, &pos)) {
            io.MousePos = ImVec2((float)pos.x, (float)pos.y);
        }
    }

    io.MouseDown[0] = m_InputSystem->GetLeftMouseButtonPressed();
    io.MouseDown[1] = m_InputSystem->GetRightMouseButtonPressed();
    io.MouseDown[2] = m_InputSystem->GetMiddleMouseButtonPressed();

    io.KeyCtrl = m_InputSystem->GetKeyboardButtonDown(Keys::Ctrl);
    io.KeyShift = m_InputSystem->GetKeyboardButtonDown(Keys::Shift);
    io.KeyAlt = m_InputSystem->GetKeyboardButtonDown(Keys::Alt);
    io.KeySuper = m_InputSystem->GetKeyboardButtonDown(Keys::WinLeft) || m_InputSystem->GetKeyboardButtonDown(Keys::WinRight);

    std::int32_t keyBegin = (std::int32_t)Keys::BEGIN;
    std::int32_t keyEnd = (std::int32_t)Keys::END;
    for (std::int32_t key = keyBegin; key < keyEnd; ++key) {
        io.KeysDown[key] = m_InputSystem->GetKeyboardButtonDown((Keys)key);

        if (m_InputSystem->GetKeyboardButtonJustReleased((Keys)key)) {
            char result = (char)InputSystem::GetCharFromKeys((Keys)key);
            m_InputSystem->GetKeyboardButtonJustReleased((Keys)key);
            if(result)
                io.AddInputCharacter(result);
        }
    }

    ImGui::NewFrame();
}

void ImGuiHelper::EndFrame()
{
    ImGui::EndFrame();
}


