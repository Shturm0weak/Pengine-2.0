#version 450

layout(location = 0) in vec3 color;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outShading;
layout(location = 3) out vec4 outEmissive;

void main()
{
	outAlbedo = vec4(color, 1.0f);
	outNormal = vec4(0.0f);
	outShading = vec4(0.0f);
	outEmissive = vec4(0.0f);
}
