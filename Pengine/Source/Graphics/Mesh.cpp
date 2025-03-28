#include "Mesh.h"

#include "../Core/Logger.h"
#include "../Core/Raycast.h"

using namespace Pengine;

Mesh::Mesh(
	CreateInfo& createInfo)
	: Asset(createInfo.name, createInfo.filepath)
	, m_RawVertices(createInfo.vertices)
	, m_RawIndices(std::move(createInfo.indices))
	, m_VertexCount(createInfo.vertexCount)
	, m_VertexSize(createInfo.vertexSize)
	, m_IndexCount(m_RawIndices.size())
	, m_VertexLayouts(std::move(createInfo.vertexLayouts))
	, m_Type(createInfo.type)
{
	m_Indices = Buffer::Create(
		sizeof(m_RawIndices[0]),
		m_RawIndices.size(),
		Buffer::Usage::INDEX_BUFFER,
		MemoryType::GPU);

	m_Indices->WriteToBuffer(m_RawIndices.data(), m_Indices->GetSize());

	std::vector<std::vector<uint8_t>> vertexBuffers;
	for (size_t i = 0; i < m_VertexLayouts.size(); i++)
	{
		std::vector<uint8_t>& vertexBuffer = vertexBuffers.emplace_back();
		vertexBuffer.resize(m_VertexLayouts[i].size * m_VertexCount);
	}

	for (size_t i = 0; i < m_VertexCount; i++)
	{
		uint32_t offset = 0;
		for (size_t j = 0; j < m_VertexLayouts.size(); j++)
		{
			const size_t dstOffset = i * m_VertexLayouts[j].size;
			const size_t srcOffset = offset + i * m_VertexSize;
			memcpy(
				vertexBuffers[j].data() + dstOffset,
				(uint8_t*)createInfo.vertices + srcOffset,
				m_VertexLayouts[j].size);

			offset += m_VertexLayouts[j].size;
		}
	}

	for (std::vector<uint8_t>& vertexBuffer : vertexBuffers)
	{
		m_Vertices.emplace_back(Buffer::Create(
			sizeof(vertexBuffer[0]),
			vertexBuffer.size(),
			Buffer::Usage::VERTEX_BUFFER,
			MemoryType::GPU));

		m_Vertices.back()->WriteToBuffer(vertexBuffer.data(), vertexBuffer.size());
	}

	if (createInfo.boundingBox)
	{
		m_BoundingBox = *createInfo.boundingBox;
	}
	else
	{
		for (size_t i = 0; i < m_VertexCount; i++)
		{
			const glm::vec3* vertexPosition = (glm::vec3*)((uint8_t*)m_RawVertices + i * m_VertexSize);

			if (vertexPosition->x < m_BoundingBox.min.x)
			{
				m_BoundingBox.min.x = vertexPosition->x;
			}

			if (vertexPosition->y < m_BoundingBox.min.y)
			{
				m_BoundingBox.min.y = vertexPosition->y;
			}

			if (vertexPosition->z < m_BoundingBox.min.z)
			{
				m_BoundingBox.min.z = vertexPosition->z;
			}

			if (vertexPosition->x > m_BoundingBox.max.x)
			{
				m_BoundingBox.max.x = vertexPosition->x;
			}

			if (vertexPosition->y > m_BoundingBox.max.y)
			{
				m_BoundingBox.max.y = vertexPosition->y;
			}

			if (vertexPosition->z > m_BoundingBox.max.z)
			{
				m_BoundingBox.max.z = vertexPosition->z;
			}
		}

		m_BoundingBox.offset = m_BoundingBox.max + (m_BoundingBox.min - m_BoundingBox.max) * 0.5f;
	}

	if (createInfo.raycastCallback)
	{
		m_RaycastCallback = createInfo.raycastCallback;
	}
	else
	{
		m_RaycastCallback = [](
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length,
			const glm::mat4& transform,
			const void* vertices,
			const uint32_t vertexCount,
			std::vector<uint32_t> indices,
			float& distance,
			Visualizer& visualizer) -> bool
		{
			const VertexDefault* vertex = (const VertexDefault*)vertices;
			for (size_t i = 0; i < indices.size(); i += 3)
			{
				const glm::vec3& vertex0 = vertex[indices[i + 0]].position;
				const glm::vec3& vertex1 = vertex[indices[i + 1]].position;
				const glm::vec3& vertex2 = vertex[indices[i + 2]].position;

				const glm::vec3 a = transform * glm::vec4(vertex0, 1.0f);
				const glm::vec3 b = transform * glm::vec4(vertex1, 1.0f);
				const glm::vec3 c = transform * glm::vec4(vertex2, 1.0f);

				const glm::vec3 normal = glm::normalize(glm::cross((b - a), (c - a)));

				Raycast::Hit hit{};
				if (Raycast::IntersectTriangle(start, direction, a, b, c, normal, length, hit))
				{
					/*visualizer.DrawLine(a, b, { 1.0f, 0.0f, 1.0f }, 5.0f);
					visualizer.DrawLine(a, c, { 1.0f, 0.0f, 1.0f }, 5.0f);
					visualizer.DrawLine(c, b, { 1.0f, 0.0f, 1.0f }, 5.0f);*/

					distance = hit.distance;
					return true;
				}
			}

			return false;
		};
	}
}

Mesh::~Mesh()
{
	if (m_RawVertices)
	{
		delete m_RawVertices;
		m_RawVertices = nullptr;
	}
}

std::shared_ptr<Buffer> Mesh::GetVertexBuffer(const size_t index) const
{
	if (index >= m_Vertices.size())
	{
		const std::string message = GetFilepath().string() + ":doesn't have enough vertex buffers!";
		FATAL_ERROR(message);
	}
	
	return m_Vertices[index];
}

bool Mesh::Raycast(
	const glm::vec3& start,
	const glm::vec3& direction,
	const float length,
	const glm::mat4& transform,
	float& distance,
	Visualizer& visualizer) const
{
	return m_RaycastCallback(
		start,
		direction,
		length,
		transform,
		m_RawVertices,
		m_VertexCount,
		m_RawIndices,
		distance,
		visualizer);
}
