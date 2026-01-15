#version 450

layout(location = 0) in vec3 normalViewSpace;
layout(location = 1) in vec3 tangentViewSpace;
layout(location = 2) in vec3 bitangentViewSpace;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec4 color;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec2 outNormal;
layout(location = 2) out vec4 outShading;
layout(location = 3) out vec4 outEmissive;

#include "Shaders/Includes/DefaultMaterial.h"
layout(set = 1, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
	vec2 size;
	vec4 firstColor;
	vec4 secondColor;
};

#include "Shaders/Includes/Camera.h"
layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

layout(set = 2, binding = 0) uniform sampler2D bindlessTextures[10000];

void main()
{
	ivec2 coords = ivec2(size * uv) % 2;

	vec4 checkerBoardColor = secondColor;
	if (coords.x == coords.y)
	{
		checkerBoardColor = firstColor;
	}

	vec4 albedoColor = checkerBoardColor * texture(bindlessTextures[material.albedoTexture], uv) * material.albedoColor * color;
	if (material.useAlphaCutoff > 0)
	{
		if (albedoColor.a < material.alphaCutoff)
		{
			discard;
		}
	}

	vec3 metallicRoughness = texture(bindlessTextures[material.metallicRoughnessTexture], uv).xyz;
	float metallic = metallicRoughness.b;
	float roughness = metallicRoughness.g;
	float ao = texture(bindlessTextures[material.aoTexture], uv).r;

	outAlbedo = albedoColor;
	outShading = vec4(
		metallic * material.metallicFactor,
		roughness * material.roughnessFactor,
		ao * material.aoFactor,
		1.0f);
	outEmissive = texture(bindlessTextures[material.emissiveTexture], uv) * material.emissiveColor * material.emissiveFactor;

	vec3 normalViewSpaceFinal = gl_FrontFacing ? normalViewSpace : -normalViewSpace;
	normalViewSpaceFinal = normalize(normalViewSpaceFinal);
	if (material.useNormalMap > 0)
	{
		mat3 TBN = mat3(normalize(tangentViewSpace), normalize(bitangentViewSpace), normalViewSpaceFinal);
		normalViewSpaceFinal = texture(bindlessTextures[material.normalTexture], uv).xyz;
		normalViewSpaceFinal = normalViewSpaceFinal * 2.0f - 1.0f;
		outNormal = OctEncode(normalize(TBN * normalViewSpaceFinal));
	}
	else
	{
		outNormal = OctEncode(normalViewSpaceFinal);
	}
}
