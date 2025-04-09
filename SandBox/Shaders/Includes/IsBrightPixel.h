vec3 IsBrightPixel(in vec3 color, in float threshold)
{
	const vec3 colorSRGB = pow(color, vec3(1.0f / 2.2f));
    return dot(colorSRGB, vec3(0.2126f, 0.7152f, 0.0722f)) > threshold ? colorSRGB : vec3(0.0f);
}
