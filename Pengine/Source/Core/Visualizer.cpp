#include "Visualizer.h"

using namespace Pengine;

void Visualizer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
{
	Line line{};
	line.start = start;
	line.end = end;
	line.color = color;
	m_Lines.emplace(line);
}

void Visualizer::DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color, const glm::mat4& transform)
{
	// AntiClockWise, First digit is 0 - FrontFacePoint, 1 - BackFacePoint.
	glm::vec3 point00 = transform * glm::vec4(min.x, min.y, min.z, 1.0f);
	glm::vec3 point01 = transform * glm::vec4(max.x, min.y, min.z, 1.0f);
	glm::vec3 point02 = transform * glm::vec4(max.x, max.y, min.z, 1.0f);
	glm::vec3 point03 = transform * glm::vec4(min.x, max.y, min.z, 1.0f);

	glm::vec3 point10 = transform * glm::vec4(min.x, min.y, max.z, 1.0f);
	glm::vec3 point11 = transform * glm::vec4(max.x, min.y, max.z, 1.0f);
	glm::vec3 point12 = transform * glm::vec4(max.x, max.y, max.z, 1.0f);
	glm::vec3 point13 = transform * glm::vec4(min.x, max.y, max.z, 1.0f);

	// Front.
	DrawLine(point00, point01, color);
	DrawLine(point01, point02, color);
	DrawLine(point02, point03, color);
	DrawLine(point03, point00, color);

	// Back.
	DrawLine(point10, point11, color);
	DrawLine(point11, point12, color);
	DrawLine(point12, point13, color);
	DrawLine(point13, point10, color);

	// Left.
	DrawLine(point00, point10, color);
	DrawLine(point03, point13, color);

	// Right.
	DrawLine(point01, point11, color);
	DrawLine(point02, point12, color);
}
