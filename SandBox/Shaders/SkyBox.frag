#version 450

layout(set = 1, binding = 0) uniform samplerCube SkyBox;

layout(location = 0) in vec3 uv;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outShading;

void main()
{
    vec4 color = texture(SkyBox, uv);

    // Apply reverse gamma correction, otherwise the skybox will be too bright,
    // you can play around with it.
    color.rgb = pow(color.rgb, vec3(2.2 / 1.0));

    outAlbedo = color;
    outNormal = vec4(0.0f);
}
