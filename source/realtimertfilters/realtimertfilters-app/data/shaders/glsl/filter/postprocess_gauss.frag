#version 420

layout (set = 0, binding = 0, rgba32f) uniform readonly image2D Source;
layout (set = 0, binding = 1, rgba32f) uniform writeonly image2D Output;

float KERNEL5X5[25] =
{
	1, 4, 7, 4, 1,
	4, 16, 26, 16, 4,
	7, 26, 41, 26, 7,
	4, 16, 26, 16, 4,
	1, 4, 7, 4, 1
};

#include "filtercommon.glsl"

void main()
{
	vec3 color = vec3(0.0,0.0,0.0);

	for (int x = 0; x < 5; x++)
	{
		for (int y = 0; y < 5; y++)
		{
			ivec2 loadCoord = ivec2(x - 2 + Texel.x, y - 2 + Texel.y);
			color += imageLoad(Source, loadCoord).xyz * KERNEL5X5[x * 5 + y];
		}
	}

	color /= 273.f;

	imageStore(Output, Texel, vec4(color, 1.0));
}