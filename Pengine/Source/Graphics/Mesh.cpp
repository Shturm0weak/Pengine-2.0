#include "Mesh.h"

#include "../Core/Logger.h"

using namespace Pengine;

Mesh::Mesh(const std::string& name, const std::string& filepath,
	std::vector<float>& vertices, std::vector<uint32_t>& indices)
	: Asset(name, filepath)
	, m_RawVertices(std::move(vertices))
	, m_RawIndices(std::move(indices))
{
	m_VertexCount = static_cast<int>(m_RawVertices.size());
	m_IndexCount = static_cast<int>(m_RawIndices.size());

	m_Vertices = Buffer::Create(
		sizeof(m_RawVertices[0]),
		m_RawVertices.size(),
		Buffer::Usage::VERTEX_BUFFER,
		Buffer::MemoryType::GPU);

	m_Vertices->WriteToBuffer(m_RawVertices.data(), m_Vertices->GetSize());

	m_Indices = Buffer::Create(
		sizeof(m_RawIndices[0]),
		m_RawIndices.size(),
		Buffer::Usage::INDEX_BUFFER,
		Buffer::MemoryType::GPU);

	m_Indices->WriteToBuffer(m_RawIndices.data(), m_Indices->GetSize());
}