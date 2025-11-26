#pragma once

#include "../Core/Core.h"
#include "../Core/Raycast.h"
#include "../Core/Visualizer.h"
#include "../Core/BoundingBox.h"

#include <numeric>
#include <stack>

namespace Pengine
{

	class PENGINE_API MeshBVH
	{
	public:
		
		struct BVHNode
		{
			AABB aabb;
			uint32_t left = -1;
			uint32_t right = -1;
			std::vector<uint32_t> triangleIndices;
			bool isLeaf;

			BVHNode(const AABB& aabb, std::vector<uint32_t>&& indices)
				: aabb(aabb)
				, left(-1)
				, right(-1)
				, triangleIndices(std::move(indices)), isLeaf(true)
			{
			}

			BVHNode(const AABB& aabb, uint32_t left,
				uint32_t right)
				: aabb(aabb)
				, left(left)
				, right(right)
				, isLeaf(false)
			{
			}
		};

		MeshBVH(void* vertices,
			const std::vector<uint32_t>& indices,
			const uint32_t vertexSize,
			int leafSize = 4);

		void Traverse(const std::function<void(const BVHNode&)>& callback) const;

		bool Raycast(
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length,
			Raycast::Hit& hit,
			Visualizer& visualizer);

	private:
		void* m_Vertices;
		const std::vector<uint32_t>& m_Indices;
		const uint32_t m_VertexSize;
		const int m_LeafSize;
		std::vector<BVHNode> m_Nodes;
		uint32_t m_Root = -1;

		uint32_t BuildRecursive(std::vector<uint32_t> triangleIndices);

		glm::vec3 GetTriangleCentroid(uint32_t index) const;

		AABB ComputeBoundingBox(const std::vector<uint32_t>& triangleIndices);

		void ExpandAABB(AABB& aabb, const glm::vec3& point);

		AABB MergeAABBs(const AABB& a, const AABB& b);
	};

}
