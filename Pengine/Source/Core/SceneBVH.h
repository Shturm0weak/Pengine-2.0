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
			BVHNode* left = nullptr;
			BVHNode* right = nullptr;
			std::shared_ptr<Entity> entity = nullptr;
			int subtreeSize = 0;

			[[nodiscard]] bool IsLeaf() const { return left == nullptr && right == nullptr; }

			~BVHNode()
			{
				delete left;
				delete right;

				left = nullptr;
				right = nullptr;
			}
		};

		explicit SceneBVH(Scene* scene) : m_Scene(scene) {}

		~SceneBVH() { Clear(); }

		void Clear();

		void Update();

		void Traverse(const std::function<bool(BVHNode*)>& callback) const;

		std::vector<entt::entity> CullAgainstFrustum(const std::array<glm::vec4, 6>& planes);

		std::multimap<Raycast::Hit, std::shared_ptr<Entity>> Raycast(
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length) const;

		[[nodiscard]] BVHNode* GetRoot() const { return m_Root; }

	private:
		BVHNode* m_Root = nullptr;
		Scene* m_Scene;

		/**
		 * Protection that there is currently no use (ray cast or traverse in progress) before rebuilding or clearing.
		 */
		std::mutex m_LockBVH;
		mutable std::condition_variable m_BVHConditionalVariable;
		mutable std::atomic<size_t> m_BVHUseCount;

		void WaitIdle();

		void Rebuild();

		BVHNode* BuildRecursive(std::vector<BVHNode*>& nodes, int start, int end);

		BVHNode* FindLeaf(BVHNode* node, std::shared_ptr<Entity> entity) const;

		BVHNode* FindParent(BVHNode* root, BVHNode* target) const;

		AABB LocalToWorldAABB(const AABB& localAABB, const glm::mat4& transformMat4);

		static bool IntersectsFrustum(const AABB& aabb, const std::array<glm::vec4, 6>& planes);
	};

}
