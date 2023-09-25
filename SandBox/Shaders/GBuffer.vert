#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec3 normalA;
layout(location = 2) in vec2 uvA;
layout(location = 3) in mat4 transformA;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec3 worldPosition;
layout(location = 2) out vec2 uv;

layout(std140, set = 0, binding = 0) uniform GlobalBuffer
{
	mat4 viewProjectionMat4;
};

void main()
{
	worldPosition = (transformA * vec4(positionA, 1.0)).xyz;
	gl_Position = viewProjectionMat4 * vec4(worldPosition, 1.0);
	normal = (transformA * vec4(normalA, 1.0)).xyz;
	uv = uvA;
}