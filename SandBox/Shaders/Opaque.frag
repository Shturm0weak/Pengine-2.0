#version 450

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 tangent;
layout(location = 2) in vec3 bitangent;
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
	vec4 albedo = texture(albedoTexture, uv) * material.albedoColor;
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
	outEmissive = texture(emissiveTexture, uv) * material.emissiveColor * material.emissiveFactor;

	vec3 normalViewSpace = gl_FrontFacing ? normal : -normal;
	normalViewSpace = normalize(normalViewSpace);
	if (material.useNormalMap > 0)
	{
		mat3 TBN = mat3(tangent, bitangent, normalViewSpace);
		vec3 normalMap = texture(normalTexture, uv).xyz;
		normalMap *= normalMap * 2.0f - 1.0f;
		outNormal = vec4(normalize(TBN * normalMap), 1.0f);
	}
	else
	{
		outNormal = vec4(normalViewSpace, 1.0f);
	}
}
