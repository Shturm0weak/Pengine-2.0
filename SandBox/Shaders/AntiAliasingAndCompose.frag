#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 v_rgbNW;
layout(location = 2) in vec2 v_rgbNE;
layout(location = 3) in vec2 v_rgbSW;
layout(location = 4) in vec2 v_rgbSE;
layout(location = 5) in vec2 v_rgbM;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D toneMappedColorTexture;
layout(set = 0, binding = 1) uniform sampler2D uiTexture;

layout(set = 0, binding = 2) uniform PostProcessBuffer
{
	vec2 viewportSize;
	int fxaa;
};

#include "Shaders/Includes/FXAA.h"

void main()
{
	vec3 toneMappedColor;
	if (fxaa == 1)
	{
		toneMappedColor = FXAA(
			toneMappedColorTexture,
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
		toneMappedColor = texture(toneMappedColorTexture, uv).xyz;
	}

	vec4 uiColor = texture(uiTexture, uv);
	outColor = vec4(mix(toneMappedColor, uiColor.xyz, uiColor.a), 1.0f);
}
