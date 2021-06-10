#version 450

layout (set=1, binding = 0) uniform sampler2D samplerColor;
layout (set=1, binding = 1) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec3 inWorldPos;			// Vertex position in world space
layout (location = 1) in vec4 inDevicePos;			// Vertex position in normalized device space (current frame)
layout (location = 2) in vec4 inOldDevicePos;		// Vertex position in normalized device space (previous frame)
layout (location = 3) in vec3 inNormal; 			// Normal in world space
layout (location = 4) in vec3 inTangent;			// Tangent in world space
layout (location = 5) in vec2 inUV;				// UV coordinates
layout (location = 6) in vec3 inColor;				// Passthrough for vertex color
layout (location = 7) flat in int inMeshId;			// Mesh id

layout (location = 0) out vec4 outPosition;			// Fragment position in world spcae
layout (location = 1) out vec4 outNormal;			// Fragment normal in world space
layout (location = 2) out vec4 outAlbedo;			// Fragment raw albedo
layout (location = 3) out vec2 outMotion;			// Fragment screenspace motion delta
layout (location = 4) out int outMeshId;			// Fragment mesh id

void main() 
{
	outPosition = vec4(inWorldPos, 1.0);

	// Calculate normal in tangent space
	vec3 texNormal = texture(samplerNormalMap, inUV).xyz;
	if (texNormal == vec3(0.0))
	{
		outNormal = vec4(normalize(inNormal), 0.0);
	}
	else
	{
		vec3 N = normalize(inNormal);
		vec3 T = normalize(inTangent);
		vec3 B = cross(N, T);
		mat3 TBN = mat3(T, B, N);
		vec3 tnorm = TBN * normalize(texNormal * 2.0 - vec3(1.0));
		outNormal = vec4(tnorm, 0.0);
	}

	// Get albedo. If texture yields full black (probably hasn't been set) and the 
	vec4 textureColor = texture(samplerColor, inUV);
	if (textureColor.xyz == vec3(0.f) && inColor != vec3(0.f))
	{
		textureColor = vec4(inColor, 1.f);
	}
	outAlbedo =	textureColor;

	// Calculate motion delta
	vec2 screenPos = inDevicePos.xy / inDevicePos.w;
	vec2 old_screenPos = inOldDevicePos.xy / inOldDevicePos.w;
   	outMotion = (old_screenPos-screenPos) * 0.5;

	outMeshId = inMeshId;
}