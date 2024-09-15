#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 30) out;

layout(set = 0, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[3];
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
