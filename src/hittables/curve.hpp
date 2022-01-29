#ifndef __CURVE_HPP__
#define __CURVE_HPP__

#include "hittables/hittable.hpp"

struct Cylinder {
	glm::fvec3 axis;
	glm::fvec3 oe;
	float rayMax;
	float de;
};

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
		glm::fvec3 BlossomBezier(const glm::fvec3 cPts[4], float u0, float u1, float u2) const;
		bool intersect(const Ray& ray, float tMin, float tMax, HitRecord& rec) const;
		bool recursiveIntersect(const Ray& ray, float tMin, float tMax, HitRecord& rec, const glm::fvec3 cPts[4], glm::mat4x4& rayToObject, float u0, float u1, int depth) const;
		void SubdivideBezier(const glm::fvec3 cp[4], glm::fvec3 cpSplit[7]) const;
		glm::fvec3 EvalBezier(const glm::fvec3 cp[4], float u, glm::fvec3* deriv) const;

	private:
		const std::shared_ptr<CurveCommon> common;
		Cylinder enclosingCylinder;
		const bool isClosed;
		const int mat;
		float uMin, uMax;

		void getLocalControlPoints(glm::fvec3 *pts) const;
		bool hitEnclosingCylinder(const Ray& ray) const;
};

#endif // __CURVE_HPP__
