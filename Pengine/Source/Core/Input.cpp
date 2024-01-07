#include "Input.h"

using namespace Pengine;

std::unordered_map<int, int> Input::s_ActionsByKeycode;
std::function<bool(int)> Input::s_IsMouseDownCallback;
std::function<bool(int)> Input::s_IsKeyDownCallback;
Input::JoyStickInfo Input::s_JoyStick;
glm::dvec2 Input::s_MousePosition;
glm::dvec2 Input::s_MousePositionPrevious;
glm::dvec2 Input::s_MousePositionDelta;

bool Input::Mouse::IsMouseDown(int button)
{
    if (s_IsMouseDownCallback)
    {
        return s_IsMouseDownCallback(button);
    }

    return false;
}

bool Input::Mouse::IsMousePressed(int button)
{
    auto buttonByKeycode = s_ActionsByKeycode.find(button);
    if (buttonByKeycode != s_ActionsByKeycode.end())
    {
        if (buttonByKeycode->second == 1) return true;
    }

    return false;
}

bool Input::Mouse::IsMouseReleased(int button)
{
    auto buttonByKeycode = s_ActionsByKeycode.find(button);
    if (buttonByKeycode != s_ActionsByKeycode.end())
    {
        if (buttonByKeycode->second == 0) return true;
    }

    return false;
}

glm::dvec2 Input::Mouse::GetMousePosition()
{
    return s_MousePosition;
}

glm::dvec2 Input::Mouse::GetMousePositionPrevious()
{
    return s_MousePositionPrevious;
}

glm::dvec2 Input::Mouse::GetMousePositionDelta()
{
    return s_MousePositionDelta;
}

bool Input::KeyBoard::IsKeyDown(int keycode)
{
    if (s_IsKeyDownCallback)
    {
        return s_IsKeyDownCallback(keycode);
    }

    return false;
}

bool Input::KeyBoard::IsKeyPressed(int keycode)
{
    auto keyByKeycode = s_ActionsByKeycode.find(keycode);
    if (keyByKeycode != s_ActionsByKeycode.end())
    {
        if (keyByKeycode->second == 1)
        {
            return true;
        }
    }

    return false;
}

bool Input::KeyBoard::IsKeyReleased(int keycode)
{
    auto keyByKeycode = s_ActionsByKeycode.find(keycode);
    if (keyByKeycode != s_ActionsByKeycode.end())
    {
        if (keyByKeycode->second == 0) return true;
    }

    return false;
}

void Input::ResetInput()
{
    s_MousePositionDelta = { 0.0, 0.0 };

    s_JoyStick.Update();

    for (auto keyByKeycode = s_ActionsByKeycode.begin(); keyByKeycode != s_ActionsByKeycode.end(); keyByKeycode++)
    {
        keyByKeycode->second = -1;
    }
}

void Input::KeyCallback(int key, int scancode, int action, int mods)
{
    auto keyByKeycode = s_ActionsByKeycode.find(key);
    if (keyByKeycode != s_ActionsByKeycode.end())
    {
        keyByKeycode->second = action;
    }
    else
    {
        s_ActionsByKeycode.insert(std::make_pair(key, action));
    }
}

void Input::MouseButtonCallback(int button, int action, int mods)
{
    auto buttonByKeycode = s_ActionsByKeycode.find(button);
    if (buttonByKeycode != s_ActionsByKeycode.end())
    {
        buttonByKeycode->second = action;
    }
    else
    {
        s_ActionsByKeycode.insert(std::make_pair(button, action));
    }
}

void Input::MousePositionCallback(double x, double y)
{
    s_MousePositionPrevious = s_MousePosition;
    s_MousePosition = { x, y };
    s_MousePositionDelta = s_MousePosition - s_MousePositionPrevious;
}

void Input::SetIsMouseDownCallback(const std::function<bool(int)>& callback)
{
    s_IsMouseDownCallback = callback;
}

void Input::SetIsKeyDownCallback(const std::function<bool(int)>& callback)
{
    s_IsKeyDownCallback = callback;
}

void Input::JoyStickInfo::Update()
{
    previuosButtonsByKeycode = buttonsByKeycode;

    for (auto valueByAxis = valuesByAxes.begin(); valueByAxis != valuesByAxes.end(); valueByAxis++)
    {
        valueByAxis->second = -1;
    }

    for (auto button = buttonsByKeycode.begin(); button != buttonsByKeycode.end(); button++)
    {
        button->second = -1;
    }

    isPresent = glfwJoystickPresent(id);

    if (isPresent)
    {
        int axesCount = 0;
        const float* axes = glfwGetJoystickAxes(id, &axesCount);

        if (axes)
        {
            for (size_t i = 0; i < axesCount; i++)
            {
                auto valueByAxis = valuesByAxes.find(i);
                if (valueByAxis != valuesByAxes.end())
                {
                    valueByAxis->second = axes[i];
                }
                else
                {
                    valuesByAxes.insert(std::make_pair(i, axes[i]));
                }
            }
        }

        int buttonCount = 0;
        const unsigned char* buttons = glfwGetJoystickButtons(id, &buttonCount);

        if (buttons)
        {
            for (size_t i = 0; i < buttonCount; i++)
            {
                auto buttonByKeycode = buttonsByKeycode.find(i);
                if (buttonByKeycode != buttonsByKeycode.end())
                {
                    buttonByKeycode->second = buttons[i];
                }
                else
                {
                    buttonsByKeycode.insert(std::make_pair(i, buttons[i]));
                }
            }
        }
    }
}

bool Input::JoyStick::IsButtonDown(int buttonCode)
{
    auto buttonByKeycode = s_JoyStick.buttonsByKeycode.find(buttonCode);
    if (buttonByKeycode != s_JoyStick.buttonsByKeycode.end())
    {
        if (buttonByKeycode->second == 1)
        {
            return true;
        }
    }

    return false;
}

bool Input::JoyStick::IsButtonPressed(int buttonCode)
{
    auto buttonByKeycode = s_JoyStick.buttonsByKeycode.find(buttonCode);
    auto previousButtonByKeycode = s_JoyStick.previuosButtonsByKeycode.find(buttonCode);
    if (buttonByKeycode != s_JoyStick.buttonsByKeycode.end())
    {
        if (buttonByKeycode->second == 1 && previousButtonByKeycode->second != 1)
        {
            return true;
        }
    }

    return false;
}

bool Input::JoyStick::IsButtonReleased(int buttonCode)
{
    auto buttonByKeycode = s_JoyStick.buttonsByKeycode.find(buttonCode);
    if (buttonByKeycode != s_JoyStick.buttonsByKeycode.end())
    {
        if (buttonByKeycode->second == 0)
        {
            return true;
        }
    }

    return false;
}

float Input::JoyStick::GetAxis(int axisCode)
{
    auto valueByAxis = s_JoyStick.valuesByAxes.find(axisCode);
    if (valueByAxis != s_JoyStick.valuesByAxes.end())
    {
        return valueByAxis->second;
    }

    return 0.0f;
}
