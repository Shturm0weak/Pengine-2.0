vec2 ParallaxOcclusionMapping(
    sampler2D heightTexture,
    in vec2 uv,
    in vec3 viewDirection,
    in int minParallaxLayers,
    in int maxParallaxLayers,
    in float parallaxHeightScale)
{
    float numLayers = mix(maxParallaxLayers, minParallaxLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDirection)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    
    vec2 P = viewDirection.xy * -parallaxHeightScale;
    vec2 deltaUV = P / numLayers;
    
    vec2 currentUV = uv;
    float currentDepthMapValue = texture(heightTexture, currentUV).r;
    
    while (currentLayerDepth < currentDepthMapValue)
	{
        currentUV -= deltaUV;
        currentDepthMapValue = texture(heightTexture, currentUV).r;
        currentLayerDepth += layerDepth;
    }
    
    vec2 prevUV = currentUV + deltaUV;
    
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(heightTexture, prevUV).r - currentLayerDepth + layerDepth;
    
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalUV = mix(currentUV, prevUV, weight);
    
    return finalUV;
}
