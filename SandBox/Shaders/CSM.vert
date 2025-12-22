#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in mat4 transformA;
layout(location = 6) in uint layersA;

layout(location = 0) out vec2 uv;
layout(location = 1) flat out uint layers;

void main()
{
	gl_Position = transformA * vec4(positionA, 1.0f);
	layers = layersA;
	uv = uvA;
}
