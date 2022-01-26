#ifndef __CURVE_HPP__
#define __CURVE_HPP__

#include "hittables/hittable.hpp"

class CurveCommon {
public:
	CurveCommon(glm::fvec3 cPts[4], float width0, float width1) :
		curvePoints{ cPts[0], cPts[1], cPts[2], cPts[3]},
		width{ width0, width1 } {

	};

	const glm::fvec3 curvePoints[4];
	const float width[2];
};

class Curve : public Hittable {
	public:
		Curve(float uMin, float uMax, bool isClosed, int mat, const std::shared_ptr<CurveCommon>& common);
		bool hit(const Ray& ray, float tMin, float tMax, HitRecord& rec) const override;
		glm::fvec3 BlossomBezier(const glm::fvec3 cPts[4], float u0, float u1, float u2);

	private:
		const std::shared_ptr<CurveCommon> common;
		const bool isClosed;
		const int mat;
		float uMin, uMax;
};

#endif // __CURVE_HPP__
