#version 450

layout(location = 0) in vec3 positionA;
layout(location = 1) in mat4 transformA;
layout(location = 5) in mat4 inverseTransformA;

layout(location = 0) flat out mat4 inverseTransform;

#include "Shaders/Includes/Camera.h"
layout(set = 0, binding = 0) uniform GlobalBuffer
{
	Camera camera;
};

#include "Shaders/Includes/DefaultMaterial.h"
layout(set = 1, binding = 0) uniform GBufferMaterial
{
	DefaultMaterial material;
};

void main()
{
    inverseTransform = inverseTransformA;
    gl_Position = camera.viewProjectionMat4 * transformA * vec4(positionA, 1.0f);
}
