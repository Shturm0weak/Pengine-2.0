#include "Raycast.h"

#include "SceneManager.h"

#include "../Graphics/Mesh.h"
#include "../Graphics/Vertex.h"
#include "../Components/Renderer3D.h"
#include "../Components/Transform.h"

#include "../Utils/Utils.h"

using namespace Pengine;

bool Raycast::IntersectTriangle(
	const glm::vec3& start,
	const glm::vec3& direction,
	const glm::vec3& a,
	const glm::vec3& b,
	const glm::vec3& c,
	const glm::vec3& planeNormal,
	const float length,
	Hit& hit)
{
	const glm::vec3 end = start + direction * length;
	const glm::vec3 distance = direction * length;
	const glm::vec3 distanceToPlane = a - start;
	const float vp = glm::dot(distance, planeNormal);
	if (vp >= -glm::epsilon<float>() && vp <= glm::epsilon<float>())
	{
		return false;
	}

	const float wp = glm::dot(distanceToPlane, planeNormal);
	const float t = wp / vp;
	const glm::vec3 point = distance * t + start;
	hit.point = point;
	hit.distance = glm::distance(start, point);
	const glm::vec3 edge0 = b - a;
	const glm::vec3 edge1 = c - b;
	const glm::vec3 edge2 = a - c;
	const glm::vec3 c0 = point - a;
	const glm::vec3 c1 = point - b;
	const glm::vec3 c2 = point - c;

	if (glm::dot(glm::normalize(point - start), direction) < 0.0f)
	{
		return false;
	}

	if (glm::dot(planeNormal, glm::cross(edge0, c0)) > 0.0f &&
		glm::dot(planeNormal, glm::cross(edge1, c1)) > 0.0f &&
		glm::dot(planeNormal, glm::cross(edge2, c2)) > 0.0f)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Raycast::IntersectBoxOBB(
	const glm::vec3& start,
	const glm::vec3& direction,
	const glm::vec3& min,
	const glm::vec3& max,
	const glm::vec3& position,
	const glm::vec3& scale,
	const glm::mat4& rotation,
	const float length,
	Hit& hit)
{
	glm::vec3 obbMin = min * scale;
	glm::vec3 obbMax = max * scale;
	const float* transformPtr = glm::value_ptr(rotation);
	glm::vec3 axis{};
	glm::vec3 distance = position - start;

	float tMin = 0.0f, tMax = std::numeric_limits<float>().max(), nomLen, denomLen, axesMin, axesMax;
	size_t row = 0;

	for (size_t i = 0; i < 3; i++)
	{
		row = i * 4;
		axis = glm::vec3(transformPtr[row], transformPtr[row + 1], transformPtr[row + 2]);
		nomLen = glm::dot(axis, distance);
		denomLen = glm::dot(direction, axis);
		axis = glm::normalize(axis);
		if (glm::abs(denomLen) > glm::epsilon<float>())
		{
			axesMin = (nomLen + obbMin[i]) / denomLen;
			axesMax = (nomLen + obbMax[i]) / denomLen;

			if (axesMin > axesMax) { std::swap(axesMin, axesMax); }
			if (axesMin > tMin) tMin = axesMin;
			if (axesMax < tMax) tMax = axesMax;

			if (tMax < tMin) return false;
		}
		else if (-nomLen + obbMin[i] > 0.0f || -nomLen + obbMax[i] < 0.0f) return false;
	}

	hit.distance = tMin;
	hit.point = tMin * direction + start;

	return hit.distance < length;
}

std::map<float, std::shared_ptr<Entity>> Raycast::RaycastScene(
	std::shared_ptr<Scene> scene,
	const glm::vec3& start,
	const glm::vec3& direction,
	const float length)
{
	std::map<float, std::shared_ptr<Entity>> hits;

	const auto r3dView = scene->GetRegistry().view<Renderer3D>();
	for (const entt::entity& entity : r3dView)
	{
		const Renderer3D& r3d = scene->GetRegistry().get<Renderer3D>(entity);
		const Transform& transform = scene->GetRegistry().get<Transform>(entity);
		if (!transform.GetEntity()->IsEnabled())
		{
			continue;
		}

		if (!r3d.mesh)
		{
			continue;
		}

		Hit hitOBB{};
		if (IntersectBoxOBB(
			start,
			direction,
			r3d.mesh->GetBoundingBox().min,
			r3d.mesh->GetBoundingBox().max,
			transform.GetPosition(),
			transform.GetScale(),
			transform.GetRotationMat4(),
			length,
			hitOBB))
		{
			Hit hitMesh{};

			if (r3d.mesh->Raycast(start, direction, length, transform.GetTransform(), hitMesh.distance, scene->GetVisualizer()))
			{
				hits.emplace(hitMesh.distance, transform.GetEntity());
			}
		}
	}

	return hits;
}

Raycast::OutCode Raycast::ComputeOutCode(
	const glm::vec2& point,
	const glm::vec2& min,
	const glm::vec2& max)
{
	constexpr int INSIDE = 0b0000;
	constexpr int LEFT = 0b0001;
	constexpr int RIGHT = 0b0010;
	constexpr int BOTTOM = 0b0100;
	constexpr int TOP = 0b1000;

	OutCode code = INSIDE;

	if (point.x < min.x)
		code |= LEFT;
	else if (point.x > max.x)
		code |= RIGHT;
	if (point.y < min.y)
		code |= BOTTOM;
	else if (point.y > max.y)
		code |= TOP;

	return code;
}

bool Raycast::CohenSutherlandLineClip(
	glm::vec2 start,
	glm::vec2 end,
	const glm::vec2& min,
	const glm::vec2& max)
{
	constexpr int INSIDE = 0b0000;
	constexpr int LEFT = 0b0001;
	constexpr int RIGHT = 0b0010;
	constexpr int BOTTOM = 0b0100;
	constexpr int TOP = 0b1000;

	// Compute outcodes for start, end, and whatever point lies outside the clip rectangle.
	OutCode outcode0 = ComputeOutCode(start, min, max);
	OutCode outcode1 = ComputeOutCode(end, min, max);
	bool accept = false;

	while (true) {
		if (!(outcode0 | outcode1)) {
			// Bitwise OR is 0: both points inside window, trivially accept and exit loop.
			accept = true;
			break;
		}
		else if (outcode0 & outcode1) {
			// Bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
			// or BOTTOM), so both must be outside window, exit loop (accept is false).
			break;
		}
		else {
			// Failed both tests, so calculate the line segment to clip
			// from an outside point to an intersection with clip edge.
			double x, y;

			// At least one endpoint is outside the clip rectangle, pick it.
			OutCode outcodeOut = outcode1 > outcode0 ? outcode1 : outcode0;

			// Now find the intersection point.
			// Use formulas:
			//   slope = (y1 - y0) / (x1 - x0)
			//   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
			//   y = y0 + slope * (xm - x0), where xm is xmin or xmax
			// No need to worry about divide-by-zero because, in each case, the
			// outcode bit being tested guarantees the denominator is non-zero.
			if (outcodeOut & TOP)
			{
				x = start.x + (end.x - start.x) * (max.y - start.y) / (end.y - start.y);
				y = max.y;
			}
			else if (outcodeOut & BOTTOM)
			{
				x = start.x + (end.x - start.x) * (min.y - start.y) / (end.y - start.y);
				y = min.y;
			}
			else if (outcodeOut & RIGHT)
			{
				y = start.y + (end.y - start.y) * (max.x - start.x) / (end.x - start.x);
				x = max.x;
			}
			else if (outcodeOut & LEFT)
			{
				y = start.y + (end.y - start.y) * (min.x - start.x) / (end.x - start.x);
				x = min.x;
			}

			// Now we move outside point to intersection point to clip and get ready for next pass.
			if (outcodeOut == outcode0) {
				start.x = x;
				start.y = y;
				outcode0 = ComputeOutCode(start, min, max);
			}
			else {
				end.x = x;
				end.y = y;
				outcode1 = ComputeOutCode(end, min, max);
			}
		}
	}
	return accept;
}
