#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "binding.glsl"
#include "gltf.glsl"
#include "raycommon.glsl"
#include "sampling.glsl"

hitAttributeEXT vec2 intersectionPoint;

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 2) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

layout(binding = B_UBO) uniform UBO 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
	int vertexSize;
} ubo;

layout( binding = 0) uniform accelerationStructureEXT topLevelAS;
layout( binding = B_INSTANCEINFO) readonly buffer _InstanceInfo {PrimMeshInfo primInfo[];};
layout( binding = B_VERTICES) readonly buffer _VertexBuf { vec4 v[];} vertices;
layout( binding = B_INDICES) readonly buffer _Indices { uint i[];}indices;
// layout( binding = B_MATERIALS) readonly buffer _MaterialBuffer {GltfShadeMaterial m[];} materials;
// layout( binding = B_TEXTURES) uniform sampler2D texturesMap[]; // all textures

layout(push_constant) uniform Constants
{
  vec4  clearColor;
  float lightIntensity;
  int   lightType;
  int   frame;
  int   samples;
  int   bounces;
  int   bounceSamples;
  float temporalAlpha;
}
pushC;

struct Vertex{
  vec3 pos;
  vec3 normal;
  vec2 uv;
  vec4 color;
};

struct GeometryHitPoint{
  vec3 pos;
  vec3 pos_world;
  vec3 normal;
  vec3 normal_world;
  // vec2 uv;
  // vec4 color;
};

Vertex getVertex(uint index){

  // The multiplier is the size of the vertex divided by four float components (=16 bytes)
  const int m = ubo.vertexSize / 16;

	vec4 d0 = vertices.v[m * index + 0];
	vec4 d1 = vertices.v[m * index + 1];
	vec4 d2 = vertices.v[m * index + 2];

  Vertex v;
	v.pos = d0.xyz;
	v.normal = vec3(d0.w, d1.x, d1.y);
	v.uv = vec2(d1.z, d1.w);
	v.color = vec4(d2.xyz, 1.0);
  return v;
}

void initGeometryHitPoint(GeometryHitPoint hitpoint){
  // Index of vertices
  ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);
  
  // barycentrics coordinates
  const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

  // Vertex of the triangle
  Vertex v0 = getVertex(index.x);
  Vertex v1 = getVertex(index.y);
  Vertex v2 = getVertex(index.z);
  hitpoint.pos     = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
  hitpoint.pos_world = vec3(gl_ObjectToWorldEXT * vec4(hitpoint.pos, 1.0));

  // Interpolate normal of hitpoint
	vec3 normal = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
  hitpoint.normal_world = normalize(vec3(normal * gl_WorldToObjectEXT));
  hitpoint.normal  = normalize(cross(v1.pos - v0.pos, v2.pos - v0.pos));
}

vec3 getAlbedoColorOfMesh(){
  ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);
  return getVertex(index.x).color.rgb;
}

vec3 calculateIndirectLight(GeometryHitPoint hitpoint){
  vec3 attenuation = prd.attenuation * getAlbedoColorOfMesh() / M_PI;
  vec3 indirect = vec3(0);
  { 
    for(int i = 0; i < pushC.bounceSamples; i++)
    {
      vec3 origin = hitpoint.pos_world;
      vec3 direction = sampleLambert(prd.seed, createTBN(hitpoint.normal_world));
      prd.attenuation = attenuation;
      traceRayEXT(topLevelAS,            // acceleration structure
                  gl_RayFlagsOpaqueEXT,  // rayFlags
                  0xFF,                  // cullMask
                  0,                     // sbtRecordOffset
                  0,                     // sbtRecordStride
                  0,                     // missIndex
                  origin,                // ray origin
                  0.001,                 // ray min range
                  direction,             // ray direction
                  10000.0,               // ray max range
                  0                      // payload (location = 0) prd
      );
      indirect += prd.radiance;
    }
  }
  return indirect;
}

vec3 calculateDirectLight(GeometryHitPoint hitpoint){
  vec3 direct = vec3(0);
  if (pushC.lightType != -1) // direct lighting
  {
    // Vector toward the light
    vec3  L;
    float lightIntensity = pushC.lightIntensity;
    float lightDistance  = 100000.0;

    if(pushC.lightType == 0) // Point light
    {
      vec3 lDir      = ubo.lightPos.xyz - hitpoint.pos_world;
      lightDistance  = length(lDir);
      lightIntensity = pushC.lightIntensity / (lightDistance * lightDistance);
      L              = normalize(lDir);
    }
    else  // Directional light
    {
      L = normalize(ubo.lightPos.xyz - vec3(0));
    }

    // Wrong normal?
    float NdotL = max(0.0, dot(hitpoint.normal, L));
    if(NdotL > 0)
    {
      isShadowed = true;
      traceRayEXT(topLevelAS,      // acceleration structure
                  gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,       // rayFlags
                  0xFF,            // cullMask
                  0,               // sbtRecordOffset
                  0,               // sbtRecordStride
                  1,               // missIndex
                  hitpoint.pos_world,  // ray origin
                  0.001,           // ray min range
                  L,               // ray direction
                  lightDistance,   // ray max range
                  2                // payload (location = 2) shadow
      );
      if(isShadowed)
      {
        lightIntensity = 0.0;
      }
      direct += vec3(1) * NdotL * lightIntensity;
    }
  }
  return direct;
}

bool isBouncingFinished(){
  return prd.depth > pushC.bounces;
}

void main()
{
  if(isBouncingFinished())
  {
    prd.radiance = vec3(0);
    prd.normal = vec3(0);
    prd.albedo = vec3(0);
    return;
  }
  prd.depth++;
  // Temporary vars
  vec3 emittance = vec3(.1,0,0);

  // Hit point of geometry init  
  GeometryHitPoint hitpoint;
  initGeometryHitPoint(hitpoint);

  if(emittance != vec3(0))
  {
    prd.radiance = emittance;
    prd.normal   = hitpoint.normal_world;
    prd.albedo   = getAlbedoColorOfMesh();
    return;
  }

  vec3 attenuation = prd.attenuation *  prd.albedo  / M_PI;

  vec3 indirectLight = calculateIndirectLight(hitpoint);
  vec3 directLight = calculateDirectLight(hitpoint);

  prd.radiance = (indirectLight + directLight) * attenuation;
  prd.normal   = hitpoint.normal_world;
  prd.albedo   = getAlbedoColorOfMesh();
}