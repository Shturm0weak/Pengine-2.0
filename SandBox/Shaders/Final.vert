#version 450

layout(location = 0) in vec2 positionA;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec2 v_rgbNW;
layout(location = 2) out vec2 v_rgbNE;
layout(location = 3) out vec2 v_rgbSW;
layout(location = 4) out vec2 v_rgbSE;
layout(location = 5) out vec2 v_rgbM;

layout(set = 0, binding = 5) uniform PostProcessBuffer
{
	int toneMapperIndex;
	float gamma;
	vec2 viewportSize;
	int fxaa;
};

#include "Shaders/Includes/FXAAUV.h"

void main()
{
	gl_Position = vec4(positionA.x, positionA.y, 0.0f, 1.0f);
	uv = (positionA + vec2(1.0f)) * vec2(0.5f);
	uv.y = 1.0f - uv.y;

	if(fxaa == 1)
	{
		FXAAUV(uv * viewportSize, viewportSize, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, v_rgbM);
	}
}
