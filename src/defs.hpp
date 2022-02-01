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

struct RayInfo {
	Ray ray;
	int x;
	int y;
	HitRecord rec;
	Color pxColor = Color(0.4, 0.4, 0.4);
};

struct Frustum {
	glm::fvec3 normals[4];
	float offsets[4];
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
	return (tmax >= distance);
}

inline bool hitAABB(const Ray& ray, const glm::fvec4& minAABB, const glm ::fvec4& maxAABB, float& distance) {
	return hitAABB(ray, { minAABB.x, minAABB.y, minAABB.z, maxAABB.x, maxAABB.y, maxAABB.z }, distance);
}

inline bool hitAABB(const Ray& ray, const glm::fvec4& minAABB, const glm::fvec4& maxAABB) {
	float distance = 0.0f;
	return hitAABB(ray, minAABB, maxAABB, distance);
}

inline bool hitAABB(const Ray& ray, const AABB& bbox) {
	float distance = 0.0f;
	return hitAABB(ray, bbox, distance);
}

inline uint32_t calcZOrder(int xPos, int yPos)
{
	static const uint32_t MASKS[] = { 0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF };
	static const uint32_t SHIFTS[] = { 1, 2, 4, 8 };

	uint32_t x = xPos;  // Interleave lower 16 bits of x and y, so the bits of x
	uint32_t y = yPos;  // are in the even positions and bits from y in the odd;

	x = (x | (x << SHIFTS[3])) & MASKS[3];
	x = (x | (x << SHIFTS[2])) & MASKS[2];
	x = (x | (x << SHIFTS[1])) & MASKS[1];
	x = (x | (x << SHIFTS[0])) & MASKS[0];

	y = (y | (y << SHIFTS[3])) & MASKS[3];
	y = (y | (y << SHIFTS[2])) & MASKS[2];
	y = (y | (y << SHIFTS[1])) & MASKS[1];
	y = (y | (y << SHIFTS[0])) & MASKS[0];

	const uint32_t result = x | (y << 1);
	return result;
}

inline float lerp(float x, float y, float u)
{
	return (x * (1.0f - u)) + (y * u);
}

inline glm::vec3 lerp(glm::vec3 x, glm::vec3 y, float u) {
	return x * (1.f - u) + y * u;
}

inline void expandBBox(AABB& bbox, glm::vec3 expandDimensions) {
	bbox.minX -= expandDimensions.x;
	bbox.minY -= expandDimensions.y;
	bbox.minZ -= expandDimensions.z;
	bbox.maxX += expandDimensions.x;
	bbox.maxY += expandDimensions.y;
	bbox.maxZ += expandDimensions.z;
}


inline bool overlaps(AABB& b1, AABB& b2) {
	bool x = (b1.maxX >= b2.minX) && (b1.minX <= b2.maxX);
	bool y = (b1.maxY >= b2.minY) && (b1.minY <= b2.maxY);
	bool z = (b1.maxZ >= b2.minZ) && (b1.minZ <= b2.maxZ);
	return (x && y && z);
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
