
struct S_HitPayload
{
	vec3	radiance;
	vec3	radianceDirect;
	vec3	radianceIndirect;
	vec3	attenuation;
	uint	seed;
	uint	depth;
	vec3	albedo;
	vec3	normal;
};

S_HitPayload InitHitpayload(in uint seed)
{
	S_HitPayload result;
	result.radiance = vec3(0.0);
	result.radianceDirect = vec3(0.0);
	result.radianceIndirect = vec3(0.0);
	result.attenuation = vec3(1.0);
	result.seed = seed;
	result.depth = 0;
	result.albedo = vec3(0.0);
	result.normal = vec3(0.0, 1.0, 0.0);
	return result;
}

