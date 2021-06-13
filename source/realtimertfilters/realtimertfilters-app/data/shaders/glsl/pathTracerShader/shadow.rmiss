#version 460
#extension GL_EXT_ray_tracing : require
#include "binding.glsl"

layout(location = LOCATION_SHADOW) rayPayloadInEXT bool isShadowed;

void main()
{
	isShadowed = false;
}