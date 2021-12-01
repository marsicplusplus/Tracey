#ifndef __DEFS_HPP__
#define __DEFS_HPP__

#include "materials/material.hpp"
#include "glm/vec3.hpp"
#include "ray.hpp"
#include <random>
#include <cmath>
#include <limits>
#include <memory>

#define PI 3.1415926535897932385

const double INF = std::numeric_limits<double>::infinity();

#define CHECK_ERROR(COND, MESSAGE, RET) if(!(COND)){\
	std::cerr << (MESSAGE);\
	return (RET);\
}

typedef glm::dvec3 Color;
class Material;

struct HitRecord {
	glm::dvec3 p;
	glm::dvec3 normal;
	double t;
	bool frontFace;
	std::shared_ptr<Material> material;
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


#endif
