#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) flat out int faceId;

void main()
{
    for (int face = 0; face < 6; face++)
    {
        faceId = face;
        gl_Layer = face;
        for (int i = 0; i < 3; ++i)
        {
            gl_Position = gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
