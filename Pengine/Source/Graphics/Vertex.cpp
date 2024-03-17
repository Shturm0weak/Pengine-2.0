#include "Vertex.h"

using namespace Pengine;

std::vector<Vertex::BindingDescription> Vertex::GetDefaultVertexBindingDescriptions()
{
	return
	{
		{ 0, sizeof(Vertex), InputRate::VERTEX }
	};
}

std::vector<Vertex::AttributeDescription> Vertex::GetDefaultVertexAttributeDescriptions()
{
	return
	{
		{ 0, 0, Texture::Format::R32G32B32_SFLOAT, offsetof(Vertex, position) },
		{ 0, 1, Texture::Format::R32G32_SFLOAT, offsetof(Vertex, uv) },
		{ 0, 2, Texture::Format::R32G32B32_SFLOAT, offsetof(Vertex, normal) },
		{ 0, 3, Texture::Format::R32G32B32_SFLOAT, offsetof(Vertex, tangent) },
		{ 0, 4, Texture::Format::R32G32B32_SFLOAT, offsetof(Vertex, bitangent) }
	};
}