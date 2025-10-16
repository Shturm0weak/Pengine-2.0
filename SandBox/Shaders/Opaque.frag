#version 450

layout(location = 0) in vec3 normalViewSpace;
layout(location = 1) in vec3 tangentViewSpace;
layout(location = 2) in vec3 bitangentViewSpace;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec4 color;
layout(location = 5) in vec3 positionTangentSpace;
layout(location = 6) in vec3 cameraPositionTangentSpace;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outShading;
layout(location = 3) out vec4 outEmissive;

layout(set = 1, binding = 1) uniform sampler2D albedoTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D aoTexture;
layout(set = 1, binding = 5) uniform sampler2D emissiveTexture;
layout(set = 1, binding = 6) uniform sampler2D heightTexture;

#include "Shaders/Includes/DefaultMaterial.h"
layout(set = 1, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
};

#include "Shaders/Includes/Camera.h"
layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

#include "Shaders/Includes/ParallaxOcclusionMapping.h"

void main()
{
	vec2 finalUV = uv;
	if (material.useParallaxOcclusion > 0)
	{
		vec3 viewDirection = normalize(cameraPositionTangentSpace - positionTangentSpace);
		finalUV = ParallaxOcclusionMapping(
			heightTexture,
			finalUV,
			viewDirection,
			material.minParallaxLayers,
			material.maxParallaxLayers,
			material.parallaxHeightScale);
	}

	vec4 albedoColor = texture(albedoTexture, finalUV) * material.albedoColor * color;
	if (material.useAlphaCutoff > 0)
	{
		if (albedoColor.a < material.alphaCutoff)
		{
			discard;
		}
	}

	vec3 metallicRoughness = texture(metallicRoughnessTexture, finalUV).xyz;
	float metallic = metallicRoughness.b;
	float roughness = metallicRoughness.g;
	float ao = texture(aoTexture, finalUV).r;

	outAlbedo = albedoColor;
	outShading = vec4(
		metallic * material.metallicFactor,
		roughness * material.roughnessFactor,
		ao * material.aoFactor,
		1.0f);
	outEmissive = texture(emissiveTexture, finalUV) * material.emissiveColor * material.emissiveFactor;

	vec3 normal = gl_FrontFacing ? normalViewSpace : -normalViewSpace;
	normal = normalize(normal);
	if (material.useNormalMap > 0)
	{
		mat3 TBN = mat3(normalize(tangentViewSpace), normalize(bitangentViewSpace), normal);
		normal = texture(normalTexture, finalUV).xyz;
		normal = normal * 2.0f - 1.0f;
		outNormal = vec4(normalize(TBN * normal), 1.0f);
	}
	else
	{
		outNormal = vec4(normal, 1.0f);
	}
}
