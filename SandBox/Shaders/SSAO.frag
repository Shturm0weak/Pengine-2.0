#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 viewRay;

layout(location = 0) out vec4 outColor;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
    Camera camera;
};

layout(set = 1, binding = 0) uniform SSAOBuffer
{
    vec4 samples[64];
    int kernelSize;
    int noiseSize;
    float radius;
    float bias;
    float aoScale;
    vec2 viewportScale;
};

layout(set = 1, binding = 1) uniform sampler2D depthTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D noiseTexture;

vec2 noiseScale = vec2((camera.viewportSize.x * viewportScale.x) / noiseSize, (camera.viewportSize.y * viewportScale) / noiseSize);

void main()
{
    vec3 position = CalculatePositionFromDepth(
        texture(depthTexture, uv).x,
        camera.projectionMat4,
        viewRay);

    vec3 normal = texture(normalTexture, uv).xyz;
    vec3 randomVector = normalize(texture(noiseTexture, uv * noiseScale).xyz);

    // Create TBN change-of-basis matrix: from tangent space to view space.
    vec3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0f;
    for (int i = 0; i < kernelSize; ++i)
    {
        vec3 currentSample = TBN * samples[i].xyz;
        currentSample = position + currentSample * radius;

        vec4 offset = vec4(currentSample, 1.0f);
        offset = camera.projectionMat4 * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5f + 0.5f;
        
        // Note: Only for Vulkan need to flip Y!
        offset.y = 1.0f - offset.y;

        float sampleDepth = CalculatePositionFromDepth(
            texture(depthTexture, offset.xy).x,
            camera.projectionMat4,
            viewRay).z;

        float rangeCheck = 1.0f - smoothstep(0.0f, 1.0f, abs(position.z - sampleDepth) / radius);
        occlusion += (sampleDepth >= currentSample.z + bias ? 1.0f : 0.0f) * rangeCheck;
    }
    occlusion = 1.0f - (occlusion / float(kernelSize));
    occlusion = pow(occlusion, aoScale);

    outColor = vec4(occlusion, occlusion, occlusion, 1.0f);
}
