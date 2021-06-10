#ifndef ubo_definitions_glsl
#define ubo_definitions_glsl 0
/// LIGHT UBO
/// Size: 8 * 4 = 32 Byte
/// Position = Position in World Space
/// Type = type of the light source. < 0 = disabled, 1 = Pointlight, 2 = Directional Light
/// Color = Normalized color, determines the ratio of power between 3 color wavelengths
/// RadiantFlux = Power of the light source in Watt

#ifdef __cplusplus

using uint = uint32_t;

struct S_Light
{
	glm::vec3	Position;
	float		Type;
	glm::vec3	Color;
	float		RadiantFlux;

	S_Light() : Position(), Color(0.6f), Type(1.f), RadiantFlux(1.f) {}
};

#else
struct S_Light
{
	vec3		Position;
	float		Type;
	vec3		Color;
	float		RadiantFlux;
};
#endif


/// SCENEINFO UBO
#define UBO_SCENEINFO_LIGHT_COUNT 3
// Size: 32 * UBO_SCENEINFO_LIGHT_COUNT + 100 * 4
// Lights = Array of S_Light structs
// ViewMat = camera view matrix
// ProjMat = camera projection matrix
// ViewMatPrev = previous frame camera view matrix
// ProjMatPrev = previous frame camera projection matrix
// ViewMatInverse = inverse camera view matrix
// ProjMatInverse = inverse camera projection matrix
// ViewPos = camera position in world space
// _RESERVED = unused variable to maintain std140 spec compatibility (total size = multiples of 16 bytes)

#ifdef __cplusplus

struct S_Sceneinfo
{
	S_Light		Lights[UBO_SCENEINFO_LIGHT_COUNT];
	glm::mat4	ViewMat;
	glm::mat4	ProjMat;
	glm::mat4	ViewMatPrev;
	glm::mat4	ProjMatPrev;
	glm::mat4	ViewMatInverse;
	glm::mat4	ProjMatInverse;
	glm::vec3	ViewPos;
	float		_RESERVED;

	S_Sceneinfo() : Lights(), ViewPos(), ViewMat(), ProjMat(), ViewMatPrev(), ProjMatPrev(), ViewMatInverse(), ProjMatInverse(), _RESERVED() {}
};

#endif

#ifdef BIND_SCENEINFO
#ifndef SET_SCENEINFO
#define SET_SCENEINFO 0
#endif
layout(set = SET_SCENEINFO, binding = BIND_SCENEINFO) uniform S_Sceneinfo
{
	S_Light		Lights[UBO_SCENEINFO_LIGHT_COUNT];
	mat4		ViewMat;
	mat4		ProjMat;
	mat4		ViewMatPrev;
	mat4		ProjMatPrev;
	mat4		ViewMatInverse;
	mat4		ProjMatInverse;
	vec3		ViewPos;
	float		_RESERVED;
} ubo_sceneinfo;
#endif

/// Pathtracer Config PushConstant
/// Size: 9 * 4 = 36 Byte (Alignment is not important for push constants)

#ifdef __cplusplus

struct SPC_PathtracerConfig
{
	glm::vec4	ClearColor;
	uint		Frame;
	uint		PrimarySamplesPerPixel;
	uint		MaxBounceDepth;
	uint		SecondarySamplesPerBounce;
	uint		VertexSize;

	SPC_PathtracerConfig() : ClearColor(), Frame(), PrimarySamplesPerPixel(1), MaxBounceDepth(1), SecondarySamplesPerBounce(1), VertexSize(0) {}
};

#endif
#ifdef PUSHC_PATHTRACERCONFIG
layout (push_constant) uniform SPC_PathtracerConfig
{
	vec4		ClearColor;
	uint		Frame;
	uint		PrimarySamplesPerPixel;
	uint		MaxBounceDepth;
	uint		SecondarySamplesPerBounce;
	uint		VertexSize;
} config;
#endif

/// GUI BASE UBO
/// Size: 4 * 4 = 16 Byte
/// AttachmentIndex = Index of attachment to show
/// DoComposition = If true, AttachmentIndex is ignored, and instead the first 3 attachments are interpreted as worldpos, worldnormal and albedo respectively

#ifdef __cplusplus

struct S_Guibase
{
	uint		AttachmentIndex;
	uint		DoComposition;
	uint		_RESERVED1;
	uint		_RESERVED2;

	S_Guibase() : AttachmentIndex(), DoComposition(1), _RESERVED1(), _RESERVED2() {}
};

#endif
#ifdef BIND_GUIBASE
#ifndef SET_GUIBASE
#define SET_GUIBASE 0
#endif 

layout(set = SET_GUIBASE, binding = BIND_GUIBASE) uniform S_Guibase
{
	uint		AttachmentIndex;
	uint		DoComposition;
	uint		_RESERVED1;
	uint		_RESERVED2;
} ubo_guibase;
#endif


/// ACCU UBO
/// Size: 16 byte
/// EnableAccumulation = If 0, accumulation is skipped
/// MaxPosDifference = maximum squared distance of current and previous world position of a fragment
/// MaxNormalAngleDifference = maximum angle difference in radians between current and previous worldspace normals of a fragment
/// MinNewWeight = Minimum weight that new data is mixed with old (accumulated)

#ifdef __cplusplus

struct S_AccuConfig
{
	int		EnableAccumulation;
	float		MaxPosDifference;
	float		MaxNormalAngleDifference;
	float		MinNewWeight;

	S_AccuConfig() : EnableAccumulation(1), MaxPosDifference(0.001), MaxNormalAngleDifference(0.01), MinNewWeight(0.1f) {}
};

#endif
#ifdef BIND_ACCUCONFIG
#ifndef SET_ACCUCONFIG
#define SET_ACCUCONFIG 0
#endif 

layout(set = SET_ACCUCONFIG, binding = BIND_ACCUCONFIG) uniform S_AccuConfig
{
	int		EnableAccumulation;
	float		MaxPosDifference;
	float		MaxNormalAngleDifference;
	float		MinNewWeight;
} ubo_accuconfig;
#endif

#endif