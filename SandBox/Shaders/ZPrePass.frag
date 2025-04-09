#version 450

layout(location = 0) in vec2 uv;

layout(set = 1, binding = 1) uniform sampler2D albedoTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D metalnessTexture;
layout(set = 1, binding = 4) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 5) uniform sampler2D aoTexture;
layout(set = 1, binding = 6) uniform sampler2D emissiveTexture;
layout(set = 1, binding = 7) uniform sampler2D shadingTexture;

#include "Shaders/Includes/DefaultMaterial.h"
layout(set = 1, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
};

void main()
{
	float alpha = texture(albedoTexture, uv).a;
	if (alpha < 0.01f)
	{
		discard;
	}
}
