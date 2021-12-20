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
const float INF = std::numeric_limits<float>::infinity();

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
	float t;
	glm::fvec3 p = glm::fvec3(INF, INF, INF);
	glm::fvec3 normal;

	inline void setFaceNormal(const Ray& r, const glm::fvec3& outNormal) {
		frontFace = dot(r.getDirection(), outNormal) < 0;
		normal = frontFace ? glm::normalize(outNormal) : glm::normalize(-outNormal);
	}
};

struct RayPacket {
	Ray ray;
	HitRecord rec;
};

template<typename T> T max(T a, T b) { return (a > b) ? a : b; }
template<typename T> T min(T a, T b) { return (a < b) ? a : b; }

inline bool hitAABB(const Ray& ray, const AABB& bbox, float& distance) {
	float tmin = -INFINITY, tmax = INFINITY;
	auto origin = ray.getOrigin();
	auto rayDirInv = ray.getInverseDirection();

	float tx1 = (bbox.minX - origin.x) * rayDirInv.x;
	float tx2 = (bbox.maxX - origin.x) * rayDirInv.x;

	tmin = max(tmin, min(tx1, tx2));
	tmax = min(tmax, max(tx1, tx2));

	float ty1 = (bbox.minY - origin.y) * rayDirInv.y;
	float ty2 = (bbox.maxY - origin.y) * rayDirInv.y;

	tmin = max(tmin, min(ty1, ty2));
	tmax = min(tmax, max(ty1, ty2));

	float tz1 = (bbox.minZ - origin.z) * rayDirInv.z;
	float tz2 = (bbox.maxZ - origin.z) * rayDirInv.z;

	tmin = max(tmin, min(tz1, tz2));
	tmax = min(tmax, max(tz1, tz2));

	distance = max(tmin, 0.0f);
	return (tmax > distance);
}

inline bool hitAABB(const Ray& ray, const glm::fvec4& minAABB, const glm ::fvec4& maxAABB, float& distance) {
	return hitAABB(ray, { minAABB.x, minAABB.y, minAABB.z, maxAABB.x, maxAABB.y, maxAABB.z }, distance);
}

inline bool hitAABB(const Ray& ray, const AABB& bbox) {
	float distance = 0.0f;
	return hitAABB(ray, bbox, distance);
}

namespace Random {
	inline uint32_t xorshift32( uint32_t& state ){
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return state;
	}

	inline float RandomFloat( uint32_t& s ) 
	{ return xorshift32(s) * 2.3283064365387e-10f; }
};

#endif
