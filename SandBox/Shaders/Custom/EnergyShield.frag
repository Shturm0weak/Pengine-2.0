#version 450

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 worldPosition;
layout(location = 2) in vec2 uv;
layout(location = 3) flat in vec3 cameraPosition;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform EnergyShieldBuffer
{
	vec4 color;
};

void main()
{
	
	outColor = color;
}
