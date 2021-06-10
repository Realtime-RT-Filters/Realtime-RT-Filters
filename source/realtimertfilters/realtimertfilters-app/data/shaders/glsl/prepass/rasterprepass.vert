#version 450

layout (location = 0) in vec4 inPos;				// Vertex position in model space
layout (location = 1) in vec2 inUV;					// UV coordinates
layout (location = 2) in vec3 inColor;				// Per vertex color
layout (location = 3) in vec3 inNormal;				// Vertex normal
layout (location = 4) in vec3 inTangent;			// Vertex tangent
layout (location = 5) in int inMeshId;				// Mesh Id

#define BIND_SCENEINFO 0
#include "../ubo_definitions.glsl"

layout (location = 0) out vec3 outWorldPos;			// Vertex position in world space
layout (location = 1) out vec4 outDevicePos;		// Vertex position in normalized device space (current frame)
layout (location = 2) out vec4 outOldDevicePos;		// Vertex position in normalized device space (previous frame)
layout (location = 3) out vec3 outNormal; 			// Normal in world space
layout (location = 4) out vec3 outTangent;			// Tangent in world space
layout (location = 5) out vec2 outUV;				// UV coordinates
layout (location = 6) out vec3 outColor;			// Passthrough for vertex color
layout (location = 7) flat out int outMeshId;			// Mesh Id

//const mat4 MODELMATRIX = mat4(1.0f);
//const mat4 PREVMODELMATRIX = MODELMATRIX;
//
void main() 
{
	// Get transformations out of the way
	outWorldPos = inPos.xyz;
	gl_Position = ubo_sceneinfo.ProjMat * ubo_sceneinfo.ViewMat * inPos;
	outDevicePos = gl_Position;
	outOldDevicePos = ubo_sceneinfo.ProjMatPrev * ubo_sceneinfo.ViewMatPrev * inPos;
//	outWorldPos = (MODELMATRIX * inPos).xyz;
//	gl_Position = ubo_sceneinfo.ProjMat * ubo_sceneinfo.ViewMat * MODELMATRIX * inPos;
//	outDevicePos = gl_Position;
//	outOldDevicePos = ubo_sceneinfo.ProjMatPrev * ubo_sceneinfo.ViewMatPrev * PREVMODELMATRIX * inPos;

	outUV = inUV;

	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(1.0)));
	outNormal = mNormal * normalize(inNormal);	
	outTangent = mNormal * normalize(inTangent);
	
	// Set vertex color passthrough
	outColor = inColor;

	outMeshId = inMeshId;
}
