#version 450

layout(location = 0) in vec4 positionA;
layout(location = 1) in vec2 positionInPixelsA;
layout(location = 2) in vec2 uvA;
layout(location = 3) in float cornerRadiusA;
layout(location = 4) in int instanceIndexA;

layout(location = 0) out vec2 positionInPixels;
layout(location = 1) out vec2 uv;
layout(location = 2) flat out float cornerRadius;
layout(location = 3) flat out int instanceIndex;

void main()
{
	gl_Position = positionA;
	positionInPixels = positionInPixelsA;
	uv = uvA;
	cornerRadius = cornerRadiusA;
	instanceIndex = instanceIndexA;
}
