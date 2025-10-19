#pragma once

#include "Core.h"

#include "../Graphics/Buffer.h"

#include <queue>

namespace Pengine
{

	struct Line
	{
		glm::vec3 start{};
		glm::vec3 end{};
		glm::vec3 color{};
		float duration = 0.0f;
	};

	class PENGINE_API Visualizer
	{
	public:
		void DrawLine(
			const glm::vec3& start,
			const glm::vec3& end,
			const glm::vec3& color,
			const float duration = 0.0f);

		void DrawBox(
			const glm::vec3& min,
			const glm::vec3& max,
			const glm::vec3& color,
			const glm::mat4& transform,
			const float duration = 0.0f);

		void DrawSphere(
			const glm::vec3& color,
			const glm::mat4& transform,
			const float radius,
			int segments,
			const float duration = 0.0f);

		void DrawCapsule(
			const glm::vec3& bottomCenter,
			const glm::vec3& topCenter,
			const glm::vec3& color,
			const glm::mat4& transform,
			float radius,
			int segments = 16,
			float duration = 0.0f);

		void DrawCylinder(
			const glm::vec3& bottomCenter,
			const glm::vec3& topCenter,
			const glm::vec3& color,
			const glm::mat4& transform,
			float radius,
			int segments = 16,
			float duration = 0.0f);

		std::queue<Line>& GetLines() { return m_Lines; }

	private:
		std::queue<Line> m_Lines;
	};

}
