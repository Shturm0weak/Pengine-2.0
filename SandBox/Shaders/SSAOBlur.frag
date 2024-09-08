#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D ssaoTexture;

void main()
{
    const int blurRange = 2;
    int n = 0;
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoTexture, 0));
    float result = 0.0;
    for (int x = -blurRange; x <= blurRange; x++) 
    {
        for (int y = -blurRange; y <= blurRange; y++) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoTexture, uv + offset).r;
            n++;
        }
    }
    outColor = vec4(result / (float(n)));
}
