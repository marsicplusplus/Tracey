#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"
#include <memory>
#include <algorithm>

template<typename T> T max(T a, T b) { return (a > b) ? a : b; }
template<typename T> T min(T a, T b) { return (a < b) ? a : b; }

struct AABB {
public:
	AABB() {};
	AABB(float _minX, float _minY, float _minZ, float _maxX, float _maxY, float _maxZ) : minX{_minX}, minY{_minY}, minZ{_minZ}, maxX{_maxX}, maxY{_maxY}, maxZ{_maxZ} {}

	float minX;
	float minY;
	float minZ;
	float maxX;
	float maxY;
	float maxZ;
};

class Hittable {
public:
	Hittable() : transform{ glm::fmat4x4(1.0) }, transformInv{ glm::fmat4x4(1.0) }, transposeInv{ glm::fmat4x4(1.0) } {}

		virtual bool hitSelf(const Ray& ray, float tMin, float tMax, HitRecord& rec) const = 0;
		virtual bool hitAABB(const Ray& ray, AABB bbox) const {
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

			return (tmax > max(tmin, 0.0f));
		}

		inline void translate(glm::fvec3 t){
			transform = glm::translate(transform, t);
			transformInv = glm::inverse(transform);
			updateTranspose();
		}
		inline void scale(glm::fvec3 s){
			transform = glm::scale(transform, s);
			transformInv = glm::inverse(transform);
			updateTranspose();
		}
		inline void scale(float s){
			transform = glm::scale(transform, glm::fvec3(s,s,s));
			transformInv = glm::inverse(transform);
			updateTranspose();
		}
		inline void rotate(float t, glm::fvec3 a){
			transform = glm::rotate(transform, t, a);
			transformInv = glm::inverse(transform);
			updateTranspose();
		}

private:
		inline void updateTranspose() {
			transposeInv = glm::transpose(transformInv);
		}

	protected:
		glm::fvec3 position;
		glm::fmat4x4 transformInv;
		glm::fmat4x4 transform;
		glm::fmat4x4 transposeInv;
};

typedef std::shared_ptr<Hittable> HittablePtr;

#endif
