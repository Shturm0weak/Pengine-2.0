#version 450 core

layout(location = 0) in vec2 uv;

layout(set = 2, binding = 0) uniform sampler2D bindlessTextures[10000];

#include "Shaders/Includes/DefaultMaterial.h"
layout(set = 1, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
};

void main()
{
	if (material.useAlphaCutoff > 0)
	{
		if (texture(bindlessTextures[material.albedoTexture], uv).a < material.alphaCutoff)
		{
			discard;
		}
	}
}
