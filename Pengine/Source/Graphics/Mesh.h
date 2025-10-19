#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"
#include "../Core/BoundingBox.h"
#include "../Core/Visualizer.h"

#include "Buffer.h"
#include "Vertex.h"
#include "MeshBVH.h"

namespace Pengine
{

	class PENGINE_API Mesh final : public Asset
	{
	public:
		enum class Type
		{
			STATIC,
			SKINNED
		};

		struct CreateInfo
		{
			struct SourceFileInfo
			{
				std::filesystem::path filepath;
				std::string meshName;
				uint32_t primitiveIndex;
			};

			SourceFileInfo sourceFileInfo;
			std::string name;
			std::filesystem::path filepath;
			std::vector<VertexLayout> vertexLayouts;
			uint32_t vertexCount = 0;
			uint32_t vertexSize = 0;
			void* vertices = nullptr;
			std::vector<uint32_t> indices;
			std::optional<BoundingBox> boundingBox;
			Type type = Type::STATIC;

			std::function<bool(
				const glm::vec3& start,
				const glm::vec3& direction,
				const float length,
				std::shared_ptr<MeshBVH> bvh,
				Raycast::Hit& hit,
				Visualizer& visualizer)> raycastCallback;
		};

		Mesh(const CreateInfo& createInfo);
		Mesh(const Mesh&) = delete;
		Mesh(Mesh&&) = delete;
		~Mesh();
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&&) = delete;

		[[nodiscard]] std::shared_ptr<Buffer> GetVertexBuffer(const size_t index) const;

		[[nodiscard]] std::shared_ptr<Buffer> GetIndexBuffer() const { return m_Indices; };

		[[nodiscard]] const void* GetRawVertices() const { return m_CreateInfo.vertices; }

		[[nodiscard]] const std::vector<uint32_t>& GetRawIndices() const { return m_CreateInfo.indices; };

		[[nodiscard]] uint32_t GetVertexCount() const { return m_CreateInfo.vertexCount; }

		[[nodiscard]] uint32_t GetIndexCount() const { return m_CreateInfo.indices.size(); }

		[[nodiscard]] uint32_t GetVertexSize() const { return m_CreateInfo.vertexSize; }

		[[nodiscard]] const BoundingBox& GetBoundingBox() const { return m_BoundingBox; }

		[[nodiscard]] const std::vector<VertexLayout>& GetVertexLayouts() const { return m_CreateInfo.vertexLayouts; }

		[[nodiscard]] Type GetType() const { return m_CreateInfo.type; }

		[[nodiscard]] const CreateInfo GetCreateInfo() const { return m_CreateInfo; }

		[[nodiscard]] std::shared_ptr<MeshBVH> GetBVH() const { return m_BVH; }

		[[nodiscard]] bool Raycast(
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length,
			Raycast::Hit& hit,
			Visualizer& visualizer) const;

		void Reload(const CreateInfo& createInfo);

	protected:
		std::shared_ptr<MeshBVH> m_BVH;
		std::vector<std::shared_ptr<Buffer>> m_Vertices;
		std::shared_ptr<Buffer> m_Indices;
		BoundingBox m_BoundingBox;
		CreateInfo m_CreateInfo;
	};

}