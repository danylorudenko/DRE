#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\Common.hpp>
#include <foundation\memory\MemoryOps.hpp>
#include <foundation\input\Keyboard.hpp>
#include <Windows.h>


namespace SYS
{

class InputSystem
    : public NonCopyable
    , public NonMovable
{
public:
    struct MouseState
    {
        enum MouseButtonOffsets { Left, Right, Middle };

        DRE::U32 mouseButtonStates_ = 0;
        float xDelta_ = 0.0f;
        float yDelta_ = 0.0f;
        DRE::S32 mousePosX_ = 0;
        DRE::S32 mousePosY_ = 0;
        DRE::S32 mouseWheelPos_ = 0;
        float mouseWheelDelta_ = 0.0f;

        void Reset()
        {
            mouseButtonStates_ = 0;
            xDelta_ = 0.0f;
            yDelta_ = 0.0f;
            mousePosX_ = 0;
            mousePosY_ = 0;
            mouseWheelPos_ = 0;
            mouseWheelDelta_ = 0.0f;
        }
    };

    struct KeyboardState
    {
        // so we have enough bits for the whole Keys enum
        DRE::U64 keysBits[(DRE::U32)Keys::END / 64 + 1];

        void Reset()
        {
            DRE::MemZero(&keysBits, sizeof(keysBits));
        }
    };

private:
    MouseState pendingMouseState_;
    MouseState mouseState_;
    MouseState prevMouseState_;

    KeyboardState pendingKeyboardState_;
    KeyboardState prevKeyboardState_;
    KeyboardState keyboardState_;

    bool m_MouseRegistered;
    bool m_KeyboardRegistered;


public:
    InputSystem(HWND windowHandle);

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

    static DRE::U32 GetCharFromKeys(Keys key);

private:
    static void SetKeysBitflagValue(DRE::U64* bitflag, Keys key, bool value);
    static bool GetKeysBitflagValue(DRE::U64 const* bitflag, Keys key);


};

#ifdef DRE_DEBUG
extern InputSystem* g_InputSystem;
#endif

} // namespace SYS
