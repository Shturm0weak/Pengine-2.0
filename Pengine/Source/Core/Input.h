#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Input
	{
	public:
		static Input& GetInstance(class Window* window);

		static void RemoveInstance(class Window* window);

		bool IsMouseDown(int button);

		bool IsMousePressed(int button);

		bool IsMouseReleased(int button);

		glm::dvec2 GetMousePosition();

		glm::dvec2 GetMousePositionPrevious();

		glm::dvec2 GetMousePositionDelta();

		bool IsKeyDown(int keycode);

		bool IsKeyPressed(int keycode);

		bool IsKeyReleased(int keycode);

		struct JoyStick
		{
		public:
			bool IsButtonDown(int buttonCode);

			bool IsButtonPressed(int buttonCode);

			bool IsButtonReleased(int buttonCode);

			float GetAxis(int axisCode);

		private:
			int id = 0;
			int isPresent = 0;

			std::unordered_map<int, float> valuesByAxes;		   // First - axis, second - value.
			std::unordered_map<int, int> buttonsByKeycode;		   // First - keycode, second - action.
			std::unordered_map<int, int> previuosButtonsByKeycode; // First - keycode, second - action.

			void Update();

			friend class Input;
		};

		void KeyCallback(int key, int scancode, int action, int mods);

		void MouseButtonCallback(int button, int action, int mods);

		void MousePositionCallback(double x, double y);

		void SetIsMouseDownCallback(const std::function<bool(int)>& callback);

		void SetIsKeyDownCallback(const std::function<bool(int)>& callback);

		void ResetInput();

	private:
		glm::dvec2 m_MousePosition;
		glm::dvec2 m_MousePositionPrevious;
		glm::dvec2 m_MousePositionDelta;

		std::unordered_map<int, int> m_ActionsByKeycode; // First - keycode, second - action.

		std::function<bool(int)> m_IsMouseDownCallback;

		std::function<bool(int)> m_IsKeyDownCallback;

		JoyStick m_JoyStick;
	};

}
