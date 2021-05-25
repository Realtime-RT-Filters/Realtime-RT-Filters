#version 450

layout (location = 0) in vec4 inPos;				// Vertex position in model space
layout (location = 1) in vec2 inUV;					// UV coordinates
layout (location = 2) in vec3 inColor;				// Per vertex color
layout (location = 3) in vec3 inNormal;				// Vertex normal
layout (location = 4) in vec3 inTangent;			// Vertex tangent

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	mat4 old_projection;
	mat4 old_model;
	mat4 old_view;
} ubo;

layout (location = 0) out vec3 outWorldPos;			// Vertex position in world space
layout (location = 1) out vec4 outDevicePos;		// Vertex position in normalized device space (current frame)
layout (location = 2) out vec4 outOldDevicePos;		// Vertex position in normalized device space (previous frame)
layout (location = 3) out vec3 outNormal; 			// Normal in world space
layout (location = 4) out vec3 outTangent;			// Tangent in world space
layout (location = 5) out vec2 outUV;				// UV coordinates
layout (location = 6) out vec3 outColor;			// Passthrough for vertex color
//layout (location = 7) out uint outMeshId;			// Mesh Id. Currently not supported.

void main() 
{
	// Get transformations out of the way
	outWorldPos = vec3(ubo.model * inPos);
	gl_Position = ubo.projection * ubo.view * ubo.model * inPos;
	outDevicePos = gl_Position;
	outOldDevicePos = ubo.old_projection * ubo.old_view * ubo.old_model * inPos;
	
	outUV = inUV;

	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(ubo.model)));
	outNormal = mNormal * normalize(inNormal);	
	outTangent = mNormal * normalize(inTangent);
	
	// Set vertex color passthrough
	outColor = inColor;
}
