#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 viewRay;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outEmissive;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D shadingTexture;
layout(set = 1, binding = 3) uniform sampler2D depthTexture;
layout(set = 1, binding = 4) uniform sampler2D ssaoTexture;
layout(set = 1, binding = 5) uniform sampler2DArray CSMTexture;

#include "Shaders/Includes/IsBrightPixel.h"
#include "Shaders/Includes/PointLight.h"
#include "Shaders/Includes/DirectionalLight.h"
#include "Shaders/Includes/CSM.h"
#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

layout(set = 1, binding = 6) uniform Lights
{
	PointLight pointLights[32];
	int pointLightsCount;

	DirectionalLight directionalLight;
	int hasDirectionalLight;

	float brightnessThreshold;

	CSM csm;
};

void main()
{
	vec3 albedoColor = texture(albedoTexture, uv).xyz;
	vec3 shading = texture(shadingTexture, uv).xyz;

	vec4 normalViewSpace = texture(normalTexture, uv);
	vec3 ssao = texture(ssaoTexture, uv).xyz;

	vec3 result = vec3(0.0f);

	if (normalViewSpace.a <= 0.0f)
	{
		result = albedoColor;
	}
	else
	{
		vec3 positionViewSpace = CalculatePositionFromDepth(
			texture(depthTexture, uv).x,
			camera.projectionMat4,
			viewRay);

		if (hasDirectionalLight == 1)
		{
			vec3 shadow = vec3(0.0f);
			if (csm.isEnabled == 1)
			{
				vec3 worldSpacePosition = (camera.inverseViewMat4 * vec4(positionViewSpace, 1.0f)).xyz;

				shadow = CalculateCSM(
					CSMTexture,
					csm,
					abs(positionViewSpace.z),
					worldSpacePosition,
					normalViewSpace.xyz,
					directionalLight.direction);
			}

			vec3 basicReflectivity = mix(vec3(0.05), albedoColor, shading.x);
			vec3 viewDirection = normalize(-positionViewSpace);
			result += CalculateDirectionalLight(
				directionalLight,
				viewDirection,
				basicReflectivity,
				normalViewSpace.xyz,
				albedoColor,
				shading.x,
				shading.y,
				shading.z,
				shadow);
		}

		for (int i = 0; i < pointLightsCount; i++)
		{
			result += CalculatePointLight(pointLights[i], positionViewSpace, normalViewSpace.xyz) * albedoColor;
		}
	}

	outEmissive = vec4(IsBrightPixel(result, brightnessThreshold), 1.0f);
	outColor = vec4(result * ssao, 1.0f);
}
