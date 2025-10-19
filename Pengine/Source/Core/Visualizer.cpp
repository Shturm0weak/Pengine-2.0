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
	const glm::vec3& color,
	const glm::mat4& transform,
	const float radius,
	int segments,
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
			currentRings[i] = glm::vec3(
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
				DrawLine(transform * glm::vec4(currentRings[i], 1.0f), transform * glm::vec4(currentRings[next], 1.0f), color, duration);
			}
		}

		// Connect to previous ring (if not first ring).
		if (ring > 0)
		{
			for (int i = 0; i < segments; i++)
			{
				DrawLine(transform * glm::vec4(previousRings[i], 1.0f), transform * glm::vec4(currentRings[i], 1.0f), color, duration);
			}
		}

		// Save current ring for next iteration.
		previousRings = currentRings;
	}
}

void GenerateHemisphereLines(
	std::vector<std::pair<glm::vec3, glm::vec3>>& lines,
	const glm::vec3& center,
	const glm::vec3& direction,
	const glm::vec3& perp1,
	const glm::vec3& perp2,
	float radius,
	int segments)
{
	const int ringCount = segments / 2;

	for (int i = 0; i <= ringCount; ++i)
	{
		float phi = glm::pi<float>() * 0.5f * (static_cast<float>(i) / ringCount);
		float ringRadius = radius * glm::cos(phi);
		float ringHeight = radius * glm::sin(phi);

		glm::vec3 ringCenter = center + direction * ringHeight;

		for (int j = 0; j < segments; ++j)
		{
			float theta1 = 2.0f * glm::pi<float>() * (static_cast<float>(j) / segments);
			float theta2 = 2.0f * glm::pi<float>() * (static_cast<float>(j + 1) / segments);

			glm::vec3 point1 = ringCenter +
				perp1 * (ringRadius * glm::cos(theta1)) +
				perp2 * (ringRadius * glm::sin(theta1));

			glm::vec3 point2 = ringCenter +
				perp1 * (ringRadius * glm::cos(theta2)) +
				perp2 * (ringRadius * glm::sin(theta2));

			lines.push_back({ point1, point2 });

			// Add vertical lines between rings.
			if (i < ringCount)
			{
				float nextPhi = glm::pi<float>() * 0.5f * (static_cast<float>(i + 1) / ringCount);
				float nextRingRadius = radius * glm::cos(nextPhi);
				float nextRingHeight = radius * glm::sin(nextPhi);

				glm::vec3 nextRingCenter = center + direction * nextRingHeight;
				glm::vec3 point3 = nextRingCenter +
					perp1 * (nextRingRadius * glm::cos(theta1)) +
					perp2 * (nextRingRadius * glm::sin(theta1));

				lines.push_back({ point1, point3 });
			}
		}
	}
}

void GenerateCylinderLines(
	std::vector<std::pair<glm::vec3, glm::vec3>>& lines,
	const glm::vec3& bottomCenter,
	const glm::vec3& topCenter,
	const glm::vec3& axis,
	const glm::vec3& perp1,
	const glm::vec3& perp2,
	float radius,
	int segments)
{
	glm::vec3 cylinderVec = topCenter - bottomCenter;
	float cylinderHeight = glm::length(cylinderVec);

	for (int i = 0; i < segments; ++i)
	{
		float theta1 = 2.0f * glm::pi<float>() * (static_cast<float>(i) / segments);
		float theta2 = 2.0f * glm::pi<float>() * (static_cast<float>(i + 1) / segments);

		// Bottom circle.
		glm::vec3 bottomPoint1 = bottomCenter +
			perp1 * (radius * glm::cos(theta1)) +
			perp2 * (radius * glm::sin(theta1));

		glm::vec3 bottomPoint2 = bottomCenter +
			perp1 * (radius * glm::cos(theta2)) +
			perp2 * (radius * glm::sin(theta2));

		lines.push_back({ bottomPoint1, bottomPoint2 });

		// Top circle.
		glm::vec3 topPoint1 = topCenter +
			perp1 * (radius * glm::cos(theta1)) +
			perp2 * (radius * glm::sin(theta1));

		glm::vec3 topPoint2 = topCenter +
			perp1 * (radius * glm::cos(theta2)) +
			perp2 * (radius * glm::sin(theta2));

		lines.push_back({ topPoint1, topPoint2 });

		// Vertical lines connecting bottom and top.
		lines.push_back({ bottomPoint1, topPoint1 });
	}
}

void Visualizer::DrawCapsule(
	const glm::vec3& bottomCenter,
	const glm::vec3& topCenter,
	const glm::vec3& color,
	const glm::mat4& transform,
	float radius,
	int segments,
	float duration)
{
	std::vector<std::pair<glm::vec3, glm::vec3>> lines;
	
	glm::vec3 axis = glm::normalize(topCenter - bottomCenter);
	float height = glm::length(topCenter - bottomCenter);
	
	// Find perpendicular vectors to the capsule axis.
	glm::vec3 perp1, perp2;
	if (std::abs(axis.x) > std::abs(axis.y))
	{
		perp1 = glm::normalize(glm::vec3(axis.z, 0, -axis.x));
	}
	else
	{
		perp1 = glm::normalize(glm::vec3(0, -axis.z, axis.y));
	}
	perp2 = glm::normalize(glm::cross(axis, perp1));
	
	// Generate hemisphere at bottom.
	glm::vec3 bottomHemisphereCenter = bottomCenter + axis * radius;
	GenerateHemisphereLines(lines, bottomHemisphereCenter, -axis, perp1, perp2, radius, segments);
	
	// Generate hemisphere at top.
	glm::vec3 topHemisphereCenter = topCenter - axis * radius;
	GenerateHemisphereLines(lines, topHemisphereCenter, axis, perp1, perp2, radius, segments);
	
	// Generate connecting cylinder lines.
	GenerateCylinderLines(lines, bottomHemisphereCenter, topHemisphereCenter, axis, perp1, perp2, radius, segments);
	
	for (const auto& [start, end] : lines)
	{
		DrawLine(transform * glm::vec4(start, 1.0f), transform * glm::vec4(end, 1.0f), color, duration);
	}
}

void Visualizer::DrawCylinder(
	const glm::vec3& bottomCenter,
	const glm::vec3& topCenter,
	const glm::vec3& color,
	const glm::mat4& transform,
	float radius,
	int segments,
	float duration)
{
	std::vector<std::pair<glm::vec3, glm::vec3>> lines;

	glm::vec3 axis = topCenter - bottomCenter;
	float height = glm::length(axis);
	glm::vec3 unitAxis = glm::normalize(axis);

	// Find two perpendicular vectors to the cylinder axis.
	glm::vec3 perp1, perp2;

	// Choose a vector that's not parallel to the axis.
	if (std::abs(unitAxis.x) > std::abs(unitAxis.y)) {
		perp1 = glm::normalize(glm::vec3(unitAxis.z, 0, -unitAxis.x));
	}
	else {
		perp1 = glm::normalize(glm::vec3(0, -unitAxis.z, unitAxis.y));
	}
	perp2 = glm::normalize(glm::cross(unitAxis, perp1));

	// Generate base circle.
	for (int i = 0; i < segments; ++i)
	{
		float angle1 = 2.0f * glm::pi<float>() * (static_cast<float>(i) / segments);
		float angle2 = 2.0f * glm::pi<float>() * (static_cast<float>(i + 1) / segments);

		glm::vec3 point1 = bottomCenter +
			perp1 * (radius * glm::cos(angle1)) +
			perp2 * (radius * glm::sin(angle1));

		glm::vec3 point2 = bottomCenter +
			perp1 * (radius * glm::cos(angle2)) +
			perp2 * (radius * glm::sin(angle2));

		lines.push_back({ point1, point2 });
	}

	// Generate top circle.
	for (int i = 0; i < segments; ++i)
	{
		float angle1 = 2.0f * glm::pi<float>() * (static_cast<float>(i) / segments);
		float angle2 = 2.0f * glm::pi<float>() * (static_cast<float>(i + 1) / segments);

		glm::vec3 point1 = topCenter +
			perp1 * (radius * glm::cos(angle1)) +
			perp2 * (radius * glm::sin(angle1));

		glm::vec3 point2 = topCenter +
			perp1 * (radius * glm::cos(angle2)) +
			perp2 * (radius * glm::sin(angle2));

		lines.push_back({ point1, point2 });
	}

	for (int i = 0; i < segments; ++i)
	{
		float angle = 2.0f * glm::pi<float>() * (static_cast<float>(i) / segments);

		glm::vec3 basePoint = bottomCenter +
			perp1 * (radius * glm::cos(angle)) +
			perp2 * (radius * glm::sin(angle));

		glm::vec3 topPoint = topCenter +
			perp1 * (radius * glm::cos(angle)) +
			perp2 * (radius * glm::sin(angle));

		lines.push_back({ basePoint, topPoint });
	}

	for (const auto& [start, end] : lines)
	{
		DrawLine(transform * glm::vec4(start, 1.0f), transform * glm::vec4(end, 1.0f), color, duration);
	}
}
