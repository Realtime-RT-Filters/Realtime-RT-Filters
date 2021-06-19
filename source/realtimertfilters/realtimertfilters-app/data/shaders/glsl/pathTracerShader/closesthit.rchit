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

layout(location = LOCATION_PBR) rayPayloadInEXT S_HitPayload prd;
layout(location = LOCATION_SHADOW) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

#define BIND_SCENEINFO B_UBO
#define PUSHC_PATHTRACERCONFIG 0
#include "../ubo_definitions.glsl"

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = B_VERTICES) readonly buffer _VertexBuf { vec4 v[]; } vertices;
layout(binding = B_INDICES) readonly buffer _Indices { uint i[]; }indices;
layout(binding = B_MATERIALS ) readonly buffer _MaterialBuffer {GltfShadeMaterial m[];} materials;
layout(binding = B_TEXTURES) uniform sampler2D textureMap[]; // all textures

struct S_Vertex 
{
	vec3 pos;
	vec3 normal;
	vec2 uv;
	vec4 color;
	vec4 joint0;
	vec4 weight0;
	vec4 tangent;
	int materialId;
	int meshId;
};

struct S_GeometryHitPoint
{
	vec3 pos;
	vec3 pos_world;
	vec3 normal;
	vec3 normal_world;
	vec2 uv;
	vec3 albedo;
	int materialId;
};

S_Vertex getVertex(uint index)
{
	// The multiplier is the size of the vertex divided by four float components (=16 bytes)
	const int m = int(config.VertexSize) / 16;		//ubo.vertexSize / 16;

	vec4 d0 = vertices.v[m * index + 0];	
	vec4 d1 = vertices.v[m * index + 1];
	vec4 d2 = vertices.v[m * index + 2];			// color
	vec4 d3 = vertices.v[m * index + 3];			// joint0
	vec4 d4 = vertices.v[m * index + 4];			// weight0
	vec4 d5 = vertices.v[m * index + 5];			// tangent
	vec2 d6 = vertices.v[m * index + 6].xy;			// material and  meshId

	S_Vertex v;
	v.pos = d0.xyz;
	v.normal = vec3(d0.w, d1.xy);
	v.uv = vec2(d1.z, d1.w);
	v.color = vec4(d2.xyz, 1.0);
	v.joint0 = d3;
	v.weight0 = d4;
	v.tangent = d5;
	v.materialId = floatBitsToInt(d6.x);
	return v;
}

bool hasTexture(int texId){
	return texId > -1;
}

S_GeometryHitPoint initGeometryHitPoint()
{
	S_GeometryHitPoint hitpoint;
	// Index of vertices
	ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

	// barycentrics coordinates
	const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	// Vertex of the triangle
	S_Vertex v0 = getVertex(index.x);
	S_Vertex v1 = getVertex(index.y);
	S_Vertex v2 = getVertex(index.z);
	hitpoint.pos = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
	hitpoint.pos_world = vec3(gl_ObjectToWorldEXT * vec4(hitpoint.pos, 1.0));

	// Interpolate normal of hitpoint
	vec3 normal = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
	hitpoint.normal_world = normalize(vec3(normal * gl_WorldToObjectEXT));
	hitpoint.normal = normalize(cross(v1.pos - v0.pos, v2.pos - v0.pos));

	// Calculate uv and color/texture
	hitpoint.uv = v0.uv * barycentrics.x + v1.uv * barycentrics.y + v2.uv * barycentrics.z;
	hitpoint.materialId = v0.materialId;
	// float texId = (materials.m[hitpoint.materialId].baseColorTextureId.x);
	int texId = int(materials.m[hitpoint.materialId].baseColorTextureId.x);
	// hitpoint.albedo = vec3(texId,1,1);
	if (hasTexture(texId)){
		hitpoint.albedo = texture(textureMap[texId], hitpoint.uv).rgb;
	} else {
		hitpoint.albedo = materials.m[hitpoint.materialId].baseColorFactor.rgb;
		// Read color from vertices
		// hitpoint.albedo = v0.color.xyz * barycentrics.x + v1.color.xyz * barycentrics.y + v2.color.xyz * barycentrics.z;
	}
	return hitpoint;
}

vec3 calculateIndirectLight(S_GeometryHitPoint hitpoint)
{
	if (prd.depth >= config.MaxBounceDepth)
	{
		return vec3(0.0);
	}

	vec3 attenuation = prd.attenuation * hitpoint.albedo / M_PI;
	vec3 indirect = vec3(0);
	{
		for (int i = 0; i < config.SecondarySamplesPerBounce; i++)
		{
			vec3 origin = hitpoint.pos_world;
			vec3 direction = sampleLambert(prd.seed, createTBN(hitpoint.normal_world));
			//vec3 direction = sampleGGX(prd.seed, createTBN(hitpoint.normal_world), 1.f);
			prd.attenuation = attenuation;
			traceRayEXT(
				topLevelAS,				// acceleration structure
				gl_RayFlagsOpaqueEXT,	// rayFlags
				0xFF,					// cullMask
				0,						// sbtRecordOffset
				0,						// sbtRecordStride
				0,						// missIndex
				origin,					// ray origin
				0.001,					// ray min range
				direction,				// ray direction
				10000.0,				// ray max range
				LOCATION_PBR			// payload (location = 0) prd
			);
			indirect += prd.radiance;
		}
		indirect /= config.SecondarySamplesPerBounce;
		indirect *= attenuation;
	}
	return indirect;
}

vec3 sampleLi(in S_GeometryHitPoint hitpoint, in S_Light light, in float lightDistance, in float NdotL)
{
	if (light.Type == 1.0)
	{
		float distFactor = 1.0 / (lightDistance * lightDistance);
		return NdotL * distFactor * light.RadiantFlux * light.Color;
	}
	else
	{
		return NdotL * light.RadiantFlux * light.Color;
	}
}

vec3 calculateDirectLight(in S_GeometryHitPoint hitpoint)
{
	vec3 direct = vec3(0);
	if (ubo_sceneinfo.Lights[0].Type >= 0) // direct lighting
	{
		// Vector toward the light
		vec3 lightDir;
		float lightDistance = 100000.0;

		if (ubo_sceneinfo.Lights[0].Type == 1.0) // Point light
		{
			vec3 lightVector = ubo_sceneinfo.Lights[0].Position.xyz - hitpoint.pos_world;
			lightDistance = length(lightVector);
			lightDir = normalize(lightVector);
		}
		else // Directional light
		{
			lightDir = normalize(ubo_sceneinfo.Lights[0].Position.xyz);
		}

		const float BIAS = 0.001;

		// Casting shadow ray
		float tmin = BIAS;							// Minimum travel distance means the ray cannot re-intersect with the same primitive this closest hit shader was called for.
		float tmax = lightDistance - BIAS;			// Preventing intersects "behind" the light source.
		uint rayflags =
			gl_RayFlagsTerminateOnFirstHitEXT |		// We only care about any intersect, as this is a visibility test.
			gl_RayFlagsOpaqueEXT |					// Intersect tests with opaque geometry only.
			gl_RayFlagsSkipClosestHitShaderEXT;		// No information about closest hit is required, only wether it exists or not.
		vec3 origin =
			hitpoint.pos_world + hitpoint.normal_world * BIAS;			// Origin bias away from geometry to prevent artifacts
		float NdotL = max(0.0, dot(hitpoint.normal_world, lightDir));
		isShadowed = true;
		if (NdotL > 0)				// Only do shadow tests when the surface faces the light source
		{
			// Trace shadow ray and offset indices to match shadow hit/miss shader group indices
			traceRayEXT(topLevelAS,
				rayflags,
				0xFF,					// Cullmask, can be used to remove geometry from intersection tests.
				1,						// SBT offset and
				0,						// SBT stride are both used to select the shader to call for the result of the raycast.
				1,						// Additionally which miss shader is called can be adjusted here with this offset.
				origin,					// Ray origin in world space.
				tmin,
				lightDir,				// The direction of the ray.
				tmax,
				LOCATION_SHADOW			// Layout location index the rayPayloadEXT has been assigned to in this shader.
			);
		}
		// In case of shadow we reduce the color level and don't generate a fake specular highlight
		if (!isShadowed)
		{
			direct += sampleLi(hitpoint, ubo_sceneinfo.Lights[0], lightDistance, NdotL);
		}
	}
	return direct;
}

void main()
{
	prd.depth++;

	// // Hit point of geometry init
	S_GeometryHitPoint hitpoint = initGeometryHitPoint();
	
	// vec3 attenuation = prd.albedo / M_PI;
	// vec3 attenuation = vec3(1.0);

	vec3 indirectLigthing = calculateIndirectLight(hitpoint);
	vec3 directLighting  = calculateDirectLight(hitpoint);

	prd.radiance = (directLighting + indirectLigthing) * hitpoint.albedo;
//	prd.radiance = (indirectLigthing + directLighting ) * attenuation;
	if (prd.depth == 1)
	{
		//prd.radianceIndirect = indirectLigthing;
		//prd.radianceDirect = directLighting;
	}

	prd.normal = hitpoint.normal_world;
	prd.albedo = hitpoint.albedo;
}