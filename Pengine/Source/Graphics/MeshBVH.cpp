#include "MeshBVH.h"

#include "Vertex.h"

#include "../Utils/Utils.h"

using namespace Pengine;

MeshBVH::MeshBVH(
	void* vertices,
	const std::vector<uint32_t>& indices,
	const uint32_t vertexSize,
	int leafSize)
	: m_Vertices(vertices), m_Indices(indices), m_VertexSize(vertexSize), m_LeafSize(leafSize)
{
	if (indices.empty() || indices.size() % 3 != 0) return;

	// Create triangle indices list (each index represents a triangle).
	std::vector<uint32_t> triangleIndices(indices.size() / 3);
	std::iota(triangleIndices.begin(), triangleIndices.end(), 0);

	root = BuildRecursive(triangleIndices);
}

void MeshBVH::Traverse(const std::function<void(BVHNode* node)>& callback) const
{
	if (!root) return;

	std::stack<BVHNode*> nodeStack;
	nodeStack.push(root.get());

	while (!nodeStack.empty())
	{
		BVHNode* node = nodeStack.top();
		nodeStack.pop();

		callback(node);

		if (node->right) nodeStack.push(node->right.get());
		if (node->left) nodeStack.push(node->left.get());
	};
}

bool MeshBVH::Raycast(
	const glm::vec3& start,
	const glm::vec3& direction,
	const float length,
	Raycast::Hit& hit,
	Visualizer& visualizer)
{
	if (!root) return {};

	Raycast::Hit closestHit;
	bool bHit = false;
	std::stack<BVHNode*> nodeStack;
	nodeStack.push(root.get());

	while (!nodeStack.empty())
	{
		BVHNode* node = nodeStack.top();
		nodeStack.pop();

		Raycast::Hit currentHitAABB;
		if (!Raycast::IntersectBoxAABB(start, direction, node->aabb.min, node->aabb.max, length, currentHitAABB))
		{
			continue;
		}

		//visualizer.DrawBox(node->aabb.min, node->aabb.max, { 1.0f, 1.0f, 0.0f }, glm::mat4(1.0f));

		if (node->isLeaf)
		{
			for (int index : node->triangleIndices)
			{
				const VertexPosition& v0 = *(VertexPosition*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 0] * m_VertexSize);
				const VertexPosition& v1 = *(VertexPosition*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 1] * m_VertexSize);
				const VertexPosition& v2 = *(VertexPosition*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 2] * m_VertexSize);

				const glm::vec3 normal = glm::normalize(glm::cross((v1.position - v0.position), (v2.position - v0.position)));

				Raycast::Hit currentHitTriangle;
				if (Raycast::IntersectTriangle(start, direction, v0.position, v1.position, v2.position, normal, length, currentHitTriangle))
				{
					if (currentHitTriangle.distance < closestHit.distance)
					{
						glm::vec3 bary = Utils::ComputeBarycentric(v0.position, v1.position, v2.position, currentHitTriangle.point);
						float u = bary.x, v = bary.y, w = bary.z;

						const float epsilon = 1e-5f;
						if (u >= -epsilon && v >= -epsilon && w >= -epsilon)
						{
							glm::vec2 uv0 = v0.uv;
							glm::vec2 uv1 = v1.uv;
							glm::vec2 uv2 = v2.uv;

							currentHitTriangle.uv = u * uv0 + v * uv1 + w * uv2;
						}

						closestHit = currentHitTriangle;
					}
					bHit = true;

					/*visualizer.DrawSphere(currentHitTriangle.point, 0.05f, 8, { 1.0f, 0.0f, 1.0f }, 0.0f);
					visualizer.DrawLine(a, b, { 0.0f, 0.0f, 1.0f }, 0.0f);
					visualizer.DrawLine(a, c, { 0.0f, 0.0f, 1.0f }, 0.0f);
					visualizer.DrawLine(c, b, { 0.0f, 0.0f, 1.0f }, 0.0f);*/
				}
				else
				{
					/*visualizer.DrawLine(a, b, { 1.0f, 1.0f, 1.0f }, 0.0f);
					visualizer.DrawLine(a, c, { 1.0f, 1.0f, 1.0f }, 0.0f);
					visualizer.DrawLine(c, b, { 1.0f, 1.0f, 1.0f }, 0.0f);*/
				}
			}
		}
		else
		{
			if (node->right) nodeStack.push(node->right.get());
			if (node->left) nodeStack.push(node->left.get());
		}
	}

	hit = closestHit;

	return bHit;
}

std::unique_ptr<MeshBVH::BVHNode> MeshBVH::BuildRecursive(
	std::vector<uint32_t> triangleIndices)
{
	AABB aabb = ComputeBoundingBox(triangleIndices);

	// Create leaf node if below threshold.
	if (triangleIndices.size() <= m_LeafSize)
	{
		return std::make_unique<BVHNode>(aabb, std::move(triangleIndices));
	}

	// Determine split axis (longest dimension).
	glm::vec3 extent =
	{
		aabb.max.x - aabb.min.x,
		aabb.max.y - aabb.min.y,
		aabb.max.z - aabb.min.z
	};

	int axis = 0;
	if (extent.y > extent.x) axis = 1;
	if (extent.z > extent[axis]) axis = 2;

	// Sort triangles by centroid along chosen axis.
	std::sort(triangleIndices.begin(), triangleIndices.end(),
		[&, axis](int a, int b)
		{
			return GetTriangleCentroid(a)[axis] < GetTriangleCentroid(b)[axis];
		});

	// Split into two halves.
	auto mid = triangleIndices.begin() + triangleIndices.size() / 2;
	std::vector<uint32_t> leftIndices(triangleIndices.begin(), mid);
	std::vector<uint32_t> rightIndices(mid, triangleIndices.end());

	// Build child nodes.
	auto left = BuildRecursive(std::move(leftIndices));
	auto right = BuildRecursive(std::move(rightIndices));

	// Merge child bounding boxes.
	aabb = MergeAABBs(left->aabb, right->aabb);

	return std::make_unique<BVHNode>(aabb, std::move(left), std::move(right));
}

glm::vec3 MeshBVH::GetTriangleCentroid(uint32_t index) const
{
	const glm::vec3& v0 = *(glm::vec3*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 0] * m_VertexSize);
	const glm::vec3& v1 = *(glm::vec3*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 1] * m_VertexSize);
	const glm::vec3& v2 = *(glm::vec3*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 2] * m_VertexSize);
	return (v0 + v1 + v2) / 3.0f;
}

AABB MeshBVH::ComputeBoundingBox(const std::vector<uint32_t>& triangleIndices)
{
	AABB aabb;

	for (int index : triangleIndices)
	{
		const glm::vec3& v0 = *(glm::vec3*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 0] * m_VertexSize);
		const glm::vec3& v1 = *(glm::vec3*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 1] * m_VertexSize);
		const glm::vec3& v2 = *(glm::vec3*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 2] * m_VertexSize);

		ExpandAABB(aabb, v0);
		ExpandAABB(aabb, v1);
		ExpandAABB(aabb, v2);
	}

	return aabb;
}

void MeshBVH::ExpandAABB(AABB& aabb, const glm::vec3& point)
{
	aabb.min.x = std::min(aabb.min.x, point.x);
	aabb.min.y = std::min(aabb.min.y, point.y);
	aabb.min.z = std::min(aabb.min.z, point.z);
	aabb.max.x = std::max(aabb.max.x, point.x);
	aabb.max.y = std::max(aabb.max.y, point.y);
	aabb.max.z = std::max(aabb.max.z, point.z);
}

AABB MeshBVH::MergeAABBs(const AABB& a, const AABB& b)
{
	AABB merged;
	merged.min.x = std::min(a.min.x, b.min.x);
	merged.min.y = std::min(a.min.y, b.min.y);
	merged.min.z = std::min(a.min.z, b.min.z);
	merged.max.x = std::max(a.max.x, b.max.x);
	merged.max.y = std::max(a.max.y, b.max.y);
	merged.max.z = std::max(a.max.z, b.max.z);
	return merged;
}
