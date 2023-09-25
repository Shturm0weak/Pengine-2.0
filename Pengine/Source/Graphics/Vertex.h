#pragma once

#include "../Core/Core.h"

#include "Texture.h"

namespace Pengine
{

	struct PENGINE_API Vertex
	{
		enum class InputRate
		{
			VERTEX,
			INSTANCE
		};

		struct BindingDescription
		{
			uint32_t binding;
			uint32_t stride;
			InputRate inputRate;
		};

		struct AttributeDescription
		{
			uint32_t binding;
			uint32_t location;
			Texture::Format format;
			uint32_t offset;
		};

		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;

		static std::vector<BindingDescription> GetDefaultVertexBindingDescriptions();

		static std::vector<AttributeDescription> GetDefaultVertexAttributeDescriptions();
	};

}