/// 

// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint tea(uint val0, uint val1)
{
	uint v0 = val0;
	uint v1 = val1;
	uint s0 = 0;

	for (uint n = 0; n < 16; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}

	return v0;
}

// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg(inout uint prev)
{
	uint LCG_A = 1664525u;
	uint LCG_C = 1013904223u;
	prev = (LCG_A * prev + LCG_C);
	return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint prev)
{
	return (float(lcg(prev)) / float(0x01000000));
}


//-------------------------------------------------------------------------------------------------
// Sampling
//-------------------------------------------------------------------------------------------------

// Randomly sampling around +Z
vec3 sampleLambert(inout uint seed, in mat3 reflectionTBN)
{
#define M_PI 3.141592

	float r1 = rnd(seed);
	float r2 = rnd(seed);

	float cosTheta = sqrt(1 - r1);
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	float phi = 2 * M_PI * r2;

	vec3 direction = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	direction = reflectionTBN * direction;

	return direction;
}

vec3 samplePhong(inout uint seed, in mat3 reflectionTBN, float shininess)
{
#define M_PI 3.141592

	float r1 = rnd(seed);
	float r2 = rnd(seed);

	float cosTheta = pow(1 - r1, 1 / (1 + shininess));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	float phi = 2 * M_PI * r2;

	vec3 direction = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	direction = reflectionTBN * direction;

	return direction;
}

vec3 sampleGGX(inout uint seed, in mat3 reflectionTBN, float alpha)
{
#define M_PI 3.141592

	float r1 = rnd(seed);
	float r2 = rnd(seed);

	float cosTheta = sqrt((1 - r1) / ((alpha * alpha - 1) * r1 + 1));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	float phi = 2 * M_PI * r2;

	vec3 direction = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	direction = reflectionTBN * direction;

	return direction;
}

mat3 createTBN(in vec3 N)
{
	vec3 T;
	if (abs(N.x) > abs(N.y))
		T = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else
		T = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	vec3 B = cross(N, T);

	return mat3(T, B, N);
}