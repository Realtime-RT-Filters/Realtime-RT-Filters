#version 420
#extension GL_KHR_vulkan_glsl : enable
#define BIND_ATROUSCONFIG 0
#define SET_ATROUSCONFIG 1
#include "../ubo_definitions.glsl"

layout(binding=0)uniform sampler2D Tex_colorMap;	//color data from SVGF
layout(binding=1)uniform sampler2D Tex_normalMap;	//normals from G-Buffer
layout(binding=2)uniform sampler2D Tex_depthMap;	//depth from G-Buffer

const float kernel[9]={
1.f/16.f,2.f/16.f,1.f/16.f,
  2.f/16.f,4.f/16.f,2.f/16.f,
  1.f/16.f,2.f/16.f,1.f/16.f
};

layout (location = 0) out vec4 Filteroutput;

#include "filtercommon.glsl"

void main()
{
	vec3 sum = vec3(0.0);
	vec4 colorval = texelFetch(Tex_colorMap, Texel, 0);
	float sampleFrame = colorval.a;
	float sf2 = sampleFrame*sampleFrame;
	vec3 normalvalue = texelFetch(Tex_normalMap, Texel, 0).xyz;
	float depthvalue = texelFetch(Tex_depthMap, Texel, 0).r;
	float cum_w = 0.0;
	for (int i = 0; i < 9; i++) 
	{
		ivec2 offset = ivec2(-1 + i % 3, -1 + i / 3);
		ivec2 uv = Texel + offset * ubo_atrousconfig.stepwidth;
		float depthtemp = texelFetch(Tex_depthMap, uv, 0).r;
		if (isnan(depthtemp))
			continue;
		vec3 normaltemp = texelFetch(Tex_normalMap, uv, 0).xyz;
		float n_w = dot(normalvalue, normaltemp);
		if (n_w < 1E-3)
			continue;
		vec4 colortemp = texelFetch(Tex_colorMap, uv, 0);
		vec3 ct = colorval.rgb - colortemp.rgb;
		float c_w = max(min(1.0 - dot(ct, ct) / ubo_atrousconfig.c_phi * sf2, 1.0), 0.0);
		float pt = abs(depthvalue - depthtemp);
		float p_w = max(min(1.0 - pt/ubo_atrousconfig.p_phi, 1.0), 0.0);
		float weight = c_w * p_w * n_w * kernel[i];
		sum += colortemp.rgb * weight;
		cum_w += weight;
	}
	Filteroutput = vec4(sum / cum_w, sampleFrame);
}