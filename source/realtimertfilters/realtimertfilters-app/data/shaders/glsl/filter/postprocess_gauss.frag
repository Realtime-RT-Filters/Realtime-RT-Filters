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
	ivec2 scrCoord = ivec2(UV.x * (PushC.SCR_WIDTH - 1), UV.y * (PushC.SCR_HEIGHT - 1));

	vec3 color = vec3(0.0,0.0,0.0);

	for (int x = 0; x < 3; x++)
	{
		for (int y = 0; y < 3; y++)
		{
			ivec2 loadCoord = ivec2(x - 1 + scrCoord.x, y - 1 + scrCoord.y);
			color += imageLoad(Source, loadCoord).xyz * KERNEL3X3[x * 3 + y];
		}
	}

	color /= 16.f;

	imageStore(Destination, scrCoord, vec4(color, 1.0));
	//imageStore(Destination, scrCoord, vec4(UV.x, 0.0, UV.y, 1.0));
	//imageStore(Destination, scrCoord, vec4(vec3(1.0) - imageLoad(Source, scrCoord).xzy, 1.0));
}