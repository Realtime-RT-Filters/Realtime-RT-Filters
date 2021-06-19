#version 420

// GBUFFER INPUTS
layout (set = 0, binding = 0) uniform sampler2D Tex_CurPos;				// GBuffer Worldspace Positions
layout (set = 0, binding = 1) uniform sampler2D Tex_PrevPos;			// Previous frame Worldspace Positions
layout (set = 0, binding = 2) uniform sampler2D Tex_CurNormal;			// GBuffer Worldspace Normals
layout (set = 0, binding = 3) uniform sampler2D Tex_PrevNormal;			// Previous frame Worldspace Normals
layout (set = 0, binding = 4) uniform sampler2D Tex_Motion;				// Screenspace motion vector
layout (set = 0, binding = 5) uniform sampler2D Tex_Albedo;				// Albedo

// PATHTRACER INPUTS/OUTPUTS
layout (set = 0, binding = 6) uniform sampler2D Tex_RtDirect;			// Raw RT input
layout (set = 0, binding = 7) uniform sampler2D Tex_RtIndirect;			// Raw RT input

// ACCUMULATION SPECIFIC VARIABLES
layout (set = 0, binding = 8, rgba32f) uniform image2D Tex_Direct_Color_History;
layout (set = 0, binding = 9, rgba32f) uniform image2D Tex_Indirect_Color_History;
layout (set = 0, binding = 10, rgba32f) uniform image2D Tex_Moments_History;
layout (set = 0, binding = 11) uniform isampler2D Tex_OldHistoryLength;	// Per pixel accumulated data age

// SVGF SHADER OUTPUT
layout (location = 0) out vec4 Out_IntegratedDirectColor;				// Integrated direct color to be passed to atrous
layout (location = 1) out vec4 Out_IntegratedIndirectColor;				// Integrated indirect color to be passed to atrous
layout (location = 2) out int  Out_NewHistoryLength;					// Updated history length
layout (location = 3) out vec4 Out_NewMoments;							// Updated moments

#include "../filter/filtercommon.glsl"

#define BIND_ACCUCONFIG 0
#define SET_ACCUCONFIG 1
#include "../ubo_definitions.glsl"

float luminance(vec3 color)
{
    return dot(color.rgb, vec3(0.299f, 0.587f, 0.114f));
}

void main()
{
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

	bool keepHistoryValues = (distanceSquared < ubo_accuconfig.MaxPosDifference) &&		// If worldspace position difference is too great, we discard
		(angleDiff < ubo_accuconfig.MaxNormalAngleDifference);				// If worldspace normal angle deviation is too great, we discard

	// get color values
	vec3 directColor = texelFetch(Tex_RtDirect, Texel, 0).rgb;
	vec3 indirectColor = texelFetch(Tex_RtIndirect, Texel, 0).rgb;

	// compute moments first & second moments
	vec4 moments;
    moments.r = luminance(directColor); // first moment dir color
    moments.b = luminance(indirectColor); // first moment indir color
    moments.g = moments.r * moments.r; // 2nd moment dir color
    moments.a = moments.b * moments.b; // 2nd moment indir color

	vec3 integratedDirectColor;
	vec3 integratedIndirectColor;
	vec4 integratedMoments;

	if (keepHistoryValues)
	{
		int historylength = texelFetch(Tex_OldHistoryLength, texel_prevFrame, 0).x;	// HistoryLength = number of previous frames that we didn't yet discard

		// History = 0: Previous pixel didn't exist (could only happen in very rare circumstances in the very first frame) => We mix raw 1, accu 0
		// History = 1: History was discarded previously => We mix raw 0.5, accu 0.5
		// History > 1: History is pretty old and useful => We mix up to raw MinNewWeight, accu 1 - MinNewWeight
		float mixFactor = max(ubo_accuconfig.MinNewWeight, 1.f / (historylength + 1));

		// TODO: Missing: SVGF uses a 2x2 tap to better resample the previous color. See loadPrevData in SVGFReproject.ps.hlsl
		// This can be done to improve quality later on
		vec3 directColorHistory = imageLoad(Tex_Direct_Color_History, texel_prevFrame).rgb;
		Out_IntegratedDirectColor = vec4(mix(directColorHistory, directColor,  mixFactor), 1.0f);

		vec3 indirectColorHistory = imageLoad(Tex_Indirect_Color_History, texel_prevFrame).rgb;
		Out_IntegratedIndirectColor = vec4(mix(indirectColorHistory, indirectColor,  mixFactor), 1.0f);

		vec4 momentsHistory = imageLoad(Tex_Moments_History, texel_prevFrame).rgba;
		Out_NewMoments = mix(momentsHistory, moments, mixFactor);

		// update history length
		Out_NewHistoryLength = historylength + 1;
	}
	else
	{
		// no history
		Out_IntegratedDirectColor = vec4(directColor, 1.0f);
		Out_IntegratedIndirectColor = vec4(indirectColor, 1.0f);
		Out_NewMoments = moments;

		// discarded history -> set history length to 1
		Out_NewHistoryLength = 1;
	}

	vec2 variance = max(vec2(0,0), integratedMoments.ga - integratedMoments.rb * integratedMoments.rb);

	Out_IntegratedDirectColor.a = variance.x;
	Out_IntegratedIndirectColor.a = variance.y;

	// first wavelet iteration

	//ivec2 iCoords = ivec2(gl_FragCoord.x, gl_FragCoord.y);
	//imageStore(Tex_Moments_Direct, iCoords, vec4(1));

	// we get direct & indirect input
	// for each:
		/*
			a color history
			
			colorIn

			integratedColor = prevColor + colorIn
			newColor = atrous(integratedColor);
			prevColor = newColor;
			[x4] newColor = atrous(newColor);
			output = newColor

			a moments history
		*/

	

	// compute integrated color
	// compute integrated moments
	// update moments

	// compute variance

	// guide one atrous iteration
	
	// update color history

	// n - atrous iterations (update variance in each)

	// after atrous:

	// connect indirect and direct lightning
}
