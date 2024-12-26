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

layout(set = 0, binding = 2) uniform PostProcessBuffer
{
	int toneMapperIndex;
	float gamma;
	vec2 viewportSize;
	int fxaa;
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

	if (toneMapperIndex == 0)
	{
		outColor = vec4(pow(deferred + bloom, vec3(1.0f / gamma)), 1.0f);
	}
	else if (toneMapperIndex == 1)
	{
		outColor = vec4(pow(ACES(deferred + bloom), vec3(1.0f / gamma)), 1.0f);
	}
}
