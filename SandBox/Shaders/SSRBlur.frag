#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

layout(set = 0, binding = 1) uniform SSRBlurBuffer
{
    int blurRange;
    int blurOffset;
};

void main()
{
    int n = 0;
    vec2 texelSize = 1.0f / vec2(textureSize(sourceTexture, 0));
    vec4 result = vec4(0.0f);
    for (int x = -blurRange; x <= blurRange; x++)
    {
        for (int y = -blurRange; y <= blurRange; y++)
        {
            vec2 offset = vec2(float(x) * blurOffset, float(y) * blurOffset) * texelSize;
            result += texture(sourceTexture, uv + offset);
            n++;
        }
    }
    outColor = result / float(n);
}
