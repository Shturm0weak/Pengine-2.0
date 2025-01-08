#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"
#include "../Core/BoundingBox.h"
#include "../Core/Visualizer.h"

#include "Buffer.h"
#include "Vertex.h"

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
			std::string name;
			std::filesystem::path filepath;
			std::vector<VertexLayout> vertexLayouts;
			uint32_t vertexCount;
			uint32_t vertexSize;
			void* vertices;
			std::vector<uint32_t> indices;
			std::optional<BoundingBox> boundingBox;
			Type type = Type::STATIC;

			std::function<bool(
				const glm::vec3& start,
				const glm::vec3& direction,
				const float length,
				const glm::mat4& transform,
				const void* vertices,
				const uint32_t vertexCount,
				std::vector<uint32_t> indices,
				float& distance,
				Visualizer& visualizer)> raycastCallback;
		};

		Mesh(CreateInfo& createInfo);
		Mesh(const Mesh&) = delete;
		Mesh(Mesh&&) = delete;
		~Mesh();
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&&) = delete;

		[[nodiscard]] std::shared_ptr<Buffer> GetVertexBuffer(const size_t index) const;

		[[nodiscard]] std::shared_ptr<Buffer> GetIndexBuffer() const { return m_Indices; };

		[[nodiscard]] const void* GetRawVertices() const { return m_RawVertices; }

		[[nodiscard]] const std::vector<uint32_t>& GetRawIndices() const { return m_RawIndices; };

		[[nodiscard]] uint32_t GetVertexCount() const { return m_VertexCount; }

		[[nodiscard]] uint32_t GetIndexCount() const { return m_IndexCount; }

		[[nodiscard]] uint32_t GetVertexSize() const { return m_VertexSize; }

		[[nodiscard]] const BoundingBox& GetBoundingBox() const { return m_BoundingBox; }

		[[nodiscard]] const std::vector<VertexLayout>& GetVertexLayouts() const { return m_VertexLayouts; }

		[[nodiscard]] Type GetType() const { return m_Type; }

		[[nodiscard]] bool Raycast(
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length,
			const glm::mat4& transform,
			float& distance,
			Visualizer& visualizer) const;

	protected:
		std::vector<std::shared_ptr<Buffer>> m_Vertices;
		std::shared_ptr<Buffer> m_Indices;
		void* m_RawVertices;
		std::vector<uint32_t> m_RawIndices;
		BoundingBox m_BoundingBox;
		uint32_t m_VertexCount = 0;
		uint32_t m_IndexCount = 0;
		uint32_t m_VertexSize = 0;
		std::vector<VertexLayout> m_VertexLayouts;
		Type m_Type = Type::STATIC;
		std::function<bool(
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length,
			const glm::mat4& transform,
			const void* vertices,
			const uint32_t vertexCount,
			std::vector<uint32_t> indices,
			float& distance,
			Visualizer& visualizer)> m_RaycastCallback;
	};

}