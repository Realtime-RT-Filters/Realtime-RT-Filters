#version 420
#extension GL_KHR_vulkan_glsl : enable

/*
	This filter pass temporally accumulates the regression output, reuising the history length information from the previous temporal accumulation. Albedo is also added back to the output
*/

// GBUFFER INPUTS
layout (set = 0, binding = 2) uniform sampler2D Tex_Motion;				// Screenspace motion vector

// PATHTRACER INPUTS/OUTPUTS
layout (set = 0, binding = 3) uniform sampler2D Tex_Regression;			// Raw RT input
layout (set = 0, binding = 4) uniform sampler2D Tex_PrevAccuRegression;	// Previous frame Accumulated RT input
layout (location = 0) out vec4 Out_NewAccuRegression;					// Accumulated RT input
layout (location = 1) out vec4 Out_Filteroutput;						// Filter output (AccuRegression + Albedo)

// ACCUMULATION SPECIFIC VARIABLES
layout (set = 0, binding = 7) uniform isampler2D Tex_HistoryLength;		// Per pixel accumulated data age

// Albedo in
layout (set = 0, binding = 8) uniform sampler2D Tex_Albedo;				// Albedo color

#include "../filter/filtercommon.glsl"

#define BIND_BMFRCONFIG 0
#define SET_BMFRCONFIG 1
#include "../ubo_definitions.glsl"

void main()
{
	vec3 rawColor = texelFetch(Tex_Regression, Texel, 0).xyz;

	// Set the RT Output values and a 1 to new history length by default
	Out_NewAccuRegression = vec4(rawColor, 1.0);

	vec2 motionVec = texelFetch(Tex_Motion, Texel, 0).xy;
	vec2 uv_prevFrame = UV + motionVec;
	ivec2 texel_prevFrame = TexelizeCoords(uv_prevFrame);
	int historylength = texelFetch(Tex_HistoryLength, texel_prevFrame, 0).x;	// HistoryLength = number of previous frames that we didn't yet discard

	if (ubo_bmfrconfig.EnableAccumulation == 0 || 
		historylength <= 1)							// A history length of 1 means the texel was discarded in the preprocess pass
	{
		return;
	}


	// History = 0: Previous pixel didn't exist (could only happen in very rare circumstances in the very first frame) => We mix raw 1, accu 0
	// History = 1: History was discarded previously => We mix raw 0.5, accu 0.5
	// History > 1: History is pretty old and useful => We mix up to raw MinNewWeight, accu 1 - MinNewWeight
	float mixFactor = max(ubo_bmfrconfig.MinNewWeight, 1.f / historylength);
	vec3 oldColor = texelFetch(Tex_PrevAccuRegression, texel_prevFrame, 0).xyz;
	vec3 mixedColor = mix(oldColor, rawColor,  mixFactor);
	Out_NewAccuRegression =vec4(mixedColor, 1.0);
}