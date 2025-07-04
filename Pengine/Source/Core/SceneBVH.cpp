#include "SceneBVH.h"

#include "Scene.h"
#include "Profiler.h"

#include "../Components/Transform.h"
#include "../Components/Renderer3D.h"

#include "../Graphics/Mesh.h"

using namespace Pengine;

void SceneBVH::Clear()
{
	WaitIdle();

	delete m_Root;
	m_Root = nullptr;
	m_Scene = nullptr;
}

void SceneBVH::Update()
{
	WaitIdle();

	if (!m_Scene)
	{
		return;
	}

	if (m_Scene->GetEntities().empty())
	{
		delete m_Root;
		m_Root = nullptr;
		return;
	}

	Rebuild();
}

void SceneBVH::Traverse(const std::function<void(BVHNode*)>& callback) const
{
	if (!m_Root) return;

	m_BVHUseCount.fetch_add(1);

	std::stack<BVHNode*> nodeStack;
	nodeStack.push(m_Root);

	while (!nodeStack.empty())
	{
		BVHNode* currentNode = nodeStack.top();
		nodeStack.pop();

		callback(currentNode);

		if (currentNode->right) nodeStack.push(currentNode->right);
		if (currentNode->left) nodeStack.push(currentNode->left);
	}

	m_BVHUseCount.fetch_sub(1);
	m_BVHConditionalVariable.notify_all();
}

std::multimap<Raycast::Hit, std::shared_ptr<Entity>> SceneBVH::Raycast(
	const glm::vec3& start,
	const glm::vec3& direction,
	const float length) const
{
	std::multimap<Raycast::Hit, std::shared_ptr<Entity>> hits;
	if (!m_Root) return hits;

	m_BVHUseCount.fetch_add(1);

	std::stack<BVHNode*> nodeStack;
	nodeStack.push(m_Root);

	while (!nodeStack.empty())
	{
		BVHNode* currentNode = nodeStack.top();
		nodeStack.pop();

		Raycast::Hit hit;
		if (!Raycast::IntersectBoxAABB(start, direction, currentNode->aabb.min, currentNode->aabb.max, length, hit)) continue;

		if (currentNode->IsLeaf())
		{
			if (currentNode->entity && currentNode->entity->IsEnabled())
			{
				hits.emplace(hit, currentNode->entity);
			}
		}
		else
		{
			if (currentNode->left) nodeStack.push(currentNode->left);
			if (currentNode->right) nodeStack.push(currentNode->right);
		}
	}

	m_BVHUseCount.fetch_sub(1);
	m_BVHConditionalVariable.notify_all();

	return hits;
}

void SceneBVH::WaitIdle()
{
	std::unique_lock<std::mutex> lock(m_LockBVH);
	m_BVHConditionalVariable.wait(lock, [this]
	{
		return m_BVHUseCount.load() == 0;
	});
}

void SceneBVH::Rebuild()
{
	PROFILER_SCOPE(__FUNCTION__);
	if (!m_Scene)
	{
		return;
	}

	delete m_Root;
	m_Root = nullptr;

	const auto r3dView = m_Scene->GetRegistry().view<Renderer3D>();
	std::vector<BVHNode*> nodes;
	nodes.reserve(r3dView.size());

	for (auto entity : r3dView)
	{
		const Transform& transform = m_Scene->GetRegistry().get<Transform>(entity);
		if (!transform.GetEntity()->IsEnabled())
		{
			continue;
		}

		const Renderer3D& r3d = m_Scene->GetRegistry().get<Renderer3D>(entity);
		if (!r3d.mesh || !r3d.isEnabled)
		{
			continue;
		}

		BVHNode* node = new BVHNode();
		node->aabb = LocalToWorldAABB({ r3d.mesh->GetBoundingBox().min, r3d.mesh->GetBoundingBox().max }, transform.GetTransform());
		node->entity = transform.GetEntity();
		node->subtreeSize = 1;
		nodes.emplace_back(node);
	}

	m_Root = BuildRecursive(nodes, 0, nodes.size());
}

SceneBVH::BVHNode* SceneBVH::BuildRecursive(std::vector<BVHNode*>& nodes, int start, int end)
{
	const int count = end - start;
	if (count == 0) return nullptr;

	if (count == 1)
	{
		return nodes[start];
	}

	AABB centroidBounds = nodes[start]->aabb;
	for (int i = start + 1; i < end; i++)
	{
		centroidBounds = centroidBounds.Expanded(nodes[i]->aabb);
	}

	// Choose split axis (longest).
	int axis = 0;
	glm::vec3 extent =
	{
		centroidBounds.max.x - centroidBounds.min.x,
		centroidBounds.max.y - centroidBounds.min.y,
		centroidBounds.max.z - centroidBounds.min.z
	};
	if (extent.y > extent.x) axis = 1;
	if (extent.z > extent[axis]) axis = 2;

	// Partition using SAH.
	const int BIN_COUNT = 12;
	struct Bin
	{
		AABB bounds;
		int count = 0;
	} bins[BIN_COUNT];

	// Initialize bins.
	float scale = BIN_COUNT / extent[axis];
	for (int i = start; i < end; i++)
	{
		int binIdx = std::min(BIN_COUNT - 1,
			static_cast<int>((nodes[i]->aabb.Center()[axis] - centroidBounds.min[axis]) * scale));
		bins[binIdx].count++;
		bins[binIdx].bounds = bins[binIdx].bounds.Expanded(nodes[i]->aabb);
	}

	// Find best split.
	float bestCost = std::numeric_limits<float>::infinity();
	int bestSplit = -1;

	for (int i = 1; i < BIN_COUNT; i++)
	{
		AABB leftAABB, rightAABB;
		int leftCount = 0, rightCount = 0;

		// Accumulate left bins.
		for (int j = 0; j < i; j++)
		{
			if (bins[j].count == 0) continue;
			leftAABB = leftAABB.Expanded(bins[j].bounds);
			leftCount += bins[j].count;
		}

		// Accumulate right bins.
		for (int j = i; j < BIN_COUNT; j++)
		{
			if (bins[j].count == 0) continue;
			rightAABB = rightAABB.Expanded(bins[j].bounds);
			rightCount += bins[j].count;
		}

		// Calculate SAH cost.
		float cost = leftCount * leftAABB.SurfaceArea() + rightCount * rightAABB.SurfaceArea();

		if (cost < bestCost)
		{
			bestCost = cost;
			bestSplit = i;
		}
	}

	// Partition nodes.
	auto midIter = std::partition(nodes.begin() + start, nodes.begin() + end,
		[&](BVHNode* node)
		{
			int binIdx = std::min(BIN_COUNT - 1,
				static_cast<int>((node->aabb.Center()[axis] - centroidBounds.min[axis]) * scale));
			return binIdx < bestSplit;
		});

	int mid = midIter - nodes.begin();

	// Handle bad splits.
	if (mid == start || mid == end)
	{
		mid = start + count / 2;
	}

	// Recursively build children.
	BVHNode* node = new BVHNode();
	node->left = BuildRecursive(nodes, start, mid);
	node->right = BuildRecursive(nodes, mid, end);

	// Combine AABBs.
	node->aabb = node->left->aabb.Expanded(node->right->aabb);
	node->subtreeSize = node->left->subtreeSize + node->right->subtreeSize;

	return node;
}

SceneBVH::BVHNode* SceneBVH::FindLeaf(BVHNode* node, std::shared_ptr<Entity> entity) const
{
	if (!node) return nullptr;
	if (node->entity == entity) return node;

	if (BVHNode* left = FindLeaf(node->left, entity)) return left;
	return FindLeaf(node->right, entity);
}

SceneBVH::BVHNode* SceneBVH::FindParent(BVHNode* root, BVHNode* target) const
{
	if (!root || root == target) return nullptr;
	if (root->left == target || root->right == target) return root;

	if (BVHNode* left = FindParent(root->left, target)) return left;
	return FindParent(root->right, target);
}

AABB SceneBVH::LocalToWorldAABB(const AABB& localAABB, const glm::mat4& transformMat4)
{
	const glm::vec3& min = localAABB.min;
	const glm::vec3& max = localAABB.max;

	const std::array<glm::vec3, 8> corners =
	{
		{
			{ min.x,  min.y,  min.z },
			{ max.x,  min.y,  min.z },
			{ min.x,  max.y,  min.z },
			{ max.x,  max.y,  min.z },
			{ min.x,  min.y,  max.z },
			{ max.x,  min.y,  max.z },
			{ min.x,  max.y,  max.z },
			{ max.x,  max.y,  max.z } 
		}
	};

	glm::vec3 transformed = transformMat4 * glm::vec4(corners[0], 1.0f);
	glm::vec3 worldMin = transformed;
	glm::vec3 worldMax = transformed;

	for (size_t i = 1; i < 8; ++i)
	{
		transformed = transformMat4 * glm::vec4(corners[i], 1.0f);
		worldMin = glm::min(worldMin, transformed);
		worldMax = glm::max(worldMax, transformed);
	}

	return { worldMin, worldMax };
}
