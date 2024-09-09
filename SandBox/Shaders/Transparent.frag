#version 450

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec3 worldPosition;
layout(location = 2) in vec2 uv;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec4 outColor;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

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

#include "Shaders/Includes/PointLight.h"
#include "Shaders/Includes/DirectionalLight.h"

// Deferred uniform samplers. Only need lights from this descriptor set.
layout(set = 2, binding = 0) uniform sampler2D deferredAlbedoTexture;
layout(set = 2, binding = 1) uniform sampler2D deferredNormalTexture;
layout(set = 2, binding = 2) uniform sampler2D deferredShadingTexture;
layout(set = 2, binding = 3) uniform sampler2D deferredDepthTexture;
layout(set = 2, binding = 4) uniform sampler2D deferredSsaoTexture;

layout(set = 2, binding = 5) uniform Lights
{
	PointLight pointLights[32];
	int pointLightsCount;

	DirectionalLight directionalLight;
	int hasDirectionalLight;
};

void main()
{
	vec4 albedoColor = texture(albedoTexture, uv) * material.color;
	float metallic = texture(metalnessTexture, uv).r;
	float roughness = texture(roughnessTexture, uv).r;
	float ao = texture(aoTexture, uv).r;

	vec4 shading = vec4(
		metallic * material.metallicFactor,
		roughness * material.roughnessFactor,
		ao * material.aoFactor,
		1.0f);
		
	vec4 normal;
	if (material.useNormalMap > 0)
	{
		vec3 normalMap = texture(normalTexture, uv).xyz;
		normalMap *= normalMap * 2.0f - 1.0f;
		normal = vec4(normalize(TBN * normalMap), 1.0f);
	}
	else
	{
		normal = vec4(worldNormal, 1.0f);
	}

	vec3 basicReflectivity = mix(vec3(0.05), albedoColor.xyz, shading.x);
	vec3 viewDirection = normalize(camera.position - worldPosition);
	vec3 result = vec3(0.0f);

	if (normal.a == 0)
	{
		result = albedoColor.xyz;
	}
	else
	{
		if (hasDirectionalLight == 1)
		{
			result += CalculateDirectionalLight(
				directionalLight,
				viewDirection,
				basicReflectivity,
				normal.xyz,
				albedoColor.xyz,
				shading.x,
				shading.y,
				shading.z);
		}

		for (int i = 0; i < pointLightsCount; i++)
		{
			result += CalculatePointLight(pointLights[i], worldPosition, normal.xyz) * albedoColor.xyz;
		}
	}

	outColor = vec4(result, albedoColor.w);
}
