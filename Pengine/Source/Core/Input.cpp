#include "Input.h"

#include "Window.h"

#include <GLFW/glfw3.h>

using namespace Pengine;

bool Input::IsMouseDown(const int button)
{
	if (m_IsMouseDownCallback)
	{
		return m_IsMouseDownCallback(button);
	}

	return false;
}

bool Input::IsMousePressed(const int button)
{
	if (const auto buttonByKeycode = m_ActionsByKeycode.find(button); buttonByKeycode != m_ActionsByKeycode.end())
	{
		if (buttonByKeycode->second == 1)
		{
			return true;
		}
	}

	return false;
}

bool Input::IsMouseReleased(const int button)
{
	if (const auto buttonByKeycode = m_ActionsByKeycode.find(button); buttonByKeycode != m_ActionsByKeycode.end())
	{
		if (buttonByKeycode->second == 0)
		{
			return true;
		}
	}

	return false;
}

glm::dvec2 Input::GetMousePosition()
{
	return m_MousePosition;
}

glm::dvec2 Input::GetMousePositionPrevious()
{
	return m_MousePositionPrevious;
}

glm::dvec2 Input::GetMousePositionDelta()
{
	return m_MousePositionDelta;
}

bool Input::IsKeyDown(const int keycode)
{
	if (m_IsKeyDownCallback)
	{
		return m_IsKeyDownCallback(keycode);
	}

	return false;
}

bool Input::IsKeyPressed(const int keycode)
{
	if (const auto keyByKeycode = m_ActionsByKeycode.find(keycode); keyByKeycode != m_ActionsByKeycode.end())
	{
		if (keyByKeycode->second == 1)
		{
			return true;
		}
	}

	return false;
}

bool Input::IsKeyReleased(const int keycode)
{
	if (const auto keyByKeycode = m_ActionsByKeycode.find(keycode); keyByKeycode != m_ActionsByKeycode.end())
	{
		if (keyByKeycode->second == 0)
			return true;
	}

	return false;
}

void Input::ResetInput()
{
	m_MousePositionDelta = {0.0, 0.0};

	m_JoyStick.Update();

	for (auto& [code, key] : m_ActionsByKeycode)
	{
		key = -1;
	}
}

Input& Input::GetInstance(Window* window)
{
	static std::unordered_map<Window*, Input> inputs;
	return inputs[window];
}

void Input::RemoveInstance(Window* window)
{
	static std::unordered_map<Window*, Input> inputs;
	inputs.erase(window);
}

void Input::KeyCallback(const int key, const int scancode, const int action, const int mods)
{
	if (const auto keyByKeycode = m_ActionsByKeycode.find(key); keyByKeycode != m_ActionsByKeycode.end())
	{
		keyByKeycode->second = action;
	}
	else
	{
		m_ActionsByKeycode.insert(std::make_pair(key, action));
	}
}

void Input::MouseButtonCallback(const int button, const int action, const int mods)
{
	if (const auto buttonByKeycode = m_ActionsByKeycode.find(button); buttonByKeycode != m_ActionsByKeycode.end())
	{
		buttonByKeycode->second = action;
	}
	else
	{
		m_ActionsByKeycode.insert(std::make_pair(button, action));
	}
}

void Input::MousePositionCallback(const double x, const double y)
{
	m_MousePositionPrevious = m_MousePosition;
	m_MousePosition = {x, y};
	m_MousePositionDelta = m_MousePosition - m_MousePositionPrevious;
}

void Input::SetIsMouseDownCallback(const std::function<bool(int)>& callback)
{
	m_IsMouseDownCallback = callback;
}

void Input::SetIsKeyDownCallback(const std::function<bool(int)>& callback)
{
	m_IsKeyDownCallback = callback;
}

void Input::JoyStick::Update()
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
	if (const auto buttonByKeycode = buttonsByKeycode.find(buttonCode);
		buttonByKeycode != buttonsByKeycode.end())
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
	if (const auto buttonByKeycode = buttonsByKeycode.find(buttonCode);
		buttonByKeycode != buttonsByKeycode.end())
	{
		if (const auto previousButtonByKeycode = previuosButtonsByKeycode.find(buttonCode);
			buttonByKeycode->second == 1 && previousButtonByKeycode->second != 1)
		{
			return true;
		}
	}

	return false;
}

bool Input::JoyStick::IsButtonReleased(const int buttonCode)
{
	if (const auto buttonByKeycode = buttonsByKeycode.find(buttonCode);
		buttonByKeycode != buttonsByKeycode.end())
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
	if (const auto valueByAxis = valuesByAxes.find(axisCode);
		valueByAxis != valuesByAxes.end())
	{
		return valueByAxis->second;
	}

	return 0.0f;
}
