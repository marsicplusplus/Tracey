#ifndef __DEFS_HPP__
#define __DEFS_HPP__

#include "glm/vec3.hpp"
#include <cmath>
#include <limits>

#define W_WIDTH 1280
#define W_HEIGHT 720

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
};

#endif
