#include "Mesh.h"

#include "Vertex.h"

#include "../Core/Logger.h"

using namespace Pengine;

Mesh::Mesh(
	const std::string& name,
	const std::filesystem::path& filepath,
	const size_t vertexSize,
	std::vector<float>& vertices,
	std::vector<uint32_t>& indices)
	: Asset(name, filepath)
	, m_RawVertices(std::move(vertices))
	, m_RawIndices(std::move(indices))
{
	m_VertexCount = static_cast<int>(m_RawVertices.size() / (vertexSize / sizeof(float)));
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

	// Only meshes that have Vertex structure vertices can have bounding boxes and shadows.
	if (vertexSize != sizeof(Vertex))
	{
		return;
	}

	std::vector<float> rawVerticesForShadows;
	rawVerticesForShadows.resize(m_VertexCount * (sizeof(VertexForShadows) / sizeof(float)));

	Vertex* vertex = static_cast<Vertex*>((void*)m_RawVertices.data());
	VertexForShadows* vertexForShadows = static_cast<VertexForShadows*>((void*)rawVerticesForShadows.data());
	for (size_t i = 0; i < m_VertexCount; i++)
	{
		vertexForShadows->position = vertex->position;
		vertexForShadows->uv = vertex->uv;

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
		vertexForShadows++;
	}

	m_VerticesForShadows = Buffer::Create(
		sizeof(rawVerticesForShadows[0]),
		rawVerticesForShadows.size(),
		Buffer::Usage::VERTEX_BUFFER,
		Buffer::MemoryType::GPU);

	m_VerticesForShadows->WriteToBuffer(rawVerticesForShadows.data(), rawVerticesForShadows.size() * sizeof(float));
}