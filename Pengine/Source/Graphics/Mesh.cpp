#include "Mesh.h"

#include "Vertex.h"

#include "../Core/Logger.h"

using namespace Pengine;

Mesh::Mesh(
	const std::string& name,
	const std::filesystem::path& filepath,
	std::vector<float>& vertices,
	std::vector<uint32_t>& indices)
	: Asset(name, filepath)
	, m_RawVertices(std::move(vertices))
	, m_RawIndices(std::move(indices))
{
	m_VertexCount = static_cast<int>(m_RawVertices.size());
	m_IndexCount = static_cast<int>(m_RawIndices.size());

	Vertex* vertex = static_cast<Vertex*>((void*)m_RawVertices.data());
	const int vertexCount = m_VertexCount / (sizeof(Vertex) / sizeof(float));
	for (size_t i = 0; i < vertexCount; i++)
	{
		if (vertex->position.x < m_BoundingBox.min.x)
		{
			m_BoundingBox.min.x = vertex->position.x;
		}

		if (vertex->position.y < m_BoundingBox.min.y)
		{
			m_BoundingBox.min.y = vertex->position.y;
		}

		if (vertex->position.z < m_BoundingBox.min.z)
		{
			m_BoundingBox.min.z = vertex->position.z;
		}

		if (vertex->position.x > m_BoundingBox.max.x)
		{
			m_BoundingBox.max.x = vertex->position.x;
		}

		if (vertex->position.y > m_BoundingBox.max.y)
		{
			m_BoundingBox.max.y = vertex->position.y;
		}

		if (vertex->position.z > m_BoundingBox.max.z)
		{
			m_BoundingBox.max.z = vertex->position.z;
		}

		vertex++;
	}

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