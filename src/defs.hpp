#ifndef __DEFS_HPP__
#define __DEFS_HPP__

#include "glm/vec3.hpp"
#include "ray.hpp"
#include "materials/material.hpp"
#include <cmath>
#include <limits>

#define W_WIDTH 480
#define W_HEIGHT 480

#define V_TILES 20
#define H_TILES 20

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

#endif
