#version 450

layout(location = 0) in vec2 positionA;

void main()
{
	gl_Position = vec4(positionA.x, positionA.y, 0.0f, 1.0f);
}
