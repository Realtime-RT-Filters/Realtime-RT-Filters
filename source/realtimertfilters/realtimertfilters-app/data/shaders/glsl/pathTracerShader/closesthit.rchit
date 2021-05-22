#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "binding.glsl"
#include "gltf.glsl"
#include "raycommon.glsl"
#include "sampling.glsl"


hitAttributeEXT vec2 attribs;

// clang-format off
layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(set = 0, binding = 0 ) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 3) readonly buffer _InstanceInfo {PrimMeshInfo primInfo[];};

layout(set = 1, binding = B_VERTICES) readonly buffer _VertexBuf {float vertices[];};
layout(set = 1, binding = B_INDICES) readonly buffer _Indices {uint indices[];};
layout(set = 1, binding = B_NORMALS) readonly buffer _NormalBuf {float normals[];};
layout(set = 1, binding = B_TEXCOORDS) readonly buffer _TexCoordBuf {float texcoord0[];};
layout(set = 1, binding = B_MATERIALS) readonly buffer _MaterialBuffer {GltfShadeMaterial materials[];};
layout(set = 1, binding = B_TEXTURES) uniform sampler2D texturesMap[]; // all textures


// clang-format on

layout(push_constant) uniform Constants
{
  vec4  clearColor;
  vec3  lightPosition;
  float lightIntensity;
  int   lightType;
  int   frame;
  int   samples;
  int   bounces;
  int   bounceSamples;
  float temporalAlpha;
}
pushC;

// Return the vertex position
vec3 getVertex(uint index)
{
  vec3 vp;
  vp.x = vertices[3 * index + 0];
  vp.y = vertices[3 * index + 1];
  vp.z = vertices[3 * index + 2];
  return vp;
}

vec3 getNormal(uint index)
{
  vec3 vp;
  vp.x = normals[3 * index + 0];
  vp.y = normals[3 * index + 1];
  vp.z = normals[3 * index + 2];
  return vp;
}

vec2 getTexCoord(uint index)
{
  vec2 vp;
  vp.x = texcoord0[2 * index + 0];
  vp.y = texcoord0[2 * index + 1];
  return vp;
}


void main()
{
  if(prd.depth > pushC.bounces)
  {
    prd.radiance = vec3(0);
    prd.normal = vec3(0);
    prd.albedo = vec3(0);

    return;
  }

  prd.depth++;

  // Retrieve the Primitive mesh buffer information
  PrimMeshInfo pinfo = primInfo[gl_InstanceCustomIndexEXT];

  // Getting the 'first index' for this mesh (offset of the mesh + offset of the triangle)
  uint indexOffset  = pinfo.indexOffset + (3 * gl_PrimitiveID);
  uint vertexOffset = pinfo.vertexOffset;           // Vertex offset as defined in glTF
  uint matIndex     = max(0, pinfo.materialIndex);  // material of primitive mesh

  // Getting the 3 indices of the triangle (local)
  ivec3 triangleIndex = ivec3(indices[nonuniformEXT(indexOffset + 0)],  //
                                indices[nonuniformEXT(indexOffset + 1)],  //
                                indices[nonuniformEXT(indexOffset + 2)]);
  triangleIndex += ivec3(vertexOffset);  // (global)

  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  // Vertex of the triangle
  const vec3 pos0           = getVertex(triangleIndex.x);
  const vec3 pos1           = getVertex(triangleIndex.y);
  const vec3 pos2           = getVertex(triangleIndex.z);
  const vec3 position       = pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z;
  const vec3 world_position = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));

  // Normal
  const vec3 nrm0 = getNormal(triangleIndex.x);
  const vec3 nrm1 = getNormal(triangleIndex.y);
  const vec3 nrm2 = getNormal(triangleIndex.z);
  vec3 normal = normalize(nrm0 * barycentrics.x + nrm1 * barycentrics.y + nrm2 * barycentrics.z);
  const vec3 world_normal = normalize(vec3(normal * gl_WorldToObjectEXT));
  const vec3 geom_normal  = normalize(cross(pos1 - pos0, pos2 - pos0));

  // TexCoord
  const vec2 uv0       = getTexCoord(triangleIndex.x);
  const vec2 uv1       = getTexCoord(triangleIndex.y);
  const vec2 uv2       = getTexCoord(triangleIndex.z);
  const vec2 texcoord0 = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

  // https://en.wikipedia.org/wiki/Path_tracing
  // Material of the object
  GltfShadeMaterial mat       = materials[nonuniformEXT(matIndex)];
  vec3              emittance = mat.emissiveFactor;

  vec3  albedo    = mat.pbrBaseColorFactor.xyz;
  if(mat.pbrBaseColorTexture > -1)
  {
    uint txtId = mat.pbrBaseColorTexture;
    albedo *= texture(texturesMap[nonuniformEXT(txtId)], texcoord0).xyz;
  }

  if(emittance != vec3(0))
  {
    prd.radiance = emittance;
    prd.normal   = world_normal;
    prd.albedo   = albedo;
    return;
  }

  vec3 attenuation = prd.attenuation * albedo / M_PI;
  vec3 indirect = vec3(0);
  { // indirect lighting
    for(int i = 0; i < pushC.bounceSamples; i++)
    {
      vec3 origin = world_position;
      //vec3 rayDirection = reflect(prd.rayDirection, world_normal);
      vec3 direction = sampleLambert(prd.seed, createTBN(world_normal));
      //vec3 rayDirection = samplePhong(prd.seed, createTBN(reflect(prd.rayDirection, world_normal)), 8.0f);
      //vec3 rayDirection =  sampleGGX(prd.seed, createTBN(reflect(prd.rayDirection, world_normal)), 0.2f);

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
                  0                      // payload (location = 0)
      );

      indirect += prd.radiance;
    }
  }

  //indirect /= pushC.bounceSamples;

  vec3 direct = vec3(0);
  if (pushC.lightType != -1) // direct lighting
  {
    // Vector toward the light
    vec3  L;
    float lightIntensity = pushC.lightIntensity;
    float lightDistance  = 100000.0;

    if(pushC.lightType == 0) // Point light
    {
      vec3 lDir      = pushC.lightPosition - world_position;
      lightDistance  = length(lDir);
      lightIntensity = pushC.lightIntensity / (lightDistance * lightDistance);
      L              = normalize(lDir);
    }
    else  // Directional light
    {
      L = normalize(pushC.lightPosition - vec3(0));
    }

    float NdotL = max(0.0, dot(normal, L));
    if(NdotL > 0)
    {
      isShadowed = true;
      traceRayEXT(topLevelAS,      // acceleration structure
                  gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,       // rayFlags
                  0xFF,            // cullMask
                  0,               // sbtRecordOffset
                  0,               // sbtRecordStride
                  1,               // missIndex
                  world_position,  // ray origin
                  0.001,           // ray min range
                  L,               // ray direction
                  lightDistance,   // ray max range
                  1                // payload (location = 1)
      );
      if(isShadowed)
      {
        lightIntensity = 0.0;
      }

      direct += vec3(1) * NdotL * lightIntensity;
    }
  }
  
  prd.radiance = (indirect + direct) * attenuation;
  prd.normal   = world_normal;
  prd.albedo   = albedo;
}