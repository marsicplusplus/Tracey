#ifndef __CURVE_HPP__
#define __CURVE_HPP__

#include "hittables/hittable.hpp"

class Curve : public Hittable {
	public:
		Curve(float uMin, float uMax, int material);
		bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override;
	private:
		const int mat;
		const float uMin;
		const float uMax;
};

#endif // __CURVE_HPP__
