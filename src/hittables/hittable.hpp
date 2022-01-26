#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"
#include "transform.hpp"
#include <memory>
#include <algorithm>

class Hittable {
public:
	Hittable(AABB aabb = AABB{ INF, INF, INF, -INF, -INF, -INF }) : localBBox{aabb}{}

	virtual bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const = 0;

	virtual bool update(float dt) { return false; }

	virtual inline void translate(glm::fvec3 t){}
	virtual inline void scale(glm::fvec3 s){}
	virtual inline void scale(float s){}
	virtual inline void rotate(float t, glm::fvec3 a){}

	inline AABB getLocalAABB() const {
		return localBBox;
	}
	virtual inline AABB getWorldAABB() const {
		return localBBox;
	}

	virtual inline void setLocalAABB(AABB local) {
		localBBox = local;
	}
	virtual inline void setTransform(const Transform t) {}

protected:
	inline void expandBBox(glm::vec3 expandDimensions) {
		localBBox.minX -= expandDimensions.x;
		localBBox.minY -= expandDimensions.y;
		localBBox.minZ -= expandDimensions.z;
		localBBox.maxX += expandDimensions.x;
		localBBox.maxY += expandDimensions.y;
		localBBox.maxZ += expandDimensions.z;
	}

protected:
	AABB localBBox;
};

typedef std::shared_ptr<Hittable> HittablePtr;

#endif
