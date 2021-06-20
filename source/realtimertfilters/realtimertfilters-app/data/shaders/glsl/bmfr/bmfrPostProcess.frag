#version 420

/*
	This filter pass temporally accumulates the regression output, reuising the history length information from the previous temporal accumulation. Albedo is also added back to the output
*/

// GBUFFER INPUTS
layout (set = 0, binding = 0) uniform sampler2D Tex_Motion;				// Screenspace motion vector

// PATHTRACER INPUTS/OUTPUTS
layout (set = 0, binding = 1) uniform sampler2D Tex_Regression;			// Raw RT input
layout (set = 0, binding = 2) uniform sampler2D Tex_PrevAccuRegression;	// Previous frame Accumulated RT input
layout (location = 0) out vec4 Out_NewAccuRegression;					// Accumulated RT input

// ACCUMULATION SPECIFIC VARIABLES
layout (set = 0, binding = 3) uniform isampler2D Tex_HistoryLength;		// Per pixel accumulated data age

// Albedo in
layout (set = 0, binding = 4) uniform sampler2D Tex_Albedo;				// Albedo color

#include "../filter/filtercommon.glsl"

#define BIND_ACCUCONFIG 0
#define SET_ACCUCONFIG 1
#include "../ubo_definitions.glsl"

void main()
{
	vec3 rawColor = texelFetch(Tex_Regression, Texel, 0).xyz * texelFetch(Tex_Albedo, Texel, 0).xyz;

	// Set the RT Output values and a 1 to new history length by default
	Out_NewAccuRegression = vec4(rawColor, 1.0);

	vec2 motionVec = texelFetch(Tex_Motion, Texel, 0).xy;
	vec2 uv_prevFrame = UV + motionVec;
	ivec2 texel_prevFrame = TexelizeCoords(uv_prevFrame);
	int historylength = texelFetch(Tex_HistoryLength, texel_prevFrame, 0).x;	// HistoryLength = number of previous frames that we didn't yet discard

	if (ubo_accuconfig.EnableAccumulation == 0 || 
		historylength <= 1)							// A history length of 1 means the texel was discarded in the preprocess pass
	{
		return;
	}


	float mixFactor = max(ubo_accuconfig.MinNewWeight, 1.f / historylength);
	vec3 oldColor = texelFetch(Tex_PrevAccuRegression, texel_prevFrame, 0).xyz;
	vec3 mixedColor = mix(oldColor, rawColor,  mixFactor);
	Out_NewAccuRegression =vec4(mixedColor, 1.0);
}