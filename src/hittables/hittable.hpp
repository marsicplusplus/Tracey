#ifndef __HITTABLE_HPP__
#define __HITTABLE_HPP__

#include "defs.hpp"
#include "ray.hpp"

class Hittable {
	virtual bool hit(const Ray &ray, double tMin, double tMax, HitRecord &rec) const = 0;
};

#endif
