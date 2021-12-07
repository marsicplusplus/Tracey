#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"
#include <memory>

class Hittable {
	public:
		virtual bool hit(const Ray &ray, float tMin, float tMax, HitRecord &rec) const = 0;
	protected:
		glm::fvec3 position;
};

typedef std::shared_ptr<Hittable> HittablePtr;

#endif
