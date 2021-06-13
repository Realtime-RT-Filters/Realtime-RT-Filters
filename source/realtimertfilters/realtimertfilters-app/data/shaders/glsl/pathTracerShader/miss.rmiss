#version 460
#extension GL_EXT_ray_tracing : require
#include "raycommon.glsl"
#include "binding.glsl"

layout(location = LOCATION_PBR) rayPayloadInEXT S_HitPayload prd;

void main()
{
	prd.radiance = vec3(0);		// Not light -> Black
	prd.depth = 100;			// no recursive calc. anymore
}
