#define MAX_CASCADE_COUNT 10

struct CSM
{
	int cascadeCount;
    float fogFactor;
    int pcfEnabled;
    int pcfRange;
	mat4 lightSpaceMatrices[MAX_CASCADE_COUNT];
	float distances[MAX_CASCADE_COUNT];
    float biases[MAX_CASCADE_COUNT];
};

float CalculateCSM(
	sampler2DArray CSMTexture,
	in CSM csm,
	in float depth,
	in vec3 positionWorldSpace,
	in vec3 normalViewSpace,
	in vec3 lightDirectionViewSpace,
	in mat4 viewMat4,
	in float zFar)
{
    if (csm.cascadeCount == 0)
    {
        return 0.0f;
    }

    int layer = -1;
    for(int i = 0; i < csm.cascadeCount; ++i)
    {
        if(depth < csm.distances[i])
        {
            layer = i;
            break;
        }
    }

    if (layer == -1)
    {
        layer = csm.cascadeCount;
    }

    vec4 uv = csm.lightSpaceMatrices[layer] * vec4(positionWorldSpace, 1.0f);
    uv.xyz /= uv.w;
    uv.xy = uv.xy * 0.5f + 0.5f;

    // Note: Only for Vulkan need to flip Y!
    uv.y = 1.0f - uv.y;

    float currentDepth = uv.z;

    if (currentDepth < -1.0f || currentDepth > 1.0f)
    {
        return 0.0f;
    }

    float bias = max(csm.biases[layer] * (1.0f - dot(normalViewSpace, lightDirectionViewSpace)), 0.001f);
    if (layer == csm.cascadeCount)
    {
        bias *= 1 / (zFar * 0.5f);
    }
    else
    {
        bias *= 1 / (csm.distances[layer] * 0.5f);
    }

    float shadow = 0.0f;
    if(csm.pcfEnabled == 1)
    {
        vec2 texelSize = 1.0f / vec2(textureSize(CSMTexture, 0));
        for(int x = -csm.pcfRange; x <= csm.pcfRange; ++x)
        {
            for(int y = -csm.pcfRange; y <= csm.pcfRange; ++y)
            {
                float pcfDepth = texture(CSMTexture, vec3(uv.xy + vec2(x, y) * texelSize, layer)).r;
                shadow += (currentDepth - bias) > pcfDepth ? 1.0f : 0.0f;
            }
        }
        shadow /= (csm.pcfRange * 2 + 1) * (csm.pcfRange * 2 + 1);
    }
    else
    {
        float shadowDepth = texture(CSMTexture, vec3(uv.xy, layer)).r;
        shadow += (currentDepth - bias) > shadowDepth ? 1.0f : 0.0f;
    }

    float fadeDistance = zFar * (1.0f - csm.fogFactor);
    if (depth > fadeDistance)
    {
        float fogFactor = clamp((depth - fadeDistance) / (zFar * csm.fogFactor), 0.0f, 1.0f);
        shadow *= (1.0f - fogFactor);
    }

    return shadow;
}
