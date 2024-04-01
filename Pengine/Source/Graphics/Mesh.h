#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

#include "Buffer.h"

namespace Pengine
{

	class PENGINE_API Mesh final : public Asset
	{
	public:
		Mesh(
			const std::string& name,
			const std::filesystem::path& filepath,
			std::vector<float>& vertices,
			std::vector<uint32_t>& indices);
		Mesh(const Mesh&) = delete;
		Mesh(Mesh&&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&&) = delete;

		[[nodiscard]] std::shared_ptr<Buffer> GetVertices() const { return m_Vertices; }

		[[nodiscard]] std::shared_ptr<Buffer> GetIndices() const { return m_Indices; }

		[[nodiscard]] std::shared_ptr<Buffer> GetVertexBuffer() const { return m_Vertices; }

		[[nodiscard]] std::shared_ptr<Buffer> GetIndexBuffer() const { return m_Indices; };

		[[nodiscard]] const std::vector<float>& GetRawVertices() const { return m_RawVertices; }

		[[nodiscard]] const std::vector<uint32_t>& GetRawIndices() const { return m_RawIndices; };

		[[nodiscard]] int GetVertexCount() const { return m_VertexCount; }

		[[nodiscard]] int GetIndexCount() const { return m_IndexCount; }

	protected:
		std::shared_ptr<Buffer> m_Vertices;
		std::shared_ptr<Buffer> m_Indices;
		std::vector<float> m_RawVertices;
		std::vector<uint32_t> m_RawIndices;
		int m_VertexCount = 0;
		int m_IndexCount = 0;
	};

}