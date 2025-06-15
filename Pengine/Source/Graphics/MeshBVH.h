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
			std::unique_ptr<BVHNode> left;
			std::unique_ptr<BVHNode> right;
			std::vector<uint32_t> triangleIndices;
			bool isLeaf;

			BVHNode(const AABB& aabb, std::vector<uint32_t>&& indices)
				: aabb(aabb)
				, left(nullptr)
				, right(nullptr)
				, triangleIndices(std::move(indices)), isLeaf(true)
			{
			}

			BVHNode(const AABB& aabb, std::unique_ptr<BVHNode> left,
				std::unique_ptr<BVHNode> right)
				: aabb(aabb)
				, left(std::move(left))
				, right(std::move(right))
				, isLeaf(false)
			{
			}
		};

		std::unique_ptr<BVHNode> root;

		MeshBVH(void* vertices,
			const std::vector<uint32_t>& indices,
			const uint32_t vertexSize,
			int leafSize = 4);

		void Traverse(const std::function<void(BVHNode* node)>& callback) const;

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

		std::unique_ptr<BVHNode> BuildRecursive(std::vector<uint32_t> triangleIndices);

		glm::vec3 GetTriangleCentroid(uint32_t index) const;

		AABB ComputeBoundingBox(const std::vector<uint32_t>& triangleIndices);

		void ExpandAABB(AABB& aabb, const glm::vec3& point);

		AABB MergeAABBs(const AABB& a, const AABB& b);
	};

}
