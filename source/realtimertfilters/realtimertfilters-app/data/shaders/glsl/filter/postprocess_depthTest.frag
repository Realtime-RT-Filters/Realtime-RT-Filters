#version 420
#extension GL_KHR_vulkan_glsl : enable

layout (set = 0, binding = 0) uniform sampler2D Depth;
//layout (set = 0, binding = 1, rgba32f) uniform writeonly image2D Output;

layout (location = 0) out vec4 Filteroutput;

#include "filtercommon.glsl"

void main()
{
	float color = texelFetch(Depth, Texel, 0).x;
	color -= 0.9;
	color *= 10;
//	imageStore(Output, scrCoord, vec4(vec3(color), 1.0));
	Filteroutput = vec4(vec3(color), 1.0);
}