#version 450

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outShading;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D metalnessTexture;
layout(set = 1, binding = 3) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D aoTexture;

#include "Shaders/Includes/DefaultMaterial.h"

layout(set = 1, binding = 5) uniform GBufferMaterial
{
	DefaultMaterial material;
};

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

void main()
{
	vec4 albedo = texture(albedoTexture, uv) * material.color;
	if (albedo.a < 0.01f)
	{
		discard;
	}

	float metallic = texture(metalnessTexture, uv).r;
	float roughness = texture(roughnessTexture, uv).r;
	float ao = texture(aoTexture, uv).r;

	outAlbedo = albedo;
	outShading = vec4(
		metallic * material.metallicFactor,
		roughness * material.roughnessFactor,
		ao * material.aoFactor,
		1.0f);
		
	if (material.useNormalMap > 0)
	{
		vec3 normalMap = texture(normalTexture, uv).xyz;
		normalMap *= normalMap * 2.0f - 1.0f;
		outNormal = vec4(normalize(TBN * normalMap), 1.0f);
	}
	else
	{
		outNormal = vec4(normalize(normal), 1.0f);
	}
}
