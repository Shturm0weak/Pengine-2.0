#version 450

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 viewRay;

layout(location = 0) out vec4 outColor;

#include "Shaders/Includes/Camera.h"

layout(set = 0, binding = 0) uniform GlobalBuffer
{
    Camera camera;
};

layout(set = 1, binding = 0) uniform sampler2D normalTexture;
layout(set = 1, binding = 1) uniform sampler2D shadingTexture;
layout(set = 1, binding = 3) uniform sampler2D colorTexture;
layout(set = 1, binding = 4) uniform sampler2D depthTexture;

layout(set = 1, binding = 5) uniform SSRBuffer
{
    vec2 viewportScale;
    float maxDistance;
    float resolution;
    int stepCount;
    float thickness; 
};

void main()
{
    vec4 normal = texture(normalTexture, uv);
    float metallic = texture(shadingTexture, uv).x;
    if (normal.a <= 0.0f || metallic <= 0.0f)
    {
        outColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    vec2 viewportSize = camera.viewportSize * viewportScale;

    vec3 positionFrom = CalculatePositionFromDepth(
        texture(depthTexture, uv).x,
        camera.projectionMat4,
        viewRay);

    vec3 unitPosition = normalize(positionFrom);
    vec3 pivot = normalize(reflect(unitPosition, normalize(normal.xyz)));
    if(pivot.z > 0.0f)
    {
        outColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    vec4 start = vec4(positionFrom, 1.0f);
    vec4 end = vec4(positionFrom + (pivot * maxDistance), 1.0f);

    vec4 startFragment = start;
    // Project to screen space.
    startFragment = camera.projectionMat4 * startFragment;
    // Perform the perspective divide.
    startFragment.xyz /= startFragment.w;
    // Convert the screen-space XY coordinates to UV coordinates.
    startFragment.xy = startFragment.xy * 0.5f + 0.5f;
    // Note: Only for Vulkan need to flip Y!
    startFragment.y = 1.0f - startFragment.y;
    // Convert the UV coordinates to fragment/pixel coordnates.
    startFragment.xy *= viewportSize;

    vec4 endFragment = end;
    endFragment /= (endFragment.z < 0.0f ? endFragment.z : 1.0f);
    endFragment = camera.projectionMat4 * endFragment;
    endFragment.xyz /= endFragment.w;
    endFragment.xy = endFragment.xy * 0.5f + 0.5f;
    endFragment.y = 1.0f - endFragment.y;
    endFragment.xy *= viewportSize;

    vec2 fragment = startFragment.xy;
    vec2 fragmentUV = fragment / viewportSize;

    float deltaX = endFragment.x - startFragment.x;
    float deltaY = endFragment.y - startFragment.y;

    float useX = abs(deltaX) >= abs(deltaY) ? 1.0f : 0.0f;
    float delta = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0f, 1.0f);
    vec2 increment = vec2(deltaX, deltaY) / max(delta, 0.001f);

    float search0 = 0.0f;
    float search1 = 0.0f;

    int hit0 = 0;
    int hit1 = 0;

    float viewDistance = start.z;
    float depth = thickness;

    vec3 positionTo = positionFrom;

    int i = 0;
    for (i = 0; i < int(delta); ++i)
    {
        fragment += increment;
        fragmentUV = fragment / viewportSize;
        positionTo = CalculatePositionFromDepth(
            texture(depthTexture, fragmentUV).x,
            camera.projectionMat4,
            viewRay);

        search1 = mix((fragment.y - startFragment.y) / deltaY, (fragment.x - startFragment.x) / deltaX, useX);

        viewDistance = (start.z * end.z) / mix(end.z, start.z, search1);
        depth = positionTo.z - viewDistance;

        if (depth > 0 && depth < thickness)
        {
            hit0 = 1;
            break;
        }
        else
        {
            search0 = search1;
        }
    }

    search1 = search0 + ((search1 - search0) / 2.0f);

    for (i = 0; i < stepCount * hit0; ++i)
    {
        fragment = mix(startFragment.xy, endFragment.xy, search1);
        fragmentUV.xy = fragment / viewportSize;
        positionTo = CalculatePositionFromDepth(
            texture(depthTexture, fragmentUV).x,
            camera.projectionMat4,
            viewRay);

        viewDistance = (start.z * end.z) / mix(end.z, start.z, search1);
        depth = positionTo.z - viewDistance;

        if (depth > 0 && depth < thickness)
        {
            hit1 = 1;
            search1 = search0 + ((search1 - search0) / 2.0f);
        }
        else
        {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2.0f);
            search0 = temp;
        }
    }

    float visibility = hit1 * (1.0f - max(dot(-unitPosition, pivot), 0.0f))
        * (1.0f - clamp(depth / thickness, 0.0f, 1.0f))
        * (1.0f - clamp(length(positionFrom - positionTo) / maxDistance, 0.0f, 1.0f))
        * (fragmentUV.x < 0.0f || fragmentUV.x > 1.0f ? 0.0f : 1.0f)
        * (fragmentUV.y < 0.0f || fragmentUV.y > 1.0f ? 0.0f : 1.0f);

    visibility = clamp(visibility, 0.0f, 1.0f);

    vec3 reflectionColor = mix(vec3(0.0f, 0.0f, 0.0f), texture(colorTexture, fragmentUV).xyz, visibility);

    outColor = vec4(reflectionColor, visibility);
    //outColor = vec4(visibility, visibility, visibility, 1.0f);
}
