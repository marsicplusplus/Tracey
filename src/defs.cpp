#include "defs.hpp"

bool hitAABB(const Ray& ray, const AABB& bbox, float& distance) {
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

bool hitAABB(const Ray& ray, const glm::fvec4& minAABB, const glm ::fvec4& maxAABB, float& distance) {
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
inline uint32_t calcZOrder(int xPos, int yPos){
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
float lerp(float x, float y, float u) {
	return (x * (1.0 - u)) + (y * u);
}
glm::vec3 lerp(glm::vec3 x, glm::vec3 y, float u) {
	return x * (1.f - u) + y * u;
}
void expandBBox(AABB& bbox, glm::vec3 expandDimensions) {
	bbox.minX -= expandDimensions.x;
	bbox.minY -= expandDimensions.y;
	bbox.minZ -= expandDimensions.z;
	bbox.maxX += expandDimensions.x;
	bbox.maxY += expandDimensions.y;
	bbox.maxZ += expandDimensions.z;
}
bool overlaps(AABB& b1, AABB& b2) {
	bool x = (b1.maxX >= b2.minX) && (b1.minX <= b2.maxX);
	bool y = (b1.maxY >= b2.minY) && (b1.minY <= b2.maxY);
	bool z = (b1.maxZ >= b2.minZ) && (b1.minZ <= b2.maxZ);
	return (x && y && z);
}
void CoordinateSystem(const glm::fvec3 &v1, glm::fvec3 *v2, glm::fvec3 *v3){
	if (std::abs(v1.x) > std::abs(v1.y))
		*v2 = glm::fvec3(-v1.z, 0, v1.x) /
			std::sqrt(v1.x * v1.x + v1.z * v1.z);
	else
		*v2 = glm::fvec3(0, v1.z, -v1.y) /
			std::sqrt(v1.y * v1.y + v1.z * v1.z);
	*v3 = glm::cross(v1, *v2);
}
uint32_t Random::xorshift32( uint32_t& state ){
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return state;
}

float Random::RandomFloat( uint32_t& s ) 
{ return xorshift32(s) * 2.3283064365387e-10f; }

