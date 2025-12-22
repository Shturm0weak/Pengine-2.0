#version 450

layout(location = 0) in vec3 positionViewSpace;
layout(location = 1) in vec3 positionWorldSpace;
layout(location = 2) in vec3 normalViewSpace;
layout(location = 3) in vec3 tangentViewSpace;
layout(location = 4) in vec3 bitangentViewSpace;
layout(location = 5) in vec2 uv;
layout(location = 6) in vec4 color;
layout(location = 7) in vec3 positionTangentSpace;
layout(location = 8) in vec3 cameraPositionTangentSpace;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outNormal;
layout(location = 2) out vec4 outShading;
layout(location = 3) out vec4 outEmissive;

#include "Shaders/Includes/Camera.h"
layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

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

#include "Shaders/Includes/IsBrightPixel.h"
#include "Shaders/Includes/PointLight.h"
#include "Shaders/Includes/DirectionalLight.h"
#include "Shaders/Includes/CSM.h"

layout(set = 2, binding = 0) uniform sampler2D deferredAlbedoTexture;
layout(set = 2, binding = 1) uniform sampler2D deferredNormalTexture;
layout(set = 2, binding = 2) uniform sampler2D deferredShadingTexture;
layout(set = 2, binding = 3) uniform sampler2D deferredDepthTexture;
layout(set = 2, binding = 4) uniform sampler2D deferredSsaoTexture;
layout(set = 2, binding = 5) uniform sampler2D deferredSssTexture;
layout(set = 2, binding = 6) uniform sampler2DArray deferredCSMTexture;

layout(set = 3, binding = 0) uniform Lights
{
	PointLight pointLights[32];
	int pointLightsCount;

	DirectionalLight directionalLight;
	int hasDirectionalLight;

	float brightnessThreshold;

	CSM csm;
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

	vec3 metallicRoughness = texture(metallicRoughnessTexture, finalUV).xyz;
	float metallic = metallicRoughness.b;
	float roughness = metallicRoughness.g;
	float ao = texture(aoTexture, finalUV).r;

	vec4 shading = vec4(
		metallic * material.metallicFactor,
		roughness * material.roughnessFactor,
		ao * material.aoFactor,
		albedoColor.a);
		
	vec3 normal = gl_FrontFacing ? normalViewSpace : -normalViewSpace;
	normal = normalize(normal);
	if (material.useNormalMap > 0)
	{
		mat3 TBN = mat3(normalize(tangentViewSpace), normalize(bitangentViewSpace), normal);
		normal = texture(normalTexture, finalUV).xyz;
		normal = normal * 2.0f - 1.0f;
		normal = normalize(TBN * normal);
	}

	vec3 basicReflectivity = mix(vec3(0.05f), albedoColor.xyz, shading.x);
	vec3 viewDirection = normalize(-positionViewSpace);
	vec3 result = vec3(0.0f);

	if (hasDirectionalLight == 1)
	{
	    vec3 shadow = vec3(0.0f);
		if (csm.isEnabled == 1)
		{
			shadow = CalculateCSM(
				deferredCSMTexture,
				csm,
				abs(positionViewSpace.z),
				positionWorldSpace,
				normal.xyz,
				directionalLight.direction);
		}

		result += CalculateDirectionalLight(
			directionalLight,
			viewDirection,
			basicReflectivity,
			normal.xyz,
			albedoColor.xyz,
			shading.x,
			shading.y,
			shading.z,
			shadow,
			vec3(1.0f));
	}

	for (int i = 0; i < pointLightsCount; i++)
	{
		result += CalculatePointLight(pointLights[i], positionViewSpace, normal.xyz) * albedoColor.xyz;
	}

	vec3 emissiveColor = texture(emissiveTexture, finalUV).xyz;
    outEmissive = max(vec4(emissiveColor * material.emissiveColor.xyz * material.emissiveFactor, albedoColor.a), vec4(IsBrightPixel(result, brightnessThreshold), albedoColor.a));
	outColor = vec4(result, albedoColor.a);
	outNormal = OctEncode(normal);
	outShading = shading;
}
