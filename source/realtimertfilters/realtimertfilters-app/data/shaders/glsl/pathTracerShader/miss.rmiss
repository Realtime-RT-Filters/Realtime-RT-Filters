#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
  prd.radiance = vec3(0);
  prd.depth    = 100;
  prd.albedo = vec3(1,0,0);
}
