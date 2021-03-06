
#ifdef __cplusplus
#pragma once
struct GltfShadeMaterial
{
	glm::vec4 baseColorFactor;
	glm::vec4 baseColorTextureId; // origin int
};
#endif

#ifndef __cplusplus
struct GltfShadeMaterial
{
	vec4	baseColorFactor;
	vec4	baseColorTextureId;
};

vec3 computeDiffuse(GltfShadeMaterial mat, vec3 lightDir, vec3 normal)
{
	// Lambertian
	float dotNL = max(dot(normal, lightDir), 0.0);
	return mat.baseColorFactor.xyz * dotNL;
}

vec3 computeSpecular(GltfShadeMaterial mat, vec3 viewDir, vec3 lightDir, vec3 normal)
{
	// Compute specular only if not in shadow
	const float kPi = 3.14159265;
	const float kShininess = 60.0;

	// Specular
	const float kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
	vec3 V = normalize(-viewDir);
	vec3 R = reflect(-lightDir, normal);
	float specular = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);

	return vec3(specular);
}
#endif
