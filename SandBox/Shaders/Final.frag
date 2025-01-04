#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 v_rgbNW;
layout(location = 2) in vec2 v_rgbNE;
layout(location = 3) in vec2 v_rgbSW;
layout(location = 4) in vec2 v_rgbSE;
layout(location = 5) in vec2 v_rgbM;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D deferredTexture;
layout(set = 0, binding = 1) uniform sampler2D bloomTexture;
layout(set = 0, binding = 2) uniform sampler2D shadingTexture;
layout(set = 0, binding = 3) uniform sampler2D SSRTexture;
layout(set = 0, binding = 4) uniform sampler2D rawSSRTexture;

layout(set = 0, binding = 5) uniform PostProcessBuffer
{
	int toneMapperIndex;
	float gamma;
	vec2 viewportSize;
	int fxaa;
	int isSSREnabled;
	int isSSRMipMapsEnabled;
};

#include "Shaders/Includes/ACES.h"
#include "Shaders/Includes/FXAA.h"

void main()
{
	vec3 bloom = texture(bloomTexture, uv).xyz;
	
	vec3 deferred;
	if (fxaa == 1)
	{
		deferred = FXAA(
			deferredTexture,
			uv * viewportSize,
			viewportSize,
			v_rgbNW,
			v_rgbNE,
			v_rgbSW,
			v_rgbSE,
			v_rgbM);
	}
	else
	{
		deferred = texture(deferredTexture, uv).xyz;
	}

	vec4 shading = texture(shadingTexture, uv);
	float metallic = shading.r;
	float roughness = shading.g;
	float alpha = shading.a;

	vec4 reflectionColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);

	if (isSSREnabled == 1)
	{
		if (isSSRMipMapsEnabled == 1)
		{
			int reflectionLod0 = int(floor(roughness * 7.0f));
			int reflectionLod1 = min(reflectionLod0 + 1, 7);
			reflectionColor = mix(textureLod(SSRTexture, uv, reflectionLod0), textureLod(SSRTexture, uv, reflectionLod1), roughness * 7.0f - reflectionLod0);
		}
		else
		{
			reflectionColor = mix(texture(rawSSRTexture, uv), textureLod(SSRTexture, uv, 0), roughness);
		}
	}

	deferred = mix(deferred, reflectionColor.xyz, metallic * reflectionColor.a * isSSREnabled * alpha);
	
	if (toneMapperIndex == 0)
	{
		outColor = vec4(pow(deferred + bloom, vec3(1.0f / gamma)), 1.0f);
	}
	else if (toneMapperIndex == 1)
	{
		outColor = vec4(pow(ACES(deferred + bloom), vec3(1.0f / gamma)), 1.0f);
	}
}
