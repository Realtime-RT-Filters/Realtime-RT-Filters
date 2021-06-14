#version 450
#extension GL_KHR_vulkan_glsl : enable
#define BIND_ATROUSCONFIG 0
#define SET_ATROUSCONFIG 1
#include "../ubo_definitions.glsl"

layout (binding = 0, rgba32f) uniform coherent image2D Tex_colorA;
layout (binding = 1, rgba32f) uniform coherent image2D Tex_colorB;
layout (binding = 2) uniform sampler2D Tex_normalMap;	//normals from G-Buffer
layout (binding = 3) uniform sampler2D Tex_depthMap;	//depth from G-Buffer

const float kernel[9]={
1.f/16.f,2.f/16.f,1.f/16.f,
  2.f/16.f,4.f/16.f,2.f/16.f,
  1.f/16.f,2.f/16.f,1.f/16.f
};

layout (location = 0) out vec4 Filteroutput;

#include "filtercommon.glsl"

vec4 loadColor(in int iteration, in ivec2 UV)
{
	if (iteration % 2 == 0)
	{
		return imageLoad(Tex_colorA, UV);
	}
	return imageLoad(Tex_colorB, UV);
}

void storeColor(in int iteration, in ivec2 UV, in vec4 color)
{
	if (iteration % 2 == 1)
	{
		imageStore(Tex_colorA, UV, color);
		return;
	}
	imageStore(Tex_colorB, UV, color);
}

#define ITERATIONS 5

void main()
{
	for (int iteration = 0; iteration < ITERATIONS; iteration++)
	{
		float c_phi = 1.f / float(iteration) * ubo_atrousconfig.c_phi;
		float p_phi = 1.f / float(iteration) * ubo_atrousconfig.p_phi;
		float n_phi = 1.f / float(iteration) * ubo_atrousconfig.n_phi;
		int stepw = iteration * 2 + 1;

		vec3 sum = vec3(0.0);
		vec4 colorval = loadColor(iteration, Texel);
		vec3 normalvalue = texelFetch(Tex_normalMap, Texel, 0).xyz;
		float depthvalue = texelFetch(Tex_depthMap, Texel, 0).r;
		float cum_w = 0.0;

		for (int i = 0; i < 9; i++) 
		{
			ivec2 offset = ivec2(-1 + i % 3, -1 + i / 3);
			ivec2 uv = Texel + offset * stepw;
			float depthtemp = texelFetch(Tex_depthMap, uv, 0).r;
			if (isnan(depthtemp))
				continue;
			vec3 normaltemp = texelFetch(Tex_normalMap, uv, 0).xyz;
			float n_w = dot(normalvalue, normaltemp);
			if (n_w < 1E-3)
				continue;
			vec4 colortemp = loadColor(iteration, uv);
			vec3 ct = colorval.rgb - colortemp.rgb;
			float c_w = max(min(1.0 - dot(ct, ct) / c_phi, 1.0), 0.0);
			float pt = abs(depthvalue - depthtemp);
			float p_w = max(min(1.0 - pt/p_phi, 1.0), 0.0);
			float weight = c_w * p_w * n_w * kernel[i];
			sum += colortemp.rgb * weight;
			cum_w += weight;
		}
		storeColor(iteration, Texel, vec4(sum / cum_w, 1.0));
		if (iteration < ITERATIONS - 1)
		{
			memoryBarrierImage();
		}
	}
}