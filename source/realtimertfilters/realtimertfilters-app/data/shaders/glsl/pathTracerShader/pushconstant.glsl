layout(push_constant) uniform Constants
{
	vec4	clearColor;
	float	lightIntensity;
	int		lightType;
	int		frame;
	int		samples;
	int		bounces;
	int		bounceSamples;
	float	temporalAlpha;
}
pushC;