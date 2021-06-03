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
// layout( binding = B_INSTANCEINFO) readonly buffer _InstanceInfo {PrimMeshInfo primInfo[];};
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

void main()
{
  ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);
  // // Vertex of the triangle
  Vertex v0 = getVertex(index.x);
  Vertex v1 = getVertex(index.y);
  Vertex v2 = getVertex(index.z);

  	// Interpolate normal
  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);

	// Basic lighting
	vec3 lightVector = normalize(ubo.lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.2);
	prd.albedo = v0.color.rgb * dot_product;

  // Shadow casting
	float tmin = 0.001;
	float tmax = 10000.0;
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	isShadowed = true;  

  traceRayEXT(
      topLevelAS,
      gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 
      0xFF, 
      1, 
      0, 
      1, 
      origin, 
      tmin, 
      lightVector, 
      tmax, 
      0
      );
	if (isShadowed) {
			prd.albedo  *= 0.3;
	}

  // if(prd.depth > pushC.bounces)
  // {
  //   prd.radiance = vec3(0);
  //   prd.normal = vec3(0);
  //   prd.albedo = vec3(0);
  //   return;
  // }

  // prd.depth++;
  
  // // Retrieve the Primitive mesh buffer information
  // PrimMeshInfo pinfo = primInfo[gl_InstanceCustomIndexEXT];

  // // Getting the 'first index' for this mesh (offset of the mesh + offset of the triangle)
  // uint indexOffset  = pinfo.indexOffset + (3 * gl_PrimitiveID);
  // uint vertexOffset = pinfo.vertexOffset;           // Vertex offset as defined in glTF
  // uint matIndex     = max(0, pinfo.materialIndex);  // material of primitive mesh

  // // Getting the 3 indices of the triangle (local)
  // ivec3 triangleIndex = ivec3(indices.i[nonuniformEXT(indexOffset + 0)],  
  //                               indices.i[nonuniformEXT(indexOffset + 1)],  
  //                               indices.i[nonuniformEXT(indexOffset + 2)]);
  // triangleIndex += ivec3(vertexOffset);  // (global)

  // const vec3 barycentrics = vec3(1.0 - intersectionPoint.x - intersectionPoint.y, intersectionPoint.x, intersectionPoint.y);

	// ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);
  // // // Vertex of the triangle
  // Vertex v0 = getVertex(index.x);
  // Vertex v1 = getVertex(index.y);
  // Vertex v2 = getVertex(index.z);
  // const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	// vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);

	// // Basic lighting
	// vec3 lightVector = normalize(ubo.lightPos.xyz);
	// float dot_product = max(dot(lightVector, normal), 0.2);
	// vec3 hitValue = v0.color.rgb * dot_product;

  // // Position
  // const vec3 pos0           = v0.pos;
  // const vec3 pos1           = v1.pos;
  // const vec3 pos2           = v2.pos;
  // const vec3 position       = pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z;
  // const vec3 world_position = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));

  // // Normal
  // const vec3 nrm0 = v0.normal;
  // const vec3 nrm1 = v1.normal;
  // const vec3 nrm2 = v2.normal;
  // vec3 normal = normalize(nrm0 * barycentrics.x + nrm1 * barycentrics.y + nrm2 * barycentrics.z);
  // const vec3 world_normal = normalize(vec3(normal * gl_WorldToObjectEXT));
  // const vec3 geom_normal  = normalize(cross(pos1 - pos0, pos2 - pos0));

  // // TexCoord
  // const vec2 uv0       = v0.uv;
  // const vec2 uv1       = v1.uv;;
  // const vec2 uv2       = v2.uv;
  // const vec2 texcoord0 = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

  // GltfShadeMaterial mat = materials.m[nonuniformEXT(matIndex)];
  // vec3  emittance = mat.emissiveFactor;
  // vec3  albedo    = mat.pbrBaseColorFactor.xyz;

  // if(mat.pbrBaseColorTexture > -1)
  // {
  //   uint txtId = mat.pbrBaseColorTexture;
  //   albedo *= texture(texturesMap[nonuniformEXT(txtId)], texcoord0).xyz;
  // }

  // if(emittance != vec3(0))
  // {
  //   prd.radiance = emittance;
  //   prd.normal   = world_normal;
  //   prd.albedo   = albedo;
  //   return;
  // }

  // vec3 attenuation = prd.attenuation * albedo / M_PI;
  // vec3 indirect = vec3(0);
  // { // indirect lighting
  //   for(int i = 0; i < pushC.bounceSamples; i++)
  //   {
  //     vec3 origin = world_position;
  //     vec3 direction = sampleLambert(prd.seed, createTBN(world_normal));
  //     prd.attenuation = attenuation;

  //     traceRayEXT(topLevelAS,            // acceleration structure
  //                 gl_RayFlagsOpaqueEXT,  // rayFlags
  //                 0xFF,                  // cullMask
  //                 0,                     // sbtRecordOffset
  //                 0,                     // sbtRecordStride
  //                 0,                     // missIndex
  //                 origin,                // ray origin
  //                 0.001,                 // ray min range
  //                 direction,             // ray direction
  //                 10000.0,               // ray max range
  //                 0                      // payload (location = 0)
  //     );

  //     indirect += prd.radiance;
  //   }
  // }

  // vec3 direct = vec3(0);
  // if (pushC.lightType != -1) // direct lighting
  // {
  //   // Vector toward the light
  //   vec3  L;
  //   float lightIntensity = pushC.lightIntensity;
  //   float lightDistance  = 100000.0;

  //   if(pushC.lightType == 0) // Point light
  //   {
  //     vec3 lDir      = ubo.lightPos.xyz - world_position;
  //     lightDistance  = length(lDir);
  //     lightIntensity = pushC.lightIntensity / (lightDistance * lightDistance);
  //     L              = normalize(lDir);
  //   }
  //   else  // Directional light
  //   {
  //     L = normalize(ubo.lightPos.xyz - vec3(0));
  //   }

  //   float NdotL = max(0.0, dot(normal, L));
  //   if(NdotL > 0)
  //   {
  //     isShadowed = true;
  //     traceRayEXT(topLevelAS,      // acceleration structure
  //                 gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,       // rayFlags
  //                 0xFF,            // cullMask
  //                 0,               // sbtRecordOffset
  //                 0,               // sbtRecordStride
  //                 1,               // missIndex
  //                 world_position,  // ray origin
  //                 0.001,           // ray min range
  //                 L,               // ray direction
  //                 lightDistance,   // ray max range
  //                 1                // payload (location = 1)
  //     );
  //     if(isShadowed)
  //     {
  //       lightIntensity = 0.0;
  //     }

  //     direct += vec3(1) * NdotL * lightIntensity;
  //   }
  // }
  
  // prd.radiance = (indirect + direct) * attenuation;
  // prd.normal   = world_normal;
  // prd.albedo   = albedo;
}