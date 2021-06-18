#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : require

#define BIND_GUIBASE 2
#define BIND_SCENEINFO 3
#include "../ubo_definitions.glsl"

layout(set = 0, binding = 1) uniform sampler2D attachments[];

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragcolor;

#define INT_MAX 2147483647

void main()
{
	int index = (inUV.x > ubo_guibase.SplitViewFactor) ? ubo_guibase.ImageRight : ubo_guibase.ImageLeft;
	if (index != INT_MAX)
	{
		// Debug display
		outFragcolor.xyz = texture(attachments[index], inUV).xyz;
		outFragcolor.a = 1.0;
		return;
	}

	// Render-target composition

// Get G-Buffer values
	vec3 fragPos = texture(attachments[0], inUV).rgb;
	vec3 normal = texture(attachments[1], inUV).rgb;
	vec4 albedo = texture(attachments[2], inUV);

#define ambient 0.0

	// Ambient part
	vec3 fragcolor = albedo.rgb * ambient;

	for (int i = 0; i < UBO_SCENEINFO_LIGHT_COUNT; ++i)
	{
		if (ubo_sceneinfo.Lights[i].Type < 0)
		{
			continue;
		}
		// Vector to light
		vec3 L = ubo_sceneinfo.Lights[i].Position.xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L);

		// Viewer to fragment
		vec3 V = ubo_sceneinfo.ViewPos.xyz - fragPos;
		V = normalize(V);

		//if(dist < ubo.lights[i].radius)
		{
			// Light to fragment
			L = normalize(L);

			// Attenuation
			float atten = ubo_sceneinfo.Lights[i].RadiantFlux / (pow(dist, 2.0) + 1.0);
			vec3 colorNormalized = ubo_sceneinfo.Lights[i].Color;

			// Diffuse part
			vec3 N = normalize(normal);
			float NdotL = max(0.0, dot(N, L));
			vec3 diff = colorNormalized * albedo.rgb * NdotL * atten;

			// Specular part
			// Specular map values are stored in alpha of albedo mrt
			vec3 R = reflect(-L, N);
			float NdotR = max(0.0, dot(R, V));
			vec3 spec = colorNormalized * albedo.a * pow(NdotR, 16.0) * atten;

			fragcolor += diff + spec;
		}
	}

	outFragcolor = vec4(fragcolor, 1.f);
}