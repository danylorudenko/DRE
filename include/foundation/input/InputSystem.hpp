#pragma once

#include <foundation\class_features\NonCopyable.hpp>

#include <foundation\input\Keyboard.hpp>
#include <cstdint>
#include <Windows.h>

#include <foundation\Common.hpp>

namespace SYS
{

class InputSystem
{
public:
    struct MouseState
    {
        enum MouseButtonOffsets { Left, Right, Middle };
        std::uint32_t mouseButtonStates_ = 0;
        float xDelta_ = 0.0f;
        float yDelta_ = 0.0f;
        std::int32_t mousePosX_ = 0;
        std::int32_t mousePosY_ = 0;
        std::int32_t mouseWheelPos_ = 0;
        float mouseWheelDelta_ = 0.0f;
    };

    struct KeyboardState
    {
        // so we have enough bits for the whole Keys enum
        std::uint64_t keysBits[(std::uint32_t)Keys::END / 64 + 1];
    };

private:
    MouseState pendingMouseState_;
    MouseState mouseState_;
    MouseState prevMouseState_;

    KeyboardState pendingKeyboardState_;
    KeyboardState prevKeyboardState_;
    KeyboardState keyboardState_;


public:
    InputSystem();
    InputSystem(HWND windowHandle);
    InputSystem(InputSystem&& rhs);
    InputSystem& operator=(InputSystem&& rhs);

    ~InputSystem();

    void Update();

    void ProcessSystemInput(HWND handle, WPARAM wparam, LPARAM lparam);

    MouseState const& GetMouseState() const;

    bool GetLeftMouseButtonPressed() const;
    bool GetRightMouseButtonPressed() const;
    bool GetMiddleMouseButtonPressed() const;
    bool GetLeftMouseButtonJustPressed() const;
    bool GetRightMouseButtonJustPressed() const;
    bool GetMiddleMouseButtonJustPressed() const;
    bool GetLeftMouseButtonJustReleased() const;
    bool GetRightMouseButtonJustReleased () const;
    bool GetMiddleMouseButtonJustReleased() const;

    bool GetKeyboardButtonDown(Keys key) const;
    bool GetKeyboardButtonJustPressed(Keys key) const;
    bool GetKeyboardButtonJustReleased(Keys key) const;

    static std::uint32_t GetCharFromKeys(Keys key);

private:
    static void SetKeysBitflagValue(std::uint64_t* bitflag, Keys key, bool value);
    static bool GetKeysBitflagValue(std::uint64_t const* bitflag, Keys key);


};

#ifdef DRE_DEBUG
extern InputSystem* g_InputSystem;
#endif

} // namespace SYS
