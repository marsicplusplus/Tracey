#ifndef __DEFS_HPP__
#define __DEFS_HPP__

#include "materials/material.hpp"
#include "glm/vec3.hpp"
#include "ray.hpp"
#include "thread_pool.hpp"
#include <cmath>
#include <limits>
#include <memory>

const float PI = 3.141592653589793238463f;
const float INVPI = 0.3183098861837907f;
const float INF = std::numeric_limits<float>::infinity();
const float EPS = std::numeric_limits<float>::epsilon();

#define CHECK_ERROR(COND, MESSAGE, RET) if(!(COND)){\
	std::cerr << (MESSAGE);\
	return (RET);\
}

typedef glm::fvec3 Color;
class Material;

struct AABB {
	float minX;
	float minY;
	float minZ;
	float maxX;
	float maxY;
	float maxZ;
};

struct HitRecord {
	bool frontFace;
	int material;
	float u;
	float v;
	float t = INF;
	glm::fvec3 p = glm::fvec3(INF, INF, INF);
	glm::fvec3 normal;

	inline void setFaceNormal(const Ray& r, const glm::fvec3& outNormal) {
		frontFace = dot(r.getDirection(), outNormal) < 0;
		normal = frontFace ? glm::normalize(outNormal) : glm::normalize(-outNormal);
	}
};

template<typename T> T max(T a, T b) { return (a > b) ? a : b; }
template<typename T> T min(T a, T b) { return (a < b) ? a : b; }

bool hitAABB(const Ray& ray, const AABB& bbox, float& distance);

bool hitAABB(const Ray& ray, const glm::fvec4& minAABB, const glm ::fvec4& maxAABB, float& distance);

bool hitAABB(const Ray& ray, const glm::fvec4& minAABB, const glm::fvec4& maxAABB);

bool hitAABB(const Ray& ray, const AABB& bbox);

uint32_t calcZOrder(int xPos, int yPos);

float lerp(float x, float y, float u);

glm::vec3 lerp(glm::vec3 x, glm::vec3 y, float u);

void expandBBox(AABB& bbox, glm::vec3 expandDimensions);

bool overlaps(AABB& b1, AABB& b2);

namespace Random {
	uint32_t xorshift32( uint32_t& state );
	float RandomFloat( uint32_t& s ) ;
};

#endif
