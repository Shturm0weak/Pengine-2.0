#pragma once

#include "Core.h"

#include "../Graphics/Buffer.h"

#include <queue>

namespace Pengine
{

	struct Line
	{
		glm::vec3 start;
		glm::vec3 end;
		glm::vec3 color;
	};

	class PENGINE_API Visualizer
	{
	public:
		void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

		void DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color, const glm::mat4& transform);

		std::queue<Line>& GetLines() { return m_Lines; }

	private:
		std::queue<Line> m_Lines;
	};

}