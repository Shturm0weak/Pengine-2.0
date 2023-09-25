#include "Mesh.h"

#include "../Core/Logger.h"

using namespace Pengine;

Mesh::Mesh(const std::string& name, const std::string& filepath,
	std::vector<float>& vertices, std::vector<uint32_t>& indices)
	: Asset(name, filepath)
	, m_RawVertices(std::move(vertices))
	, m_RawIndices(std::move(indices))
{
	m_VertexCount = m_RawVertices.size();
	m_IndexCount = m_RawIndices.size();
	std::shared_ptr<Buffer> stagingBuffer = Buffer::Create(sizeof(m_RawVertices[0]), m_RawVertices.size(),
		std::vector<Buffer::Usage>{ Buffer::Usage::TRANSFER_SRC });

	stagingBuffer->WriteToBuffer((void*)m_RawVertices.data());

	m_Vertices = Buffer::Create(sizeof(m_RawVertices[0]), m_RawVertices.size(),
		std::vector<Buffer::Usage>{ Buffer::Usage::TRANSFER_DST, 
		Buffer::Usage::VERTEX_BUFFER});

	m_Vertices->Copy(stagingBuffer);
	stagingBuffer.reset();

	stagingBuffer = Buffer::Create(sizeof(m_RawIndices[0]), m_RawIndices.size(),
		std::vector<Buffer::Usage>{ Buffer::Usage::TRANSFER_SRC });

	stagingBuffer->WriteToBuffer((void*)m_RawIndices.data());

	m_Indices = Buffer::Create(sizeof(m_RawIndices[0]), m_RawIndices.size(),
		std::vector<Buffer::Usage>{ Buffer::Usage::TRANSFER_DST,
		Buffer::Usage::INDEX_BUFFER });

	m_Indices->Copy(stagingBuffer);
	stagingBuffer.reset();
}