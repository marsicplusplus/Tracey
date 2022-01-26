#ifndef __CURVE_HPP__
#define __CURVE_HPP__

#include "hittables/hittable.hpp"

class Curve : public Hittable {
	public:
		Curve(std::vector<glm::fvec3> cPts, bool isClosed, int mat);
		bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override;

	private:
		const std::vector<glm::fvec3> controlPoints;
		const bool isClosed;
		const int mat;
		float uMin;
		float uMax;
};

#endif // __CURVE_HPP__
