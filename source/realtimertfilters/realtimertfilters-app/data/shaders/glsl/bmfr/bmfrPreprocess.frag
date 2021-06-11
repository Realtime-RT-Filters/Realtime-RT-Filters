#version 420
#extension GL_KHR_vulkan_glsl : enable

/*
	This filter pass removes albedo from RT data and calculates temporal accumulation
*/


// GBUFFER INPUTS
layout (set = 0, binding = 0) uniform sampler2D Tex_CurPos;				// GBuffer Worldspace Positions
layout (set = 0, binding = 1) uniform sampler2D Tex_CurNormal;			// GBuffer Worldspace Normals
layout (set = 0, binding = 2) uniform sampler2D Tex_Motion;				// Screenspace motion vector

// PATHTRACER INPUTS/OUTPUTS
layout (set = 0, binding = 3) uniform sampler2D Tex_RawColor;			// Raw RT input
layout (set = 0, binding = 4) uniform sampler2D Tex_PrevAccuColor;		// Previous frame Accumulated RT input
layout (location = 0) out vec4 Out_NewAccuColor;						// Accumulated RT input

// ACCUMULATION SPECIFIC VARIABLES
layout (set = 0, binding = 5) uniform sampler2D Tex_PrevPos;			// Previous frame Worldspace Positions
layout (set = 0, binding = 6) uniform sampler2D Tex_PrevNormal;			// Previous frame Worldspace Normals
layout (set = 0, binding = 7) uniform isampler2D Tex_OldHistoryLength;	// Per pixel accumulated data age
layout (location = 1) out int Out_NewHistoryLength;						// Per pixel accumulated data age (new)

// Albedo in
layout (set = 0, binding = 8) uniform sampler2D Tex_Albedo;				// Albedo color

#include "../filter/filtercommon.glsl"

#define BIND_BMFRCONFIG 0
#define SET_BMFRCONFIG 1
#include "../ubo_definitions.glsl"

void main()
{
	vec3 rawColor = texelFetch(Tex_RawColor, Texel, 0).xyz - 
	texelFetch(Tex_Albedo, Texel, 0).xyz; // Albedo is removed from RT component, so that all filtering be done on the raw light data, rather than first hit albedo mixed in

	// Set the RT Output values and a 1 to new history length by default
	Out_NewAccuColor = vec4(rawColor, 1.0);
	Out_NewHistoryLength = 1;

	if (ubo_bmfrconfig.EnableAccumulation == 0)
	{
		return;
	}

	vec2 motionVec = texelFetch(Tex_Motion, Texel, 0).xy;
	vec2 uv_prevFrame = UV + motionVec;
	ivec2 texel_prevFrame = TexelizeCoords(uv_prevFrame);

	// Check if prev frame UV is outside of view frustrum
	bool discard_viewFrustrum = uv_prevFrame.x < 0.f || uv_prevFrame.y < 0.f || uv_prevFrame.x > 1.f || uv_prevFrame.y > 1.f;


	if (discard_viewFrustrum)
	{
		// The new pixel was outside the view frustrum in previous frame, therefor we cannot accumulate.
		return;
	}
	vec3 curPos = texelFetch(Tex_CurPos, Texel, 0).xyz;						// Current worldspace position of the fragment
	vec3 prevPos = texelFetch(Tex_PrevPos, texel_prevFrame, 0).xyz;			// Previous worldspace position of the fragment
	vec3 positionDiffVector = prevPos - curPos;								// Difference between previous and current worldspace positions
	float distanceSquared = dot(positionDiffVector, positionDiffVector);	// Squared distance between previous and current worlspace positions

	vec3 curNormal = texelFetch(Tex_CurNormal, Texel, 0).xyz;				// Current worldspace normal of the fragment
	vec3 prevNormal = texelFetch(Tex_PrevNormal, texel_prevFrame, 0).xyz;	// Previous worldspace normal of the fragment
	float angleDiff = acos(dot(curNormal, prevNormal));						// Difference between previous and current normals in radians

	bool keep = (distanceSquared < ubo_bmfrconfig.MaxPosDifference) &&		// If worldspace position difference is too great, we discard
		(angleDiff < ubo_bmfrconfig.MaxNormalAngleDifference);				// If worldspace normal angle deviation is too great, we discard

	if (keep)
	{
		int historylength = texelFetch(Tex_OldHistoryLength, texel_prevFrame, 0).x;	// HistoryLength = number of previous frames that we didn't yet discard

		// History = 0: Previous pixel didn't exist (could only happen in very rare circumstances in the very first frame) => We mix raw 1, accu 0
		// History = 1: History was discarded previously => We mix raw 0.5, accu 0.5
		// History > 1: History is pretty old and useful => We mix up to raw MinNewWeight, accu 1 - MinNewWeight
		float mixFactor = max(ubo_bmfrconfig.MinNewWeight, 1.f / (historylength + 1));
		vec3 oldColor = texelFetch(Tex_PrevAccuColor, texel_prevFrame, 0).xyz;
		vec3 mixedColor = mix(oldColor, rawColor,  mixFactor);
		Out_NewAccuColor =vec4(mixedColor, 1.0);
		
		Out_NewHistoryLength = historylength + 1;
	}
}