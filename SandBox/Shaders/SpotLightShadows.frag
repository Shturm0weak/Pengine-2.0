#version 450 core

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 positionWorldSpace;
layout(location = 2) flat in vec3 lightPositionWorldSpace;
layout(location = 3) flat in float radius;

layout(set = 0, binding = 1) uniform sampler2D albedoTexture;
layout(set = 0, binding = 2) uniform sampler2D normalTexture;
layout(set = 0, binding = 3) uniform sampler2D metallicRoughnessTexture;
layout(set = 0, binding = 4) uniform sampler2D aoTexture;
layout(set = 0, binding = 5) uniform sampler2D emissiveTexture;
layout(set = 0, binding = 6) uniform sampler2D heightTexture;

#include "Shaders/Includes/DefaultMaterial.h"
layout(set = 0, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
};

void main()
{
	if (material.useAlphaCutoff > 0)
	{
		if (texture(albedoTexture, uv).a < material.alphaCutoff)
		{
			discard;
		}
	}

    float lightDistance = length(positionWorldSpace.xyz - lightPositionWorldSpace);
    lightDistance = lightDistance / radius;
    gl_FragDepth = lightDistance;
}
