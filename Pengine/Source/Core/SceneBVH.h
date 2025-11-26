#pragma once

#include "Core.h"
#include "Raycast.h"
#include "BoundingBox.h"

namespace Pengine
{
	class Scene;
	class Entity;

	class PENGINE_API SceneBVH
	{
	public:

		struct BVHNode
		{
			AABB aabb;
			uint32_t left = -1;
			uint32_t right = -1;
			std::shared_ptr<Entity> entity;
			int subtreeSize = 0;

			[[nodiscard]] bool IsLeaf() const { return left == -1 && right == -1; }
		};

		explicit SceneBVH(Scene* scene) : m_Scene(scene) {}

		~SceneBVH() { Clear(); }

		void Clear();

		void Update();

		void Traverse(const std::function<bool(const BVHNode&)>& callback) const;

		std::vector<entt::entity> CullAgainstFrustum(const std::array<glm::vec4, 6>& planes);

		std::multimap<Raycast::Hit, std::shared_ptr<Entity>> Raycast(
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length) const;

		[[nodiscard]] std::optional<BVHNode> GetRoot() const { return m_Root == -1 ? std::nullopt : std::optional<BVHNode>(m_Nodes[m_Root]); }

	private:
		Scene* m_Scene;

		uint32_t m_Root = -1;
		std::vector<BVHNode> m_Nodes;

		/**
		 * Protection that there is currently no use (ray cast or traverse in progress) before rebuilding or clearing.
		 */
		std::mutex m_LockBVH;
		mutable std::condition_variable m_BVHConditionalVariable;
		mutable std::atomic<size_t> m_BVHUseCount;

		void WaitIdle();

		void Rebuild();

		uint32_t BuildRecursive(int start, int end);

		//BVHNode* FindLeaf(BVHNode* node, std::shared_ptr<Entity> entity) const;

		//BVHNode* FindParent(BVHNode* root, BVHNode* target) const;

		AABB LocalToWorldAABB(const AABB& localAABB, const glm::mat4& transformMat4);

		static bool IntersectsFrustum(const AABB& aabb, const std::array<glm::vec4, 6>& planes);
	};

}
