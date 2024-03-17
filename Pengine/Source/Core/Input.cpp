#include "Input.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace Pengine;

std::unordered_map<int, int> Input::s_ActionsByKeycode;
std::function<bool(int)> Input::s_IsMouseDownCallback;
std::function<bool(int)> Input::s_IsKeyDownCallback;
Input::JoyStickInfo Input::s_JoyStick;
glm::dvec2 Input::s_MousePosition;
glm::dvec2 Input::s_MousePositionPrevious;
glm::dvec2 Input::s_MousePositionDelta;

bool Input::Mouse::IsMouseDown(const int button)
{
	if (s_IsMouseDownCallback)
	{
		return s_IsMouseDownCallback(button);
	}

	return false;
}

bool Input::Mouse::IsMousePressed(const int button)
{
	if (const auto buttonByKeycode = s_ActionsByKeycode.find(button); buttonByKeycode != s_ActionsByKeycode.end())
	{
		if (buttonByKeycode->second == 1)
		{
			return true;
		}
	}

	return false;
}

bool Input::Mouse::IsMouseReleased(const int button)
{
	if (const auto buttonByKeycode = s_ActionsByKeycode.find(button); buttonByKeycode != s_ActionsByKeycode.end())
	{
		if (buttonByKeycode->second == 0)
		{
			return true;
		}
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

bool Input::KeyBoard::IsKeyDown(const int keycode)
{
	if (s_IsKeyDownCallback)
	{
		return s_IsKeyDownCallback(keycode);
	}

	return false;
}

bool Input::KeyBoard::IsKeyPressed(const int keycode)
{
	if (const auto keyByKeycode = s_ActionsByKeycode.find(keycode); keyByKeycode != s_ActionsByKeycode.end())
	{
		if (keyByKeycode->second == 1)
		{
			return true;
		}
	}

	return false;
}

bool Input::KeyBoard::IsKeyReleased(const int keycode)
{
	if (const auto keyByKeycode = s_ActionsByKeycode.find(keycode); keyByKeycode != s_ActionsByKeycode.end())
	{
		if (keyByKeycode->second == 0)
			return true;
	}

	return false;
}

void Input::ResetInput()
{
	s_MousePositionDelta = {0.0, 0.0};

	s_JoyStick.Update();

	for (auto& [code, key] : s_ActionsByKeycode)
	{
		key = -1;
	}
}

void Input::KeyCallback(const int key, const int scancode, const int action, const int mods)
{
	if (const auto keyByKeycode = s_ActionsByKeycode.find(key); keyByKeycode != s_ActionsByKeycode.end())
	{
		keyByKeycode->second = action;
	}
	else
	{
		s_ActionsByKeycode.insert(std::make_pair(key, action));
	}
}

void Input::MouseButtonCallback(const int button, const int action, const int mods)
{
	if (const auto buttonByKeycode = s_ActionsByKeycode.find(button); buttonByKeycode != s_ActionsByKeycode.end())
	{
		buttonByKeycode->second = action;
	}
	else
	{
		s_ActionsByKeycode.insert(std::make_pair(button, action));
	}
}

void Input::MousePositionCallback(const double x, const double y)
{
	s_MousePositionPrevious = s_MousePosition;
	s_MousePosition = {x, y};
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

	for (auto& [axis, value] : valuesByAxes)
	{
		value = -1;
	}

	for (auto& [keycode, button] : buttonsByKeycode)
	{
		button = -1;
	}

	isPresent = glfwJoystickPresent(id);

	if (isPresent)
	{
		int axesCount = 0;
		if (const float* axes = glfwGetJoystickAxes(id, &axesCount); axes)
		{
			for (int i = 0; i < axesCount; i++)
			{

				if (const auto valueByAxis = valuesByAxes.find(i); valueByAxis != valuesByAxes.end())
				{
					valueByAxis->second = axes[i];
				}
				else
				{
					valuesByAxes.insert(std::make_pair(i, axes[i]));
				}
			}
		}

		int buttonsCount = 0;
		if (const unsigned char* buttons = glfwGetJoystickButtons(id, &buttonsCount); buttons)
		{
			for (int i = 0; i < buttonsCount; i++)
			{
				if (const auto buttonByKeycode = buttonsByKeycode.find(i); buttonByKeycode != buttonsByKeycode.end())
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

bool Input::JoyStick::IsButtonDown(const int buttonCode)
{
	if (const auto buttonByKeycode = s_JoyStick.buttonsByKeycode.find(buttonCode);
		buttonByKeycode != s_JoyStick.buttonsByKeycode.end())
	{
		if (buttonByKeycode->second == 1)
		{
			return true;
		}
	}

	return false;
}

bool Input::JoyStick::IsButtonPressed(const int buttonCode)
{
	if (const auto buttonByKeycode = s_JoyStick.buttonsByKeycode.find(buttonCode);
		buttonByKeycode != s_JoyStick.buttonsByKeycode.end())
	{
		if (const auto previousButtonByKeycode = s_JoyStick.previuosButtonsByKeycode.find(buttonCode);
			buttonByKeycode->second == 1 && previousButtonByKeycode->second != 1)
		{
			return true;
		}
	}

	return false;
}

bool Input::JoyStick::IsButtonReleased(const int buttonCode)
{
	if (const auto buttonByKeycode = s_JoyStick.buttonsByKeycode.find(buttonCode);
		buttonByKeycode != s_JoyStick.buttonsByKeycode.end())
	{
		if (buttonByKeycode->second == 0)
		{
			return true;
		}
	}

	return false;
}

float Input::JoyStick::GetAxis(const int axisCode)
{
	if (const auto valueByAxis = s_JoyStick.valuesByAxes.find(axisCode);
		valueByAxis != s_JoyStick.valuesByAxes.end())
	{
		return valueByAxis->second;
	}

	return 0.0f;
}
