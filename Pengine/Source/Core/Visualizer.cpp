#include "Visualizer.h"

using namespace Pengine;

void Visualizer::DrawLine(
	const glm::vec3& start,
	const glm::vec3& end,
	const glm::vec3& color,
	const float duration)
{
	Line line{};
	line.start = start;
	line.end = end;
	line.color = color;
	line.duration = duration;
	m_Lines.emplace(line);
}

void Visualizer::DrawBox(
	const glm::vec3& min,
	const glm::vec3& max,
	const glm::vec3& color,
	const glm::mat4& transform,
	const float duration)
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
	DrawLine(point00, point01, color, duration);
	DrawLine(point01, point02, color, duration);
	DrawLine(point02, point03, color, duration);
	DrawLine(point03, point00, color, duration);

	// Back.
	DrawLine(point10, point11, color, duration);
	DrawLine(point11, point12, color, duration);
	DrawLine(point12, point13, color, duration);
	DrawLine(point13, point10, color, duration);

	// Left.
	DrawLine(point00, point10, color, duration);
	DrawLine(point03, point13, color, duration);

	// Right.
	DrawLine(point01, point11, color, duration);
	DrawLine(point02, point12, color, duration);
}

void Visualizer::DrawSphere(
	const glm::vec3& position,
	const float radius,
	int segments,
	const glm::vec3& color,
	const float duration)
{
	// Need at least 3 segments.
	segments = std::max(3, segments);

	std::vector<glm::vec3> currentRings(segments);
	std::vector<glm::vec3> previousRings(segments);

	// Generate and draw rings.
	for (int ring = 0; ring <= segments; ring++)
	{
		float phi = glm::pi<float>() * float(ring) / float(segments);
		float y = radius * cos(phi);
		float ringRadius = radius * sin(phi);

		// Generate points for current ring.
		for (int i = 0; i < segments; i++)
		{
			float theta = 2.0f * glm::pi<float>() * float(i) / float(segments);
			currentRings[i] = position + glm::vec3(
				ringRadius * cos(theta),
				y,
				ringRadius * sin(theta)
			);
		}

		// Connect points within current ring (skip for first and last point).
		if (ring > 0 && ring < segments)
		{
			for (int i = 0; i < segments; i++)
			{
				int next = (i + 1) % segments;
				DrawLine(currentRings[i], currentRings[next], color, duration);
			}
		}

		// Connect to previous ring (if not first ring).
		if (ring > 0)
		{
			for (int i = 0; i < segments; i++)
			{
				DrawLine(previousRings[i], currentRings[i], color, duration);
			}
		}

		// Save current ring for next iteration.
		previousRings = currentRings;
	}
}
