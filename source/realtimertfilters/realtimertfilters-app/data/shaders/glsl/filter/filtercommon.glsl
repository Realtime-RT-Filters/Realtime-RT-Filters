layout(push_constant) uniform constants
{
	uint SCR_WIDTH;
	uint SCR_HEIGHT;
} PushC;

ivec2	iSCRDIM = ivec2(PushC.SCR_WIDTH, PushC.SCR_HEIGHT);
vec2	SCRDIM = vec2(PushC.SCR_WIDTH, PushC.SCR_HEIGHT);

// Normalized coordinates (topleft = 0, 0; bottomright = 1 - a, 1 - b, where a, b ~= 0 at higher resolutions)
layout(location = 0) in vec2 UV;

ivec2 TexelizeCoords(in vec2 normalizedCoords)
{
	return ivec2(normalizedCoords * SCRDIM);
}

vec2 NormalizeCoords(in ivec2 texelCoords)
{
	return vec2(texelCoords) / SCRDIM;
}

// Texelcoords (topleft = 0, 0; bottomright = SCR_WIDTH - 1, SCR_HEIGHT - 1)
ivec2 Texel = TexelizeCoords(UV);
