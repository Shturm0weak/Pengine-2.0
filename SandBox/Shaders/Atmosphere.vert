#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in vec2 uvA;
layout(location = 2) in vec3 normalA;
layout(location = 3) in vec3 tangentA;
layout(location = 4) in vec3 bitangentA;

void main()
{
	gl_Position = vec4(positionA.x, positionA.z, positionA.y, 1.0f);
}
