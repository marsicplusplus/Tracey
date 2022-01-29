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
		bool hitPBRT(const Ray& ray, float tMin, float tMax, HitRecord& rec) const;
		bool hitPhantom(const Ray& ray, float tMin, float tMax, HitRecord& rec) const;
		glm::fvec3 BlossomBezier(const glm::fvec3 cPts[4], float u0, float u1, float u2) const;
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

struct RayConeIntersection{ // ray.o = {0,0,0}; ray.d = {0,0,1};
	inline bool intersect(float r , float dr) {
		// cone is defined by base center c0 , radius r ,
		// axis cd , and slant dr
		float r2 = r * r; // dr could be either positive
		float drr = r * dr; // or negative (0 for cylinder)

		float ddd = cd.x * cd.x + cd.y * cd.y; // all possible
		dp = c0.x * c0.x + c0.y * c0.y; // combinations
		float cdd = c0.x * cd.x + c0.y * cd.y; // of x∗y terms
		float cxd = (c0.x * cd.y) - (c0.y * cd.x); // (c0 × cd)z

		float c = ddd; // compute a, b, c in
		float b = cd.z * (drr - cdd); // a − 2bs + cs2
		float cdz2 = cd.z * cd.z; // (s for ray ∩ cone)
		ddd += cdz2; // now it is cd·cd
		float a = 2 * drr * cdd + cxd * cxd - ddd * r2 + dp * cdz2;
		#if defined(KEEP_DR2) // dr2 adjustments
		float qs = (dr ∗ dr) / ddd; // ( it does not help
		a − = qs ∗ cdd∗cdd; // much with neither
		b − = qs ∗ cd.z ∗cdd; // performance nor
		c − = qs ∗ cdz2; // accuracy )
		#endif

		// We will add c0.z to s and splatter if needed
		float det = b * b - a * c; // for a − 2bs + cs2
		s = (b  -(det > 0 ? sqrt(det) : 0)) / c; // c > 0
		dt = (s * cd.z - cdd) / ddd; // wrt t
		dc = s * s + dp; // | (ray ∩ cone) − c0 | 2
		sp = cdd / cd.z; // will add c0.z latter
		dp += sp * sp; // | (ray ∩ plane) − c0 | 2

		return det > 0; // true (real) or false (phantom)
	 }

	glm::fvec3 c0; // curve (t) in RCC (base center)
	glm::fvec3 cd; // tangent (t) in RCC (cone's axis)
	float s; // ray.s − c0.z for ray ∩ cone (t)
	float dt; // dt to the (ray ∩ cone) from t
	float dp; // | (ray ∩ plane (t)) − curve (t) | 2
	float dc; // | (ray ∩ cone (t)) − curve (t) | 2
	float sp; // ray.s − c0.z for ray ∩ plane (t)
};

#endif // __CURVE_HPP__
