#version 450
#extension GL_KHR_vulkan_glsl : enable

#define BIND_ATROUSCONFIG 0
#define SET_ATROUSCONFIG 1
#include "../ubo_definitions.glsl"

layout (binding = 0) uniform sampler2D Tex_albedoMap;	//normals from G-Buffer
layout (binding = 1) uniform sampler2D Tex_normalMap;
layout (binding = 2) uniform sampler2D Tex_depthMap;	//depth from G-Buffer

layout (binding = 3, rgba32f) uniform coherent image2D Tex_DirectIntegratedColor_A;
layout (binding = 4, rgba32f) uniform coherent image2D Tex_DirectIntegratedColor_B;

layout (binding = 5, rgba32f) uniform coherent image2D Tex_IndirectIntegratedColor_A;
layout (binding = 6, rgba32f) uniform coherent image2D Tex_IndirectIntegratedColor_B;

layout (binding = 7, rgba32f) uniform coherent image2D Tex_DirectColorHistory;
layout (binding = 8, rgba32f) uniform coherent image2D Tex_IndirectColorHistory;

layout (location = 0) out vec4 Out_SVGFOutput;	

const float kernel[9]={
1.f/16.f,2.f/16.f,1.f/16.f,
  2.f/16.f,4.f/16.f,2.f/16.f,
  1.f/16.f,2.f/16.f,1.f/16.f
};

#include "../filter/filtercommon.glsl"

vec4 loadColorDirect(in int iteration, in ivec2 UV)
{
	if (iteration % 2 == 0)
	{
		return imageLoad(Tex_DirectIntegratedColor_A, UV);
	}
	return imageLoad(Tex_DirectIntegratedColor_B, UV);
}

void storeColorDirect(in int iteration, in ivec2 UV, in vec4 color)
{
	if (iteration % 2 == 1)
	{
		imageStore(Tex_DirectIntegratedColor_A, UV, color);
		return;
	}
	imageStore(Tex_DirectIntegratedColor_B, UV, color);
}

vec4 loadColorIndirect(in int iteration, in ivec2 UV)
{
	if (iteration % 2 == 0)
	{
		return imageLoad(Tex_IndirectIntegratedColor_A, UV);
	}
	return imageLoad(Tex_IndirectIntegratedColor_B, UV);
}

void storeColorIndirect(in int iteration, in ivec2 UV, in vec4 color)
{
	if (iteration % 2 == 1)
	{
		imageStore(Tex_IndirectIntegratedColor_A, UV, color);
		return;
	}
	imageStore(Tex_IndirectIntegratedColor_B, UV, color);
}


void main()
{
	vec4 directOutputColor;
	vec4 indirectOutputColor;

	for (int iteration = 0; iteration < ubo_atrousconfig.iterations; iteration++)
	{
		float c_phi = 1.f / float(iteration+1) * ubo_atrousconfig.c_phi;
		float p_phi = 1.f / float(iteration+1) * ubo_atrousconfig.p_phi;
		float n_phi = 1.f / float(iteration+1) * ubo_atrousconfig.n_phi;
		int stepw = iteration * 2 + 1;

		vec3 sum_direct = vec3(0.0);
		vec3 sum_indirect = vec3(0.0);
		vec4 colorval_direct = loadColorDirect(iteration, Texel);
		vec4 colorval_indirect = loadColorIndirect(iteration, Texel);
		vec3 normalvalue = texelFetch(Tex_normalMap, Texel, 0).xyz;
		float depthvalue = texelFetch(Tex_depthMap, Texel, 0).r;

		float cum_w_direct = 0.0;
		float cum_w_indirect = 0.0;

		if (depthvalue < 1.f) 
		{
			for (int i = 0; i < 9; i++) 
			{
				ivec2 offset = ivec2(-1 + i % 3, -1 + i / 3);
				ivec2 uv = Texel + offset * stepw;
				float depthtemp = texelFetch(Tex_depthMap, uv, 0).r;
				if (depthtemp == 1.f)
					continue;

				vec3 normaltemp = texelFetch(Tex_normalMap, uv, 0).xyz;
				float n_w = dot(normalvalue, normaltemp);
				if (n_w < 0.001)
					continue;

				vec4 colortemp_direct = loadColorDirect(iteration, uv);
				vec4 colortemp_indirect = loadColorIndirect(iteration, uv);

				vec3 ct_direct = colorval_direct.rgb - colortemp_direct.rgb;
				vec3 ct_indirect = colorval_indirect.rgb - colortemp_indirect.rgb;

				float c_w_direct = max(min(1.0 - dot(ct_direct, ct_direct) / c_phi, 1.0), 0.0);
				float c_w_indirect = max(min(1.0 - dot(ct_indirect, ct_indirect) / c_phi, 1.0), 0.0);

				float pt = abs(depthvalue - depthtemp);
				float p_w = max(min(1.0 - pt/p_phi, 1.0), 0.0);

				float weight_direct = c_w_direct * p_w * n_w * kernel[i];
				float weight_indirect = c_w_indirect * p_w * n_w * kernel[i];

				sum_direct += colortemp_direct.rgb * weight_direct;
				sum_indirect += colortemp_indirect.rgb * weight_indirect;

				cum_w_direct += weight_direct;
				cum_w_indirect += weight_indirect;
			}
		}
		else 
		{
			sum_direct = colorval_direct.xyz;
			sum_indirect = colorval_indirect.xyz;
			cum_w_direct = 1.f;
			cum_w_indirect = 1.f;
		}

		storeColorDirect(iteration, Texel, vec4(sum_direct / cum_w_direct, 1.0));
		storeColorIndirect(iteration, Texel, vec4(sum_indirect / cum_w_indirect, 1.0));

		if(iteration == 0)
		{
			imageStore(Tex_DirectColorHistory, Texel, vec4(sum_direct / cum_w_direct, 1.0));
			imageStore(Tex_IndirectColorHistory, Texel, vec4(sum_indirect / cum_w_indirect, 1.0));
		}

		if (iteration < ubo_atrousconfig.iterations - 1)
		{
			memoryBarrierImage();
		}
		else
		{
			directOutputColor = vec4(sum_direct / cum_w_direct, 1.0);
			indirectOutputColor = vec4(sum_indirect / cum_w_indirect, 1.0);
		}
	}

	// construct svgf output image1D
	vec4 albedoColor = texelFetch(Tex_albedoMap, Texel, 0);
	Out_SVGFOutput = directOutputColor * albedoColor * indirectOutputColor;
}