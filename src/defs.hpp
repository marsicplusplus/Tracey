#ifndef __DEFS_HPP__
#define __DEFS_HPP__

#include "materials/material.hpp"
#include "glm/vec3.hpp"
#include "ray.hpp"
#include <random>
#include <cmath>
#include <limits>
#include <memory>

const float PI = 3.141592653589793238463L;
const float INF = std::numeric_limits<float>::infinity();

#define CHECK_ERROR(COND, MESSAGE, RET) if(!(COND)){\
	std::cerr << (MESSAGE);\
	return (RET);\
}

typedef glm::fvec3 Color;
class Material;

struct HitRecord {
	bool frontFace;
	int material;
	float u;
	float v;
	float t;
	glm::fvec3 p;
	glm::fvec3 normal;

	inline void setFaceNormal(const Ray& r, const glm::fvec3& outNormal) {
		frontFace = dot(r.getDirection(), outNormal) < 0;
		normal = frontFace ? glm::normalize(outNormal) : glm::normalize(-outNormal);
	}
};
inline float randomfloat(std::mt19937 &gen, float min, float max){
	std::uniform_real_distribution<float> dist(min, max);
	return dist(gen);
}


#endif
