#version 450

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 color;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPosition;
layout(location = 3) out vec4 outShading;

void main()
{
	outAlbedo = vec4(color, 1.0f);
	outNormal = vec4(0.0f);
	outPosition = vec4(worldPosition, 1.0f);
	outShading = vec4(0.0f);
}
