#pragma once

#include "../Core/Core.h"
#include "../Core/Asset.h"

#include "Buffer.h"
#include "RenderPass.h"
#include "Texture.h"

namespace Pengine
{

	class PENGINE_API Mesh : public Asset
	{
	public:
		Mesh(const std::string& name, const std::string& filepath,
			std::vector<float>& vertices, std::vector<uint32_t>& indices);
		virtual ~Mesh() = default;
		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		std::shared_ptr<Buffer> GetVertices() const { return m_Vertices; }

		std::shared_ptr<Buffer> GetIndices() const { return m_Indices; }

		std::shared_ptr<Buffer> GetVertexBuffer() const { return m_Vertices; }

		std::shared_ptr<Buffer> GetIndexBuffer() const { return m_Indices; };

		const std::vector<float>& GetRawVertices() const { return m_RawVertices; }

		const std::vector<uint32_t>& GetRawIndices() const { return m_RawIndices; };

		int GetVertexCount() const { return m_VertexCount; }

		int GetIndexCount() const { return m_IndexCount; }

	protected:
		std::shared_ptr<Buffer> m_Vertices;
		std::shared_ptr<Buffer> m_Indices;
		std::vector<float> m_RawVertices;
		std::vector<uint32_t> m_RawIndices;
		int m_VertexCount = 0;
		int m_IndexCount = 0;
	};

}