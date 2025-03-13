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
layout(set = 0, binding = 3) uniform sampler2D rawSSRTexture;
layout(set = 0, binding = 4) uniform sampler2D blurSSRTexture;
layout(set = 0, binding = 5) uniform sampler2D uiTexture;

layout(set = 0, binding = 6) uniform PostProcessBuffer
{
	int toneMapperIndex;
	float gamma;
	vec2 viewportSize;
	int fxaa;
	int isSSREnabled;
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
		reflectionColor = mix(texture(rawSSRTexture, uv), texture(blurSSRTexture, uv), roughness);
	}

	deferred = mix(deferred, reflectionColor.xyz, metallic * reflectionColor.a * isSSREnabled * alpha);

	vec4 uiColor = texture(uiTexture, uv);

	vec3 toneMappedColor;
	if (toneMapperIndex == 0)
	{
		toneMappedColor = pow(deferred + bloom, vec3(1.0f / gamma));
	}
	else if (toneMapperIndex == 1)
	{
		toneMappedColor = pow(ACES(deferred + bloom), vec3(1.0f / gamma));
	}

	outColor = vec4(mix(toneMappedColor, uiColor.xyz, uiColor.a), 1.0f);
}
