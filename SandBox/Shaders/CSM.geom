#version 450 core

#include "Shaders/Includes/CSM.h"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3 * MAX_CASCADE_COUNT) out;

layout(set = 0, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[MAX_CASCADE_COUNT];
    int cascadeCount;
};

void main()
{
    for (int i = 0; i < cascadeCount; ++i)
    {
	    for (int j = 0; j < 3; ++j)
	    {
	        gl_Position = lightSpaceMatrices[i] * gl_in[j].gl_Position;
	        gl_Layer = i;
	        EmitVertex();
	    }
        EndPrimitive();
    }
}
