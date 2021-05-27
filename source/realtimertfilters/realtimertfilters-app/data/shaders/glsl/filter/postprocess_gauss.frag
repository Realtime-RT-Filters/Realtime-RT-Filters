#version 420

layout (set = 0, binding = 0, rgba32f) uniform readonly image2D Source;
layout (set = 0, binding = 1, rgba32f) uniform writeonly image2D Destination;

layout( push_constant ) uniform constants
{
	uint SCR_WIDTH;
	uint SCR_HEIGHT;
} PushC;

float KERNEL3X3[9] =
{
	1.f, 2.f, 1.f,
	2.f, 4.f, 2.f,
	1.f, 2.f, 1.f,
};

layout (location = 0) in vec2 UV;

void main()
{
	ivec2 scrCoord = ivec2(UV.x * PushC.SCR_WIDTH, UV.y * PushC.SCR_HEIGHT);

	vec4 color;

	for (int x = 0; x < 3; x++)
	{
		for (int y = 0; y < 3; y++)
		{
			ivec2 loadCoord = ivec2(x - 1 + scrCoord.x, y - 1 + scrCoord.y);
			color += imageLoad(Source, loadCoord) * KERNEL3X3[x * 3 + y];
		}
	}

	color /= 16.f;

	imageStore(Destination, scrCoord, color);
}