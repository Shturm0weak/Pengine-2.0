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

		Hit hit{};
		if (IntersectBoxOBB(
			start,
			direction,
			r3d.mesh->GetBoundingBox().min,
			r3d.mesh->GetBoundingBox().max,
			transform.GetPosition(),
			transform.GetScale(),
			transform.GetRotationMat4(),
			length,
			hit))
		{
			const std::vector<float>& vertices = r3d.mesh->GetRawVertices();
			const std::vector<uint32_t>& indices = r3d.mesh->GetRawIndices();
			const Vertex* vertex = reinterpret_cast<const Vertex*>(vertices.data());
			for (size_t i = 0; i < r3d.mesh->GetIndexCount(); i+=3)
			{
				const glm::vec3& vertex0 = vertex[indices[i + 0]].position;
				const glm::vec3& vertex1 = vertex[indices[i + 1]].position;
				const glm::vec3& vertex2 = vertex[indices[i + 2]].position;

				const glm::vec3 a = transform.GetTransform() * glm::vec4(vertex0, 1.0f);
				const glm::vec3 b = transform.GetTransform() * glm::vec4(vertex1, 1.0f);
				const glm::vec3 c = transform.GetTransform() * glm::vec4(vertex2, 1.0f);

				const glm::vec3 normal = glm::normalize(glm::cross((b - a), (c - a)));

				if (IntersectTriangle(start, direction, a, b, c, normal, length, hit))
				{
					//scene->GetVisualizer().DrawLine(a, b, { 1.0f, 0.0f, 1.0f }, 5.0f);
					//scene->GetVisualizer().DrawLine(a, c, { 1.0f, 0.0f, 1.0f }, 5.0f);
					//scene->GetVisualizer().DrawLine(c, b, { 1.0f, 0.0f, 1.0f }, 5.0f);
					
					hits.emplace(hit.distance, transform.GetEntity());
					break;
				}
			}
		}
	}

	return hits;
}
