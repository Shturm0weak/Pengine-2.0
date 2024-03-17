#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Input
	{
	public:
		struct Mouse
		{
			static bool IsMouseDown(int button);

			static bool IsMousePressed(int button);

			static bool IsMouseReleased(int button);

			static glm::dvec2 GetMousePosition();

			static glm::dvec2 GetMousePositionPrevious();

			static glm::dvec2 GetMousePositionDelta();
		};

		struct KeyBoard
		{
			static bool IsKeyDown(int keycode);

			static bool IsKeyPressed(int keycode);

			static bool IsKeyReleased(int keycode);
		};

		struct JoyStick
		{
			static bool IsButtonDown(int buttonCode);

			static bool IsButtonPressed(int buttonCode);

			static bool IsButtonReleased(int buttonCode);

			static float GetAxis(int axisCode);
		};

		static void KeyCallback(int key, int scancode, int action, int mods);

		static void MouseButtonCallback(int button, int action, int mods);

		static void MousePositionCallback(double x, double y);

		static void SetIsMouseDownCallback(const std::function<bool(int)>& callback);

		static void SetIsKeyDownCallback(const std::function<bool(int)>& callback);

		static void ResetInput();

	private:
		static std::unordered_map<int, int> s_ActionsByKeycode; // First - keycode, second - action.

		static std::function<bool(int)> s_IsMouseDownCallback;

		static std::function<bool(int)> s_IsKeyDownCallback;

		static glm::dvec2 s_MousePosition;
		static glm::dvec2 s_MousePositionPrevious;
		static glm::dvec2 s_MousePositionDelta;

		struct JoyStickInfo
		{
			int id = 0;
			int isPresent = 0;

			std::unordered_map<int, float> valuesByAxes;		   // First - axis, second - value.
			std::unordered_map<int, int> buttonsByKeycode;		   // First - keycode, second - action.
			std::unordered_map<int, int> previuosButtonsByKeycode; // First - keycode, second - action.

			void Update();
		} static s_JoyStick;
	};

}