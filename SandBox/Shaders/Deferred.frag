#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D positionTexture;
layout(set = 1, binding = 3) uniform sampler2D shadingTexture;

#include "Shaders/Includes/PointLight.h"
#include "Shaders/Includes/DirectionalLight.h"
#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

layout(set = 1, binding = 4) uniform Lights
{
	PointLight pointLights[32];
	int pointLightsCount;

	DirectionalLight directionalLight;
	int hasDirectionalLight;
};

void main()
{
	vec3 albedoColor = texture(albedoTexture, uv).xyz;
	vec4 normal = texture(normalTexture, uv);
	vec3 position = texture(positionTexture, uv).xyz;
	vec3 shading = texture(shadingTexture, uv).xyz;

	vec3 basicReflectivity = mix(vec3(0.05), albedoColor, shading.x);
	vec3 viewDirection = normalize(camera.position - position);
	vec3 result = vec3(0.0f);

	if (normal.a == 0)
	{
		result = albedoColor;
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
				albedoColor,
				shading.x,
				shading.y,
				shading.z);
		}

		for (int i = 0; i < pointLightsCount; i++)
		{
			result += CalculatePointLight(pointLights[i], position, normal.xyz) * albedoColor;
		}
	}

	outColor = vec4(result, 1.0f);
}
