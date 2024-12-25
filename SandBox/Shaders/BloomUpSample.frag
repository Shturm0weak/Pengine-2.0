#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

void main()
{
    float x = 0.0f;
    float y = 0.0f;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(sourceTexture, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 b = texture(sourceTexture, vec2(uv.x,     uv.y + y)).rgb;
    vec3 c = texture(sourceTexture, vec2(uv.x + x, uv.y + y)).rgb;

    vec3 d = texture(sourceTexture, vec2(uv.x - x, uv.y)).rgb;
    vec3 e = texture(sourceTexture, vec2(uv.x,     uv.y)).rgb;
    vec3 f = texture(sourceTexture, vec2(uv.x + x, uv.y)).rgb;

    vec3 g = texture(sourceTexture, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 h = texture(sourceTexture, vec2(uv.x,     uv.y - y)).rgb;
    vec3 i = texture(sourceTexture, vec2(uv.x + x, uv.y - y)).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    outColor.xyz = e * 4.0f;
    outColor.xyz += (b + d + f + h) * 2.0f;
    outColor.xyz += (a + c + g + i);
    outColor.xyz *= 1.0f / 16.0f;
}
