#ifndef __DEFS_HPP__
#define __DEFS_HPP__

#include "glm/vec3.hpp"
#include "ray.hpp"
#include "materials/material.hpp"
#include <random>
#include <cmath>
#include <limits>

#define W_WIDTH 640
#define W_HEIGHT 384
#define PI 3.1415926535897932385

const double INF = std::numeric_limits<double>::infinity();

#define CHECK_ERROR(COND, MESSAGE, RET) if(!(COND)){\
	std::cerr << (MESSAGE);\
	return (RET);\
}

typedef glm::dvec3 Color;

struct HitRecord {
	glm::dvec3 p;
	glm::dvec3 normal;
	double t;
	bool frontFace;
	MaterialPtr material;
	double u;
	double v;

	inline void setFaceNormal(const Ray& r, const glm::dvec3& outNormal) {
		frontFace = dot(r.getDirection(), outNormal) < 0;
		normal = frontFace ? outNormal : -outNormal;
	}
};
inline double randomDouble(std::mt19937 &gen, double min, double max){
	std::uniform_real_distribution<double> dist(min, max);
	return dist(gen);
}
inline glm::dvec3 randomVec3(std::mt19937 &gen, double min, double max) {
	return glm::dvec3(randomDouble(gen, min, max), randomDouble(gen, min, max), randomDouble(gen, min, max));
}
inline glm::dvec3 randomUnitSphere(std::mt19937 &gen) {
	while (true) {
		auto p = randomVec3(gen, -1.0, 1.0);
		if (glm::length2(p)>= 1) continue;
		return p;
	}
}
inline static glm::dvec3 randomUnitVector(std::mt19937 &gen) {
	return glm::normalize(randomUnitSphere(gen));
}

#endif
