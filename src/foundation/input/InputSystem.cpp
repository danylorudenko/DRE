#include <foundation\input\InputSystem.hpp>

#include <foundation\memory\Memory.hpp>

#include <utility>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace SYS
{

#ifdef DRE_DEBUG
InputSystem* g_InputSystem = nullptr;
#endif

InputSystem::InputSystem()
    : pendingMouseState_{}
    , mouseState_{}
    , prevMouseState_{}
    , pendingKeyboardState_{}
    , prevKeyboardState_{}
    , keyboardState_{}
{
    DRE::MemZero(&pendingMouseState_, sizeof(pendingMouseState_));
    DRE::MemZero(&mouseState_, sizeof(mouseState_));
    DRE::MemZero(&prevMouseState_, sizeof(prevMouseState_));

    DRE::MemZero(&pendingKeyboardState_, sizeof(pendingKeyboardState_));
    DRE::MemZero(&prevKeyboardState_, sizeof(prevKeyboardState_));
    DRE::MemZero(&keyboardState_, sizeof(keyboardState_));
}

InputSystem::InputSystem(HWND windowHandle)
    : pendingMouseState_{}
    , mouseState_{}
    , prevMouseState_{}
    , pendingKeyboardState_{}
    , prevKeyboardState_{}
    , keyboardState_{}
{
    DRE::MemZero(&pendingMouseState_, sizeof(pendingMouseState_));
    DRE::MemZero(&mouseState_, sizeof(mouseState_));
    DRE::MemZero(&prevMouseState_, sizeof(prevMouseState_));

    DRE::MemZero(&pendingKeyboardState_, sizeof(pendingKeyboardState_));
    DRE::MemZero(&prevKeyboardState_, sizeof(prevKeyboardState_));
    DRE::MemZero(&keyboardState_, sizeof(keyboardState_));

    UINT inputDeviceCount = 0;
    {
        UINT err = GetRawInputDeviceList(NULL, &inputDeviceCount, sizeof(RAWINPUTDEVICELIST));

        if (err == (UINT)-1) {
            DWORD lastError = GetLastError();
            std::cerr << "InputSystem: failed to retrieve RAWINPUTDEVICELIST(0). Error code: " << lastError << std::endl;
            return;
        }
    }

    RAWINPUTDEVICELIST* list = NULL;
    if (inputDeviceCount > 0) {
        list = (RAWINPUTDEVICELIST*)DRE::g_FrameScratchAllocator.Alloc(sizeof(RAWINPUTDEVICELIST) * inputDeviceCount, alignof(RAWINPUTDEVICELIST));
        {
            UINT err = GetRawInputDeviceList(list, &inputDeviceCount, sizeof(RAWINPUTDEVICELIST));
            if (err == (UINT)-1) {
                DWORD lastError = GetLastError();
                std::cerr << "InputSystem: failed to retrieve RAWINPUTDEVICELIST(1). Error code: " << lastError << std::endl;
                return;
            }
        }
    }

    bool keyboardConnected = false;
    bool mouseConnected = false;

    for (std::uint32_t i = 0; i < inputDeviceCount; ++i) {
        if (list[i].dwType == RIM_TYPEMOUSE) {
            mouseConnected = true;
        }
        else if (list[i].dwType == RIM_TYPEKEYBOARD) {
            keyboardConnected = true;
        }
    }

    UINT rawDevicesCount = 0;
    RAWINPUTDEVICE rawDevices[2];
    if (mouseConnected) {
        rawDevices[rawDevicesCount].usUsagePage = 1;
        rawDevices[rawDevicesCount].usUsage = 2;
        rawDevices[rawDevicesCount].dwFlags = 0;
        rawDevices[rawDevicesCount].hwndTarget = windowHandle; // TODO: let's check how that works

        rawDevicesCount += 1;
    }

    if (keyboardConnected) {
        rawDevices[rawDevicesCount].usUsagePage = 1;
        rawDevices[rawDevicesCount].usUsage = 6;
        rawDevices[rawDevicesCount].dwFlags = 0;
        rawDevices[rawDevicesCount].hwndTarget = windowHandle; // TODO: let's check how that works

        rawDevicesCount += 1;
    }

    if (RegisterRawInputDevices(rawDevices, rawDevicesCount, sizeof(*rawDevices)) != TRUE) {
        UINT err = GetLastError();
        std::cerr << "Input System: Failed to register raw input devices. Error code: " << err << std::endl;
    }

    g_InputSystem = this;
}

InputSystem::InputSystem(InputSystem&& rhs)
{
    operator=(std::move(rhs));
}

InputSystem& InputSystem::operator=(InputSystem&& rhs)
{
    return *this;
}

InputSystem::~InputSystem()
{

}

InputSystem::MouseState const& InputSystem::GetMouseState() const
{
    return mouseState_;
}

bool InputSystem::GetLeftMouseButtonPressed() const
{
    return mouseState_.mouseButtonStates_ & 1 << (int)MouseState::Left;
}

bool InputSystem::GetRightMouseButtonPressed() const
{
    return mouseState_.mouseButtonStates_ & 1 << (int)MouseState::Right;
}

bool InputSystem::GetMiddleMouseButtonPressed() const
{
    return mouseState_.mouseButtonStates_ & 1 << (int)MouseState::Middle;
}

bool InputSystem::GetLeftMouseButtonJustPressed() const
{
    bool prevValue = prevMouseState_.mouseButtonStates_ & 1 << (int)MouseState::Left;
    return !prevValue && GetLeftMouseButtonPressed();
}

bool InputSystem::GetRightMouseButtonJustPressed() const
{
    bool prevValue = prevMouseState_.mouseButtonStates_ & 1 << (int)MouseState::Right;
    return !prevValue && GetRightMouseButtonPressed();
}

bool InputSystem::GetMiddleMouseButtonJustPressed() const
{
    bool prevValue = prevMouseState_.mouseButtonStates_ & 1 << (int)MouseState::Middle;
    return !prevValue && GetMiddleMouseButtonPressed();
}

bool InputSystem::GetLeftMouseButtonJustReleased() const
{
    bool prevValue = prevMouseState_.mouseButtonStates_ & 1 << (int)MouseState::Left;
    return prevValue && !GetLeftMouseButtonPressed();
}

bool InputSystem::GetRightMouseButtonJustReleased() const
{
    bool prevValue = prevMouseState_.mouseButtonStates_ & 1 << (int)MouseState::Right;
    return prevValue && !GetRightMouseButtonPressed();
}

bool InputSystem::GetMiddleMouseButtonJustReleased() const
{
    bool prevValue = prevMouseState_.mouseButtonStates_ & 1 << (int)MouseState::Middle;
    return prevValue && !GetMiddleMouseButtonPressed();
}

bool InputSystem::GetKeyboardButtonDown(Keys key) const
{
    return GetKeysBitflagValue(keyboardState_.keysBits, key);
}

bool InputSystem::GetKeyboardButtonJustPressed(Keys key) const
{
    bool prevValue = GetKeysBitflagValue(prevKeyboardState_.keysBits, key);
    return !prevValue && GetKeyboardButtonDown(key);
}

bool InputSystem::GetKeyboardButtonJustReleased(Keys key) const
{
    bool prevValue = GetKeysBitflagValue(prevKeyboardState_.keysBits, key);
    return prevValue && !GetKeyboardButtonDown(key);
}

void InputSystem::SetKeysBitflagValue(std::uint64_t* bitflag, Keys key, bool value)
{
    std::uint32_t member = (std::uint32_t)key / 64;
    std::uint32_t bitOffset = (std::uint32_t)key - 64 * member;

    if (value) {
        bitflag[member] |= 1ULL << bitOffset;
    }
    else {
        bitflag[member] &= ~(1ULL << bitOffset);
    }

}

bool InputSystem::GetKeysBitflagValue(std::uint64_t const* bitflag, Keys key)
{
    std::uint32_t member = (std::uint32_t)key / 64;
    std::uint32_t bitOffset = (std::uint32_t)key - 64 * member;

    return bitflag[member] & (1ULL << bitOffset);
}

std::uint32_t InputSystem::GetCharFromKeys(Keys key)
{
    return MapVirtualKeyW(KeysToVKey(key), MAPVK_VK_TO_CHAR);
}

void InputSystem::Update()
{
    pendingMouseState_.mouseWheelDelta_ = static_cast<float>(pendingMouseState_.mouseWheelPos_ - mouseState_.mouseWheelPos_);

    prevMouseState_ = mouseState_;
    mouseState_ = pendingMouseState_;

    prevKeyboardState_ = keyboardState_;
    keyboardState_ = pendingKeyboardState_;
}

void InputSystem::ProcessSystemInput(HWND handle, WPARAM wparam, LPARAM lparam)
{
    UINT dataSize = 0;

    UINT result = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));
    if (result != 0) {
        DWORD err = GetLastError();
        std::cerr <<
            "InputSystem::ProcessSystemInput: GetRawInputData failed. Can't retrieve system input. "
            "Error code: " << err << std::endl;
        return;
    }

    void* data = DRE::g_FrameScratchAllocator.Alloc(dataSize, alignof(RAWINPUT));
    result = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, data, &dataSize, sizeof(RAWINPUTHEADER));
    if (result < 0 || result != dataSize) {
        DWORD err = GetLastError();
        std::cerr <<
            "InputSystem::ProcessSystemInput: GetRawInputData failed. Can't retrieve system input. "
            "Error code: " << err << std::endl;
    }

    RAWINPUT* rawInput = (RAWINPUT*)data;
    RAWINPUTHEADER& header = rawInput->header;
    if (header.dwType == RIM_TYPEMOUSE) {
        RAWMOUSE& mouse = rawInput->data.mouse;

        pendingMouseState_.xDelta_ = static_cast<float>(mouse.lLastX);
        pendingMouseState_.yDelta_ = static_cast<float>(mouse.lLastY);

        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
            pendingMouseState_.mouseButtonStates_ |= 1 << static_cast<std::uint32_t>(MouseState::MouseButtonOffsets::Left);

        if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
            pendingMouseState_.mouseButtonStates_ |= 1 << static_cast<std::uint32_t>(MouseState::MouseButtonOffsets::Right);

        if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
            pendingMouseState_.mouseButtonStates_ |= 1 << static_cast<std::uint32_t>(MouseState::MouseButtonOffsets::Middle);

        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
            pendingMouseState_.mouseButtonStates_ &= ~(1 << static_cast<std::uint32_t>(MouseState::MouseButtonOffsets::Left));

        if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
            pendingMouseState_.mouseButtonStates_ &= ~(1 << static_cast<std::uint32_t>(MouseState::MouseButtonOffsets::Right));

        if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
            pendingMouseState_.mouseButtonStates_ &= ~(1 << static_cast<std::uint32_t>(MouseState::MouseButtonOffsets::Middle));

        if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
            pendingMouseState_.mouseWheelPos_ += static_cast<short>(mouse.usButtonData) / WHEEL_DELTA;
    }
    else if (header.dwType == RIM_TYPEKEYBOARD) {
        RAWKEYBOARD& keyboard = rawInput->data.keyboard;
        if (keyboard.Message == WM_KEYUP || keyboard.Message == WM_SYSKEYUP) {
            // up
            SetKeysBitflagValue(pendingKeyboardState_.keysBits, VKeyToKeys(keyboard.VKey), false);
        }
        if (keyboard.Message == WM_KEYDOWN || keyboard.Message == WM_SYSKEYDOWN) {
            // down
            SetKeysBitflagValue(pendingKeyboardState_.keysBits, VKeyToKeys(keyboard.VKey), true);
        }
    }


    POINT mouseScreenPos;
    bool hasScreenPos = ::GetCursorPos(&mouseScreenPos) != 0;

    if (hasScreenPos)
    {
        POINT mousePos = mouseScreenPos;
        ::ScreenToClient(handle, &mousePos);

        pendingMouseState_.mousePosX_ = mousePos.x;
        pendingMouseState_.mousePosY_ = mousePos.y;
    }
    else
    {
        pendingMouseState_.mousePosX_ = mouseState_.mousePosX_;
        pendingMouseState_.mousePosY_ = mouseState_.mousePosY_;
    }

}

} // namespace SYS