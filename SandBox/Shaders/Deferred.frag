#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D albedoTexture;
layout(set = 0, binding = 1) uniform sampler2D normalTexture; 
layout(set = 0, binding = 2) uniform sampler2D positionTexture; 

#include "Shaders/Includes/Light.h"

layout(set = 0, binding = 3) uniform Lights
{
	PointLight pointLights[32];
	int pointLightsCount;
};

vec3 CalculatePointLight(PointLight light, vec3 position, vec3 normal)
{
	vec3 direction = normalize(light.position - position);
	float diff = max(dot(normal, direction), 0.0f);
	vec3 diffuse = light.color * diff;

	float distance    = length(light.position - position);
	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	diffuse   *= attenuation;

	return diffuse;
}

void main()
{
	vec3 albedoColor = texture(albedoTexture, uv).xyz;
	vec3 normal = texture(normalTexture, uv).xyz;
	vec3 position = texture(positionTexture, uv).xyz;

	vec3 result = vec3(0.0f);

	for (int i = 0; i < pointLightsCount; i++)
	{
		result += CalculatePointLight(pointLights[i], position, normal) * albedoColor;
	}

	outColor = vec4(result, 1.0f);
}
