vec3 ScreenSpaceShadows(
    sampler2D depthTexture,
    in vec3 positionViewSpace,
    in vec3 lightDirection,
    in vec2 viewRay,
    in mat4 projectionMat4,
    in uint maxSteps,
    in float maxRayDistance,
    in float maxDistance,
    in float minThickness,
    in float maxThickness)
{
    if (-positionViewSpace.z > maxDistance)
    {
        return vec3(0.0f);
    }

    vec3 rayPosition = positionViewSpace;
    vec3 rayDirection = lightDirection;

    float stepLength = maxRayDistance / maxSteps;
    vec3 rayStep = rayDirection * stepLength;

    float occlusion = 0.0f;
    vec2 rayUV = vec2(0.0f);
    for (uint i = 0; i < maxSteps; i++)
    {
        rayPosition += rayStep;

		vec4 clipPos = projectionMat4 * vec4(rayPosition, 1.0f);
        if (clipPos.w <= 0.0) break;
		vec3 ndc = clipPos.xyz / clipPos.w;
        rayUV = ndc.xy * 0.5 + 0.5;
		
        if ((rayUV.x >= 0.0f && rayUV.x <= 1.0f) && (rayUV.y >= 0.0f && rayUV.y <= 1.0f))
        {
			rayUV.y = 1.0f - rayUV.y;
            float depthZ = CalculatePositionFromDepth(
				texture(depthTexture, rayUV).x,
				projectionMat4,
				viewRay).z;
            float depthDelta = rayPosition.z - depthZ;

            if (depthDelta > minThickness && depthDelta < maxThickness)
            {
                occlusion = 1.0f;
                break;
            }
        }
    }

    return vec3(occlusion);
}
