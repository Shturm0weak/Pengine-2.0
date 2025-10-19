#include "Mesh.h"

#include "../Core/Logger.h"
#include "../Core/Raycast.h"

using namespace Pengine;

Mesh::Mesh(const CreateInfo& createInfo)
	: Asset(createInfo.name, createInfo.filepath)
{
	Reload(createInfo);
}

Mesh::~Mesh()
{
	if (m_CreateInfo.vertices)
	{
		delete m_CreateInfo.vertices;
		m_CreateInfo.vertices = nullptr;
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
	Raycast::Hit& hit,
	Visualizer& visualizer) const
{
	return m_CreateInfo.raycastCallback(
		start,
		direction,
		length,
		m_BVH,
		hit,
		visualizer);
}

void Mesh::Reload(const CreateInfo& createInfo)
{
	if (m_CreateInfo.vertices)
	{
		delete m_CreateInfo.vertices;
		m_CreateInfo.vertices = nullptr;
	}

	// TODO: Maybe need to check whether createInfo is valid!
	m_CreateInfo = createInfo;

	m_Indices = Buffer::Create(
		sizeof(m_CreateInfo.indices[0]),
		m_CreateInfo.indices.size(),
		Buffer::Usage::INDEX_BUFFER,
		MemoryType::GPU);

	m_Indices->WriteToBuffer(m_CreateInfo.indices.data(), m_Indices->GetSize());

	std::vector<std::vector<uint8_t>> vertexBuffers;
	for (size_t i = 0; i < m_CreateInfo.vertexLayouts.size(); i++)
	{
		std::vector<uint8_t>& vertexBuffer = vertexBuffers.emplace_back();
		vertexBuffer.resize(m_CreateInfo.vertexLayouts[i].size * m_CreateInfo.vertexCount);
	}

	for (size_t i = 0; i < m_CreateInfo.vertexCount; i++)
	{
		uint32_t offset = 0;
		for (size_t j = 0; j < m_CreateInfo.vertexLayouts.size(); j++)
		{
			const size_t dstOffset = i * m_CreateInfo.vertexLayouts[j].size;
			const size_t srcOffset = offset + i * m_CreateInfo.vertexSize;
			memcpy(
				vertexBuffers[j].data() + dstOffset,
				(uint8_t*)createInfo.vertices + srcOffset,
				m_CreateInfo.vertexLayouts[j].size);

			offset += m_CreateInfo.vertexLayouts[j].size;
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
		for (size_t i = 0; i < m_CreateInfo.vertexCount; i++)
		{
			const glm::vec3* vertexPosition = (glm::vec3*)((uint8_t*)m_CreateInfo.vertices + i * m_CreateInfo.vertexSize);

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

	if (m_CreateInfo.raycastCallback)
	{
		m_CreateInfo.raycastCallback = createInfo.raycastCallback;
	}
	else
	{
		m_CreateInfo.raycastCallback = [](
			const glm::vec3& start,
			const glm::vec3& direction,
			const float length,
			std::shared_ptr<MeshBVH> bvh,
			Raycast::Hit& hit,
			Visualizer& visualizer) -> bool
			{
				//const VertexDefault* vertex = (const VertexDefault*)vertices;
				//for (size_t i = 0; i < indices.size(); i += 3)
				//{
				//	const glm::vec3& vertex0 = vertex[indices[i + 0]].position;
				//	const glm::vec3& vertex1 = vertex[indices[i + 1]].position;
				//	const glm::vec3& vertex2 = vertex[indices[i + 2]].position;

				//	const glm::vec3 a = transform * glm::vec4(vertex0, 1.0f);
				//	const glm::vec3 b = transform * glm::vec4(vertex1, 1.0f);
				//	const glm::vec3 c = transform * glm::vec4(vertex2, 1.0f);

				//	const glm::vec3 normal = glm::normalize(glm::cross((b - a), (c - a)));

				//	Raycast::Hit hit{};
				//	if (Raycast::IntersectTriangle(start, direction, a, b, c, normal, length, hit))
				//	{
				//		/*visualizer.DrawLine(a, b, { 1.0f, 0.0f, 1.0f }, 5.0f);
				//		visualizer.DrawLine(a, c, { 1.0f, 0.0f, 1.0f }, 5.0f);
				//		visualizer.DrawLine(c, b, { 1.0f, 0.0f, 1.0f }, 5.0f);*/

				//		distance = hit.distance;
				//		return true;
				//	}
				//}

				//return false;

				return bvh->Raycast(start, direction, length, hit, visualizer);
			};
	}

	m_BVH = std::make_shared<MeshBVH>(m_CreateInfo.vertices, m_CreateInfo.indices, m_CreateInfo.vertexSize);
}
