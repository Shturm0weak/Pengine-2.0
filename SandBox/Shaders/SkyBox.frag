#version 450

layout(set = 1, binding = 0) uniform samplerCube SkyBox;

layout(location = 0) in vec3 uv;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec2 outNormal;
layout(location = 2) out vec4 outShading;
layout(location = 3) out vec4 outEmissive;

void main()
{
    vec4 color = texture(SkyBox, uv);

    // Apply reverse gamma correction, otherwise the skybox will be too bright,
    // you can play around with it.
    color.xyz = pow(color.xyz, vec3(2.2f / 1.0f));

    outAlbedo = color;
    outNormal = vec2(0.0f);
    outShading = vec4(0.0f);
    outEmissive = vec4(0.0f);
}
