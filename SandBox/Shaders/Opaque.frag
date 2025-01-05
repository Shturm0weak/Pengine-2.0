#version 450

layout(location = 0) in vec3 normalViewSpace;
layout(location = 1) in vec3 tangentViewSpace;
layout(location = 2) in vec3 bitangentViewSpace;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outShading;
layout(location = 3) out vec4 outEmissive;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D metalnessTexture;
layout(set = 1, binding = 3) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D aoTexture;
layout(set = 1, binding = 5) uniform sampler2D emissiveTexture;

#include "Shaders/Includes/DefaultMaterial.h"

layout(set = 1, binding = 6) uniform GBufferMaterial
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
	vec4 albedoColor = texture(albedoTexture, uv) * material.albedoColor;
	if (albedoColor.a < 0.01f)
	{
		discard;
	}

	float metallic = texture(metalnessTexture, uv).r;
	float roughness = texture(roughnessTexture, uv).r;
	float ao = texture(aoTexture, uv).r;

	outAlbedo = albedoColor;
	outShading = vec4(
		metallic * material.metallicFactor,
		roughness * material.roughnessFactor,
		ao * material.aoFactor,
		1.0f);
	outEmissive = texture(emissiveTexture, uv) * material.emissiveColor * material.emissiveFactor;

	vec3 normal = gl_FrontFacing ? normalViewSpace : -normalViewSpace;
	normal = normalize(normal);
	if (material.useNormalMap > 0)
	{
		mat3 TBN = mat3(normalize(tangentViewSpace), normalize(bitangentViewSpace), normal);
		normal = texture(normalTexture, uv).xyz;
		normal = normal * 2.0f - 1.0f;
		outNormal = vec4(normalize(TBN * normal), 1.0f);
	}
	else
	{
		outNormal = vec4(normal, 1.0f);
	}
}
