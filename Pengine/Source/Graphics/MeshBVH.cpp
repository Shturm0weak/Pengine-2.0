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

std::set<Raycast::Hit> MeshBVH::Raycast(
	const glm::vec3& start,
	const glm::vec3& direction,
	const float length,
	Visualizer& visualizer)
{
	if (!root) return {};

	std::queue<BVHNode*> nodeStack;
	nodeStack.push(root.get());

	std::set<Raycast::Hit> hits;

	while (!nodeStack.empty())
	{
		BVHNode* node = nodeStack.front();
		nodeStack.pop();

		Raycast::Hit currentHitAABB;
		if (!Raycast::IntersectBoxAABB(start, direction, node->aabb.min, node->aabb.max, length, currentHitAABB))
		{
			continue;
		}

		//visualizer.DrawBox(node->aabb.min, node->aabb.max, { 1.0f, 1.0f, 0.0f }, glm::mat4(1.0f));

		if (node->isLeaf)
		{
			//for (int index : node->triangleIndices)
			//{
			//	const VertexDefault& v0 = *(VertexDefault*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 0] * m_VertexSize);
			//	const VertexDefault& v1 = *(VertexDefault*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 1] * m_VertexSize);
			//	const VertexDefault& v2 = *(VertexDefault*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 2] * m_VertexSize);

			//	const glm::vec3 normal = glm::normalize(glm::cross((v1.position - v0.position), (v2.position - v0.position)));

			//	Raycast::Hit currentHitTriangle{};
			//	if (Raycast::IntersectTriangle(start, direction, v0.position, v1.position, v2.position, normal, length, currentHitTriangle))
			//	{
			//		glm::vec3 bary = Utils::ComputeBarycentric(v0.position, v1.position, v2.position, currentHitTriangle.point);
			//		float u = bary.x, v = bary.y, w = bary.z;

			//		const float epsilon = 1e-5f;
			//		if (u >= -epsilon && v >= -epsilon && w >= -epsilon)
			//		{
			//
			//			potentialHits[k].uv = u * v0.uv + v * v1.uv + w * v2.uv;
			//			potentialHits[k].normal = glm::normalize(w * v0.normal + u * v1.normal + v * v2.normal);
			//		}

			//		hits.emplace(currentHitTriangle);

			//		/*visualizer.DrawSphere(currentHitTriangle.point, 0.05f, 8, { 1.0f, 0.0f, 1.0f }, 0.0f);
			//		visualizer.DrawLine(a, b, { 0.0f, 0.0f, 1.0f }, 0.0f);
			//		visualizer.DrawLine(a, c, { 0.0f, 0.0f, 1.0f }, 0.0f);
			//		visualizer.DrawLine(c, b, { 0.0f, 0.0f, 1.0f }, 0.0f);*/
			//	}
			//	else
			//	{
			//		/*visualizer.DrawLine(a, b, { 1.0f, 1.0f, 1.0f }, 0.0f);
			//		visualizer.DrawLine(a, c, { 1.0f, 1.0f, 1.0f }, 0.0f);
			//		visualizer.DrawLine(c, b, { 1.0f, 1.0f, 1.0f }, 0.0f);*/
			//	}
			//}



			struct TriangleSOA
			{
				float v0_x[8];
				float v0_y[8];
				float v0_z[8];

				float v1_x[8];
				float v1_y[8];
				float v1_z[8];

				float v2_x[8];
				float v2_y[8];
				float v2_z[8];
			};
			TriangleSOA triangles_soa{};

			Raycast::Hit potentialHits[8];

			uint32_t i = 0;
			for (int index : node->triangleIndices)
			{
				const VertexPosition& v0 = *(VertexPosition*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 0] * m_VertexSize);
				const VertexPosition& v1 = *(VertexPosition*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 1] * m_VertexSize);
				const VertexPosition& v2 = *(VertexPosition*)((uint8_t*)m_Vertices + m_Indices[index * 3 + 2] * m_VertexSize);

				triangles_soa.v0_x[i] = v0.position.x;
				triangles_soa.v0_y[i] = v0.position.y;
				triangles_soa.v0_z[i] = v0.position.z;

				triangles_soa.v1_x[i] = v1.position.x;
				triangles_soa.v1_y[i] = v1.position.y;
				triangles_soa.v1_z[i] = v1.position.z;

				triangles_soa.v2_x[i] = v2.position.x;
				triangles_soa.v2_y[i] = v2.position.y;
				triangles_soa.v2_z[i] = v2.position.z;

				i++;
			}

			__m256 tri_v0[3], tri_v1[3], tri_v2[3];
			tri_v0[0] = _mm256_load_ps(&triangles_soa.v0_x[0]);
			tri_v0[1] = _mm256_load_ps(&triangles_soa.v0_y[0]);
			tri_v0[2] = _mm256_load_ps(&triangles_soa.v0_z[0]);

			tri_v1[0] = _mm256_load_ps(&triangles_soa.v1_x[0]);
			tri_v1[1] = _mm256_load_ps(&triangles_soa.v1_y[0]);
			tri_v1[2] = _mm256_load_ps(&triangles_soa.v1_z[0]);

			tri_v2[0] = _mm256_load_ps(&triangles_soa.v2_x[0]);
			tri_v2[1] = _mm256_load_ps(&triangles_soa.v2_y[0]);
			tri_v2[2] = _mm256_load_ps(&triangles_soa.v2_z[0]);

			Raycast::RayPacket rayPacket{};
			Raycast::prepare_ray_packet(start, direction, rayPacket);

			__m256 unused_u;
			__m256 unused_v;
			__m256 t = Raycast::ray_triangle_intersect_avx2(rayPacket.origin, rayPacket.dir, tri_v0, tri_v1, tri_v2, &unused_u, &unused_v);

			auto _mm256_extract_ps = [](__m256 vec, int index) -> float
			{
				union { __m256 v; float a[8]; } converter;
				converter.v = vec;
				return converter.a[index];
			};

			for (int k = 0; k < node->triangleIndices.size(); k++)
			{
				if (_mm256_extract_ps(t, k) < potentialHits[k].distance)
				{
					potentialHits[k].distance = _mm256_extract_ps(t, k);
					potentialHits[k].point = start + potentialHits[k].distance * direction;

					const VertexDefault& v0 = *(VertexDefault*)((uint8_t*)m_Vertices + m_Indices[node->triangleIndices[k] * 3 + 0] * m_VertexSize);
					const VertexDefault& v1 = *(VertexDefault*)((uint8_t*)m_Vertices + m_Indices[node->triangleIndices[k] * 3 + 1] * m_VertexSize);
					const VertexDefault& v2 = *(VertexDefault*)((uint8_t*)m_Vertices + m_Indices[node->triangleIndices[k] * 3 + 2] * m_VertexSize);

					glm::vec3 bary = Utils::ComputeBarycentric(v0.position, v1.position, v2.position, potentialHits[k].point);
					float u = bary.x, v = bary.y, w = bary.z;

					const float epsilon = 1e-5f;
					if (u >= -epsilon && v >= -epsilon && w >= -epsilon)
					{
						potentialHits[k].uv = u * v0.uv + v * v1.uv + w * v2.uv;
						potentialHits[k].normal = glm::normalize(u * v0.normal + v * v1.normal + w * v2.normal);
					}

					hits.emplace(potentialHits[k]);
				}
			}
		}
		else
		{
			if (node->right) nodeStack.push(node->right.get());
			if (node->left) nodeStack.push(node->left.get());
		}
	}

	return std::move(hits);
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
